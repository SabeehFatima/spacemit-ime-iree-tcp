//===----------------------------------------------------------------------===//
// This file defines LLVM intrinsics for SpacemiT's XSMTVDot extension.
//===----------------------------------------------------------------------===//
let TargetPrefix ="riscv" in {
  defm smt_vmadot : RISCVTernaryWide;
  defm smt_vmadotu : RISCVTernaryWide;
  defm smt_vmadotsu : RISCVTernaryWide;
  defm smt_vmadotus : RISCVTernaryWide;
}
