//===- LowerContractionToRISCVIMEVMADOTPattern.cpp - Contract to IME ----===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements lowering patterns from vector.contract to operations
// that map to instructions from SpacemiT's Integer Matrix Extension (IME):
// riscv_ime.vmadot / vmadotu / vmadotsu / vmadotus.
//
// Structural reference: mlir/lib/Dialect/ArmNeon/Transforms/
// LowerContractToNeonPatterns.cpp. Two differences from that file, both
// deliberate:
//
//   1. ArmNeon's I8MM path only has three real hardware ops (smmla, ummla,
//      usmmla) and emulates the fourth signed/unsigned combination by
//      swapping LHS/RHS and transposing the result. SpacemiT IME has all
//      four combinations as real instructions (vmadot, vmadotu, vmadotsu,
//      vmadotus), so no operand-swap emulation is needed here at all.
//
//   2. ArmNeon's pattern explicitly rejects scalable vectors (its own
//      comment points to LowerContractToSVEPatterns.cpp as the intended
//      scalable-vector counterpart). RVV vectors are scalable by nature, so
//      this file is written to accept scalable vectors -- it is, in effect,
//      the RISC-V analogue of that SVE counterpart file.
//
//
// lower() tiles the 2D contraction into (4,4,8) M/N/K sub-tiles, matching
// IREE's own cost model (CPUEncodingExternalModels.cpp: "IME 12x16x8 tile
// (3x4 of 4x4x8)" at VLEN=256), emitting one hardware call per sub-tile and
// accumulating across K. Hardcoded for VLEN=256 (+zvl256b); generalizing to
// other VLEN values is separate follow-up work.
//===----------------------------------------------------------------------===//

#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/RISCVIME/RISCVIMEDialect.h"
#include "mlir/Dialect/RISCVIME/Transforms.h"
#include "mlir/Dialect/UB/IR/UBOps.h"
#include "mlir/Dialect/Utils/IndexingUtils.h"
#include "mlir/Dialect/Vector/IR/VectorOps.h"
#include "mlir/IR/AffineMap.h"
#include "mlir/IR/PatternMatch.h"

#define DEBUG_TYPE "lower-contract-to-riscv-ime"

using namespace mlir;
using namespace mlir::riscv_ime;

namespace {

/// Get the operand of a `vector.contract`. Abstracts away from the
/// particular way a value is extended before feeding it into the contract -
/// via zero-extend, or an explicit or implicit sign-extend (for implicit
/// sign-extension see `vector.contract` documentation).
///
/// The template parameter `Op` indicates the extension operation (explicit
/// or implicit) for which we are checking.
///
/// Return success only for extensions from `iN` (N <= 8) to `i32` -- this
/// directly implements Task 2.2's "check for i8 to i32 accumulation".
///
/// Identical in structure to ArmNeon's getExtOperand(); reused as-is since
/// this logic is dialect-agnostic (it only touches arith and builtin vector
/// types, nothing ArmNeon-specific).
template <typename Op>
std::optional<Value> getExtOperand(Value v) {

  static_assert(llvm::is_one_of<Op, arith::ExtSIOp, arith::ExtUIOp>::value,
                "Must be instantiated with either sign- or zero- extension op");

  // If the operand is not defined by an explicit extend operation of the
  // accepted operation type, allow for an implicit sign-extension.
  auto extOp = v.getDefiningOp<Op>();
  if (!extOp) {
    if constexpr (std::is_same<Op, arith::ExtSIOp>::value) {
      auto eltTy = cast<VectorType>(v.getType()).getElementType();
      if (!eltTy.isSignlessInteger() || eltTy.getIntOrFloatBitWidth() > 8)
        return {};
      return v;
    }
    return {};
  }

  // If the operand is defined by an explicit extend operation of the
  // accepted operation type, check it's extended from `iN` (N <= 8) to
  // `i32`.
  auto inOp = extOp.getIn();
  auto inTy = dyn_cast<VectorType>(inOp.getType());
  if (!inTy)
    return {};
  auto inEltTy = inTy.getElementType();
  if (!inEltTy.isSignlessInteger() || inEltTy.getIntOrFloatBitWidth() > 8)
    return {};

  auto outTy = dyn_cast<VectorType>(extOp.getType());
  if (!(outTy && outTy.getElementType().isSignlessInteger(32)))
    return {};

  return inOp;
}

/// Helper function to extend a vector with elements iN, N < 8 to a vector
/// of i8. Do sign extension if `signExt` is true, zero extension otherwise.
/// Identical to ArmNeon's extendSmallIntVector(); reused as-is.
Value extendSmallIntVector(Location loc, VectorType srcTy, Value val,
                            bool signExt, PatternRewriter &rewriter) {
  Type targetTy = srcTy.clone(rewriter.getI8Type());
  return signExt ? rewriter.createOrFold<arith::ExtSIOp>(loc, targetTy, val)
                 : rewriter.createOrFold<arith::ExtUIOp>(loc, targetTy, val);
}

/// Flatten a (possibly scalable) vector to 1D via vector.shape_cast.
/// Implements the "Handling Rank" part of Task 2.3.
Value flattenTo1D(PatternRewriter &rewriter, Location loc, Value v) {
  auto ty = cast<VectorType>(v.getType());
  int64_t flatSize = 1;
  for (int64_t d : ty.getShape())
    flatSize *= d;
  bool flatIsScalable = llvm::any_of(ty.getScalableDims(),
                                      [](bool b) { return b; });
  auto flatTy = VectorType::get({flatSize}, ty.getElementType(),
                                 ArrayRef<bool>{flatIsScalable});
  return rewriter.createOrFold<vector::ShapeCastOp>(loc, flatTy, v);
}

/// Un-flatten a 1D vector back to the given target (possibly 2D, possibly
/// scalable) shape via vector.shape_cast. The other half of "Handling Rank".
Value unflattenTo(PatternRewriter &rewriter, Location loc, Value v,
                   VectorType targetTy) {
  return rewriter.createOrFold<vector::ShapeCastOp>(loc, targetTy, v);
}

class VectorContractRewriter {
protected:
  // Designate which IME instruction is used for this contraction. Unlike
  // ArmNeon's MMLA enum, all four signed/unsigned combinations here map to
  // real hardware instructions -- there is no "MixedInt" case requiring
  // operand-swap emulation.
  enum class IMEOp {
    Nop,
    Vmadot,   // vs1 signed,   vs2 signed
    Vmadotu,  // vs1 unsigned, vs2 unsigned
    Vmadotsu, // vs1 signed,   vs2 unsigned
    Vmadotus, // vs1 unsigned, vs2 signed
  };

