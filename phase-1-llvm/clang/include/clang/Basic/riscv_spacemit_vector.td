//===----------------------------------------------------------------------===//
// SpacemiT Vector Dot Product Extension (XSMTVDot)
//===----------------------------------------------------------------------===//


include "riscv_vector_common.td"

multiclass RVVSMTVDOT<list<list<string>> i_suffixes_prototypes> {
  let RequiredFeatures = ["xsmtvdot"],
      UnMaskedPolicyScheme = HasPolicyOperand,
      HasMaskedOffOperand = false,
      Log2LMUL = [-1, 0],
      OverloadedName = NAME in {

    defm NAME : RVVOutOp1Op2BuiltinSet<NAME, "i", i_suffixes_prototypes>;
  }
}

// LHS int8, RHS int8, accumulator int32 (signed x signed)
// FIX: return type + accumulator (vd) now forced to absolute Log2LMUL=1 (VRM2/LMUL2),
// independent of the loop's Log2LMUL ([-1,0] for the 8-bit inputs). Previously both
// were plain 'v', which inherited the loop's LMUL and produced nxv2i32 (LMUL1)
// instead of the nxv4i32 (LMUL2) the Pat/pseudo layer expects.
defm smt_vmadot : RVVSMTVDOT<[["vv", "v", "(LFixedLog2LMUL:1)v(LFixedLog2LMUL:1)v(FixedSEW:8)v(FixedSEW:8)v"]]>;

// LHS uint8, RHS uint8, accumulator int32 (unsigned x unsigned)
// FIX: same LMUL correction as above. vs1/vs2 U (unsigned) markers unchanged.
defm smt_vmadotu : RVVSMTVDOT<[["vv", "v", "(LFixedLog2LMUL:1)v(LFixedLog2LMUL:1)v(FixedSEW:8)Uv(FixedSEW:8)Uv"]]>;

// LHS int8, RHS uint8, accumulator int32 (signed x unsigned)
// FIX: same LMUL correction. Only vs2 is unsigned, per instruction semantics.
defm smt_vmadotsu : RVVSMTVDOT<[["vv", "v", "(LFixedLog2LMUL:1)v(LFixedLog2LMUL:1)v(FixedSEW:8)v(FixedSEW:8)Uv"]]>;

// LHS uint8, RHS int8, accumulator int32 (unsigned x signed)
// FIX: same LMUL correction. Only vs1 is unsigned, per instruction semantics.
defm smt_vmadotus : RVVSMTVDOT<[["vv", "v", "(LFixedLog2LMUL:1)v(LFixedLog2LMUL:1)v(FixedSEW:8)Uv(FixedSEW:8)v"]]>;
