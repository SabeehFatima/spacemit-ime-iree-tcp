//===- RISCVIMEDialect.td - RISCVIME dialect definition -*- tablegen -*-===//
//
// Dialect-only definition. Ops live in a separate RISCVIME.td, matching the
// ArmNeon.td / ArmNeonDialect.h split used as this dialect's structural
// reference.
//
//===----------------------------------------------------------------------===//
 
#ifndef RISCVIME_DIALECT
#define RISCVIME_DIALECT
 
include "mlir/Dialect/LLVMIR/LLVMOpBase.td"
include "mlir/IR/OpBase.td"
 
def RISCVIME_Dialect : Dialect {
  let name = "riscv_ime";
  let useStrictPropertiesInAssemblyFormat = 1;
  let summary = "Dialect for the SpacemiT Integer Matrix Extension (IME).";
  let description = [{
    Wraps the SpacemiT `smt.vmadot` family of vendor RISC-V dot-product
    instructions (Phase 1) as MLIR operations, so that `vector.contract`
    can be pattern-matched and rewritten onto them (Phase 3, Task 2)
    instead of being hand-called from a ukernel (Phase 2).
  }];
  let cppNamespace = "::mlir::riscv_ime";
 
  // No LLVMDialect dependency needed here for the same reason ArmNeon.td
  // states explicitly: this dialect's own canonicalizations do not
  // construct LLVM dialect entities. The LLVM IR bridge is handled
  // separately in Task 3 via LLVMTranslationDialectInterface, which does
  // not require a dialect-level dependency.
}
 
#endif // RISCVIME_DIALECT