  IMEOp imeOp = IMEOp::Nop;

  // The operand tiles. These are not necessarily the operands of
  // `vector.contract` -- they may be operands to `arith.extsi`/`arith.extui`
  // that are in turn fed into `vector.contract` (see getExtOperand above).
  Value lhs;
  Value rhs;
  Value acc;

  // The dimensions logically corresponding to matrix multiplication of
  // MxK * KxN -> MxN. The operands and result do not necessarily have these
  // shapes, e.g. RHS could be NxK with a transposing indexing map.
  int64_t dimM = 0;
  int64_t dimN = 0;
  int64_t dimK = 0;

  // Create the vmadot-family operation according to `imeOp`. No operand
  // swapping is ever needed here -- see the class comment above.
  Value createIMEOp(PatternRewriter &rewriter, Location loc, Value acc,
                     Value lhs, Value rhs) {
    // Phase 1's SelectionDAG Pat only matches scalable RVV types
    // (nxv4i32/nxv8i8). Fixed-size flattened vectors fail to select at the
    // assembly stage. Wrap the fixed operands into their scalable base
    // type immediately before the hardware call, and unwrap the result
    // immediately after, via vector.scalable.insert/extract.
    //
    // Hardcodes vscale=4, matching VLEN=256 (+zvl256b).
    Type accElemTy = cast<VectorType>(acc.getType()).getElementType();
    Type inElemTy = cast<VectorType>(lhs.getType()).getElementType();
    VectorType accFixedTy = cast<VectorType>(acc.getType());
    VectorType accScalableTy = VectorType::get({4}, accElemTy, {true});
    VectorType inScalableTy = VectorType::get({8}, inElemTy, {true});

    auto toScalable = [&](Value fixedFlat, VectorType scalableTy) -> Value {
      Value poison = rewriter.create<ub::PoisonOp>(loc, scalableTy);
      return rewriter.createOrFold<vector::ScalableInsertOp>(
          loc, fixedFlat, poison, /*pos=*/0);
    };
    auto fromScalable = [&](Value scalableVal,
                             VectorType fixedFlatTy) -> Value {
      return rewriter.createOrFold<vector::ScalableExtractOp>(
          loc, fixedFlatTy, scalableVal, /*pos=*/0);
    };

    // vl = flattened operand's real element count. policy=0 is a
    // placeholder -- correct RVV tail/mask-policy encoding not yet
    // confirmed against the SpacemiT/RVV spec.
    Value vl = rewriter.create<arith::ConstantOp>(
        loc, rewriter.getI64IntegerAttr(
                 cast<VectorType>(lhs.getType()).getNumElements()));

    Value scalableAcc = toScalable(acc, accScalableTy);
    Value scalableLhs = toScalable(lhs, inScalableTy);
    Value scalableRhs = toScalable(rhs, inScalableTy);

    Value scalableResult;
    switch (imeOp) {
    case IMEOp::Vmadot:
      scalableResult = rewriter.createOrFold<riscv_ime::VmadotOp>(
          loc, accScalableTy, scalableAcc, scalableLhs, scalableRhs, vl,
          /*policy=*/0);
      break;
    case IMEOp::Vmadotu:
      scalableResult = rewriter.createOrFold<riscv_ime::VmadotUOp>(
          loc, accScalableTy, scalableAcc, scalableLhs, scalableRhs, vl,
          /*policy=*/0);
      break;
    case IMEOp::Vmadotsu:
      scalableResult = rewriter.createOrFold<riscv_ime::VmadotSUOp>(
          loc, accScalableTy, scalableAcc, scalableLhs, scalableRhs, vl,
          /*policy=*/0);
      break;
    case IMEOp::Vmadotus:
      scalableResult = rewriter.createOrFold<riscv_ime::VmadotUSOp>(
          loc, accScalableTy, scalableAcc, scalableLhs, scalableRhs, vl,
          /*policy=*/0);
      break;
    case IMEOp::Nop:
      llvm_unreachable("Uninitialized operation type");
    }

    return fromScalable(scalableResult, accFixedTy);
  }

LogicalResult matchAndInit(vector::ContractionOp op,
                            PatternRewriter &rewriter) {
  llvm::errs() << "DEBUG: base matchAndInit entered\n";
  SmallVector<vector::IteratorType> itTypes = op.getIteratorTypesArray();
  if ((itTypes.size() != 3 || itTypes[0] != vector::IteratorType::parallel ||
       itTypes[1] != vector::IteratorType::parallel ||
       itTypes[2] != vector::IteratorType::reduction) &&
      (itTypes.size() != 2 || itTypes[0] != vector::IteratorType::parallel ||
       itTypes[1] != vector::IteratorType::reduction))
    return rewriter.notifyMatchFailure(
        op, "iterator types do not correspond to matrix multiplication");

  VectorType lhsType = op.getLhsType();
  VectorType rhsType = op.getRhsType();
  if (!lhsType.hasRank() || !rhsType.hasRank() || lhsType.getRank() > 2 ||
      rhsType.getRank() != 2)
    return rewriter.notifyMatchFailure(op, "Invalid operand rank");

  // Derive dimM/dimN/dimK from the actual indexing maps rather than
  // assuming a fixed physical layout. vector.contract's 3-iterator space
  // follows the (m, n, k) convention; for each operand, find which
  // physical dimension its own indexing map assigns to each iterator.
  auto indexingMaps = op.getIndexingMapsArray();
  AffineMap lhsMap = indexingMaps[0];
  AffineMap rhsMap = indexingMaps[1];

  auto findPhysicalDim = [](AffineMap map,
                             unsigned iterDim) -> std::optional<int64_t> {
    for (unsigned physDim = 0; physDim < map.getNumResults(); ++physDim) {
      if (auto dimExpr = dyn_cast<AffineDimExpr>(map.getResult(physDim))) {
        if (dimExpr.getPosition() == iterDim)
          return physDim;
      }
    }
    return std::nullopt;
  };

  // Iterator convention: 0=m, 1=n, 2=k.
  auto rhsNDim = findPhysicalDim(rhsMap, /*n=*/1);
  auto rhsKDim = findPhysicalDim(rhsMap, /*k=*/2);
  if (!rhsNDim || !rhsKDim)
    return rewriter.notifyMatchFailure(
        op, "could not resolve N/K dims from RHS indexing map");

  dimN = rhsType.getDimSize(*rhsNDim);
  dimK = rhsType.getDimSize(*rhsKDim);

  int64_t lhsDimK;
  if (lhsType.getRank() == 1) {
    // Vecmat case: LHS has only the K dimension. Positional fallback
    // retained here — the 2-iterator vecmat indexing convention is less
    // standardized, so this path is not yet map-derived like the rank-2
    // case above.
    dimM = 1;
    lhsDimK = lhsType.getDimSize(0);
  } else {
    auto lhsMDim = findPhysicalDim(lhsMap, /*m=*/0);
    auto lhsKDim = findPhysicalDim(lhsMap, /*k=*/2);
    if (!lhsMDim || !lhsKDim)
      return rewriter.notifyMatchFailure(
          op, "could not resolve M/K dims from LHS indexing map");
    dimM = lhsType.getDimSize(*lhsMDim);
    lhsDimK = lhsType.getDimSize(*lhsKDim);
  }

  if (lhsDimK != dimK)
    return rewriter.notifyMatchFailure(op, "Dimensions mismatch");

  return success();
}

public:
  // Tiles the (possibly larger-than-one-call) 2D contraction into
  // (M0,N0,K0)=(4,4,8) sub-tiles -- matching IREE's own cost model
  // (CPUEncodingExternalModels.cpp: "IME 12x16x8 tile (3x4 of 4x4x8)" at
  // VLEN=256) -- emitting one hardware call per sub-tile and accumulating
  // across the K dimension before writing each M,N output tile back.
  //
  // Hardcoded for VLEN=256 (+zvl256b). Generalizing to other VLEN values
  // is separate follow-up work.
  void lower(vector::ContractionOp op, PatternRewriter &rewriter) {
    Location loc = op.getLoc();
    VectorType origAccType = cast<VectorType>(acc.getType());
    Type accElemTy = origAccType.getElementType();

    constexpr int64_t M0 = 4, N0 = 4, K0 = 8;

    SmallVector<int64_t> iterationBounds = {dimM, dimN, dimK};
    SmallVector<int64_t> subTileShape = {M0, N0, K0};
    SmallVector<int64_t> loopOrder = {0, 1, 2};

    Value result = rewriter.create<arith::ConstantOp>(
        loc, origAccType, rewriter.getZeroAttr(origAccType));

    auto extract2D = [&](Value v, ArrayRef<int64_t> off,
                          ArrayRef<int64_t> size) {
      SmallVector<int64_t> strides(off.size(), 1);
      return rewriter.createOrFold<vector::ExtractStridedSliceOp>(
          loc, v, off, size, strides);
    };

    Value kAcc;
    for (SmallVector<int64_t> offsets :
         StaticTileOffsetRange(iterationBounds, subTileShape, loopOrder)) {
      int64_t m = offsets[0], n = offsets[1], k = offsets[2];

      Value tiledLhs = extract2D(lhs, {m, k}, {M0, K0});
      Value tiledRhs = extract2D(rhs, {n, k}, {N0, K0});

      bool initialK = (k == 0);
      Value flatAcc;
      if (initialK) {
        Value tiledAcc = extract2D(acc, {m, n}, {M0, N0});
        flatAcc = flattenTo1D(rewriter, loc, tiledAcc);
      } else {
        flatAcc = kAcc;
      }

      Value flatLhs = flattenTo1D(rewriter, loc, tiledLhs);
      Value flatRhs = flattenTo1D(rewriter, loc, tiledRhs);

      kAcc = createIMEOp(rewriter, loc, flatAcc, flatLhs, flatRhs);

      bool isLastK = (k + K0 >= dimK);
      if (isLastK) {
        VectorType accTileType = VectorType::get({M0, N0}, accElemTy);
        Value tiledResult = unflattenTo(rewriter, loc, kAcc, accTileType);
        SmallVector<int64_t> strides = {1, 1};
        result = rewriter.createOrFold<vector::InsertStridedSliceOp>(
            loc, tiledResult, result, ArrayRef<int64_t>{m, n}, strides);
      }
    }

    rewriter.replaceOp(op, result);
  }
};

class VectorContractRewriterVMADOT : public VectorContractRewriter {
public:
  LogicalResult matchAndInit(vector::ContractionOp op,
                              PatternRewriter &rewriter) {
    if (failed(VectorContractRewriter::matchAndInit(op, rewriter)))
      return failure();
   llvm::errs() << "DEBUG: base matchAndInit passed, LHS type=" << op.getLhs().getType() << " RHS type=" << op.getRhs().getType() << "\n";
    // TODO: once a multi-call tiling loop is added (see file-level TODO),
    // this divisibility check should move here, analogous to ArmNeon's
    // `dimK % 8 != 0` check -- gated on the confirmed vmadot hardware tile
    // shape. For the current single-call implementation, dimM/dimN/dimK
    // are expected to already match the hardware shape exactly (i.e. IREE's
    // data-tiling has already produced a contraction of exactly one
    // hardware call's size); no divisibility check is performed here yet.

    // Check inputs are sign-/zero- extensions from iN (N <= 8) to i32 --
    // Task 2.2's "check for i8 to i32 accumulation", combined with Task
    // 2.3's "ensure arith.extsi is present to handle signedness correctly".
    // Unlike ArmNeon's I8MM path, no swapOperands bookkeeping is needed:
    // all four signed/unsigned combinations map directly onto a real
    // instruction, in the operand order they were already in.
    bool lhsSigned;
    auto maybeLhs = getExtOperand<arith::ExtSIOp>(op.getLhs());
    if (maybeLhs) {
      lhsSigned = true;
    } else {
      maybeLhs = getExtOperand<arith::ExtUIOp>(op.getLhs());
      lhsSigned = false;
    }
    if (!maybeLhs)
      return rewriter.notifyMatchFailure(
          op, "LHS is not a sign- or zero- extended iN, N <= 8");

    bool rhsSigned;
    auto maybeRhs = getExtOperand<arith::ExtSIOp>(op.getRhs());
    if (maybeRhs) {
      rhsSigned = true;
    } else {
      maybeRhs = getExtOperand<arith::ExtUIOp>(op.getRhs());
      rhsSigned = false;
    }
    if (!maybeRhs)
      return rewriter.notifyMatchFailure(
          op, "RHS is not a sign- or zero- extended iN, N <= 8");

    // Select the concrete hardware op directly from the two signedness
    // flags -- a straight truth table, no swap branch:
    //   lhs signed,   rhs signed   -> vmadot
    //   lhs unsigned, rhs unsigned -> vmadotu
    //   lhs signed,   rhs unsigned -> vmadotsu
    //   lhs unsigned, rhs signed   -> vmadotus
    if (lhsSigned && rhsSigned)
      imeOp = IMEOp::Vmadot;
    else if (!lhsSigned && !rhsSigned)
      imeOp = IMEOp::Vmadotu;
    else if (lhsSigned && !rhsSigned)
      imeOp = IMEOp::Vmadotsu;
    else // (!lhsSigned && rhsSigned)
      imeOp = IMEOp::Vmadotus;

    lhs = *maybeLhs;
    rhs = *maybeRhs;
    acc = op.getAcc();

    // Confirm the accumulator itself is i32 -- the explicit "check for i8
    // to i32 accumulation" from Task 2.2. getExtOperand already checked
    // this on the *source* side (the extend's output type); this checks
    // the destination/accumulator side directly.
    auto accType = cast<VectorType>(acc.getType());
    if (!accType.getElementType().isSignlessInteger(32))
      return rewriter.notifyMatchFailure(op, "Accumulator is not i32");

    // Extend inputs narrower than i8 (e.g. i4, i1) up to i8: vmadot and its
    // variants require exactly i8 operands.
    Location loc = op.getLoc();
    auto lhsExtInType = cast<VectorType>(lhs.getType());
    if (lhsExtInType.getElementTypeBitWidth() < 8)
      lhs = extendSmallIntVector(loc, lhsExtInType, lhs, lhsSigned, rewriter);

    auto rhsExtInType = cast<VectorType>(rhs.getType());
    if (rhsExtInType.getElementTypeBitWidth() < 8)
      rhs = extendSmallIntVector(loc, rhsExtInType, rhs, rhsSigned, rewriter);

    return success();
  }
};

/// Lowering from a vector.contract to a riscv_ime.vmadot-family intrinsic.
/// Currently handles a contraction shaped to exactly one hardware call
/// (see the file-level TODO for generalizing to tiled/unrolled multi-call
/// contractions).
class LowerContractionToRISCVIMEVMADOTPattern
    : public OpRewritePattern<vector::ContractionOp> {
public:
  using OpRewritePattern::OpRewritePattern;
  LogicalResult matchAndRewrite(vector::ContractionOp op,
                                 PatternRewriter &rewriter) const override {
    if (cast<vector::MaskableOpInterface>(op.getOperation()).isMasked())
      return rewriter.notifyMatchFailure(
          op, "masked contractions are not supported");

    llvm::errs() << "DEBUG: matchAndRewrite called on op\n";
    VectorContractRewriterVMADOT vcr;
    if (failed(vcr.matchAndInit(op, rewriter))) {
      llvm::errs() << "DEBUG: matchAndInit FAILED\n";
      return failure();
    }
    llvm::errs() << "DEBUG: matchAndInit SUCCEEDED, lowering now\n";
    vcr.lower(op, rewriter);

    return success();
  }
};

} // namespace

void mlir::riscv_ime::populateLowerContractionToRISCVIMEVMADOTPatterns(
    RewritePatternSet &patterns) {
  MLIRContext *context = patterns.getContext();
  patterns.add<LowerContractionToRISCVIMEVMADOTPattern>(context,
                                                          /*benefit=*/2);
}
