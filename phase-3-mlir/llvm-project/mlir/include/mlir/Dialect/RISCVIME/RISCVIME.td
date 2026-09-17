//===- RISCVIME.td - RISCVIME dialect op definitions -*- tablegen -*-===//
#ifndef RISCVIME_OPS
#define RISCVIME_OPS
include "mlir/Dialect/RISCVIME/RISCVIMEDialect.td"
include "mlir/Dialect/LLVMIR/LLVMOpBase.td"
include "mlir/Interfaces/SideEffectInterfaces.td"

class RISCVIME_IntrOp<string mnemonic, list<int> overloadedResults,
                      list<int> overloadedOperands, int numResults,
                      list<Trait> traits = [],
                      list<int> immArgPositions = [],
                      list<string> immArgAttrNames = []>
    : LLVM_IntrOpBase</*dialect=*/RISCVIME_Dialect, /*opName=*/mnemonic,
                      /*enumName=*/"riscv_smt_" # !subst(".", "_", mnemonic),
                      overloadedResults, overloadedOperands, traits, numResults,
                      /*requiresAccessGroup=*/0, /*requiresAliasAnalysis=*/0,
                      /*requiresFastmath=*/0, /*requiresArgAndResultAttrs=*/0,
                      /*requiresOpBundles=*/0, immArgPositions, immArgAttrNames>;

// Sign combinations confirmed from IME spec Table 3:
// vmadot=S*S, vmadotu=U*U, vmadotsu=S*U, vmadotus=U*S (vs1*vs2)
// vl/policy added after discovering the real LLVM intrinsic
// (RISCVTernaryWideUnMasked) takes 5 args across 4 overload groups, not 3.

def VmadotOp : RISCVIME_IntrOp<"vmadot",
                 /*overloadedResults=*/[0],
                 /*overloadedOperands=*/[1, 2, 3],
                 /*numResults=*/1,
                 /*traits=*/[Pure, AllTypesMatch<["acc", "res"]>],
                 /*immArgPositions=*/[4],
                 /*immArgAttrNames=*/["policy"]> {
  let summary = "SpacemiT IME signed x signed 8-bit dot-product matrix MAC";
  let arguments = (ins
    VectorOfRankAndType<[1], [I32]>:$acc,
    VectorOfRankAndType<[1], [I8]>:$vs1,
    VectorOfRankAndType<[1], [I8]>:$vs2,
    I64:$vl,
    I64Attr:$policy
  );
  let results = (outs VectorOfRankAndType<[1], [I32]>:$res);
  let assemblyFormat =
    "$acc `,` $vs1 `,` $vs2 `,` $vl `,` $policy attr-dict `:` type($vs1) `,` type($vs2) `to` type($res)";
}

def VmadotUOp : RISCVIME_IntrOp<"vmadotu",
                 /*overloadedResults=*/[0],
                 /*overloadedOperands=*/[1, 2, 3],
                 /*numResults=*/1,
                 /*traits=*/[Pure, AllTypesMatch<["acc", "res"]>],
                 /*immArgPositions=*/[4],
                 /*immArgAttrNames=*/["policy"]> {
  let summary = "SpacemiT IME unsigned x unsigned 8-bit dot-product matrix MAC";
  let arguments = (ins
    VectorOfRankAndType<[1], [I32]>:$acc,
    VectorOfRankAndType<[1], [I8]>:$vs1,
    VectorOfRankAndType<[1], [I8]>:$vs2,
    I64:$vl,
    I64Attr:$policy
  );
  let results = (outs VectorOfRankAndType<[1], [I32]>:$res);
  let assemblyFormat =
    "$acc `,` $vs1 `,` $vs2 `,` $vl `,` $policy attr-dict `:` type($vs1) `,` type($vs2) `to` type($res)";
}

def VmadotSUOp : RISCVIME_IntrOp<"vmadotsu",
                 /*overloadedResults=*/[0],
                 /*overloadedOperands=*/[1, 2, 3],
                 /*numResults=*/1,
                 /*traits=*/[Pure, AllTypesMatch<["acc", "res"]>],
                 /*immArgPositions=*/[4],
                 /*immArgAttrNames=*/["policy"]> {
  let summary = "SpacemiT IME signed x unsigned 8-bit dot-product matrix MAC";
  let arguments = (ins
    VectorOfRankAndType<[1], [I32]>:$acc,
    VectorOfRankAndType<[1], [I8]>:$vs1,
    VectorOfRankAndType<[1], [I8]>:$vs2,
    I64:$vl,
    I64Attr:$policy
  );
  let results = (outs VectorOfRankAndType<[1], [I32]>:$res);
  let assemblyFormat =
    "$acc `,` $vs1 `,` $vs2 `,` $vl `,` $policy attr-dict `:` type($vs1) `,` type($vs2) `to` type($res)";
}

def VmadotUSOp : RISCVIME_IntrOp<"vmadotus",
                 /*overloadedResults=*/[0],
                 /*overloadedOperands=*/[1, 2, 3],
                 /*numResults=*/1,
                 /*traits=*/[Pure, AllTypesMatch<["acc", "res"]>],
                 /*immArgPositions=*/[4],
                 /*immArgAttrNames=*/["policy"]> {
  let summary = "SpacemiT IME unsigned x signed 8-bit dot-product matrix MAC";
  let arguments = (ins
    VectorOfRankAndType<[1], [I32]>:$acc,
    VectorOfRankAndType<[1], [I8]>:$vs1,
    VectorOfRankAndType<[1], [I8]>:$vs2,
    I64:$vl,
    I64Attr:$policy
  );
  let results = (outs VectorOfRankAndType<[1], [I32]>:$res);
  let assemblyFormat =
    "$acc `,` $vs1 `,` $vs2 `,` $vl `,` $policy attr-dict `:` type($vs1) `,` type($vs2) `to` type($res)";
}

#endif // RISCVIME_OPS
