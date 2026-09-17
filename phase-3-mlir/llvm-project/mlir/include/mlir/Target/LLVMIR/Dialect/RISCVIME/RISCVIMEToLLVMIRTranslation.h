//===- RISCVIMEToLLVMIRTranslation.h - RISCVIME to LLVMIR ---*- C++ -*-===//
//
// This provides registration calls for RISCVIME dialect to LLVM IR
// translation. Structure confirmed directly against the real, currently
// compiling ArmNeonToLLVMIRTranslation.h.
//
//===----------------------------------------------------------------------===//
 
#ifndef MLIR_TARGET_LLVMIR_DIALECT_RISCVIME_RISCVIMETOLLVMIRTRANSLATION_H
#define MLIR_TARGET_LLVMIR_DIALECT_RISCVIME_RISCVIMETOLLVMIRTRANSLATION_H
 
namespace mlir {
 
class DialectRegistry;
class MLIRContext;
 
/// Register the RISCVIME dialect and the translation from it to the LLVM IR
/// in the given registry;
void registerRISCVIMEDialectTranslation(DialectRegistry &registry);
 
/// Register the RISCVIME dialect and the translation from it in the
/// registry associated with the given context.
void registerRISCVIMEDialectTranslation(MLIRContext &context);
 
} // namespace mlir
 
#endif // MLIR_TARGET_LLVMIR_DIALECT_RISCVIME_RISCVIMETOLLVMIRTRANSLATION_H
