//===- RISCVIMEToLLVMIRTranslation.cpp - RISCVIME to LLVM IR ------------===//
//
// This file implements a translation between the MLIR RISCVIME dialect and
// LLVM IR.
//
//===----------------------------------------------------------------------===//
 
#include "mlir/Target/LLVMIR/Dialect/RISCVIME/RISCVIMEToLLVMIRTranslation.h"
#include "mlir/Dialect/RISCVIME/RISCVIMEDialect.h"
#include "mlir/IR/Operation.h"
#include "mlir/Target/LLVMIR/ModuleTranslation.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/IntrinsicsRISCV.h"
 
using namespace mlir;
using namespace mlir::LLVM;
 
namespace {
/// Implementation of the dialect interface that converts operations
/// belonging to the RISCVIME dialect to LLVM IR.
class RISCVIMEDialectLLVMIRTranslationInterface
    : public LLVMTranslationDialectInterface {
public:
  using LLVMTranslationDialectInterface::LLVMTranslationDialectInterface;
 
  /// Translates the given operation to LLVM IR using the provided IR
  /// builder and saving the state in `moduleTranslation`.
  LogicalResult
  convertOperation(Operation *op, llvm::IRBuilderBase &builder,
                   LLVM::ModuleTranslation &moduleTranslation) const final {
    Operation &opInst = *op;
#include "mlir/Dialect/RISCVIME/RISCVIMEConversions.inc"
    return failure();
  }
};
} // namespace
 
void mlir::registerRISCVIMEDialectTranslation(DialectRegistry &registry) {
  registry.insert<riscv_ime::RISCVIMEDialect>();
  registry.addExtension(
      +[](MLIRContext *ctx, riscv_ime::RISCVIMEDialect *dialect) {
        dialect->addInterfaces<RISCVIMEDialectLLVMIRTranslationInterface>();
      });
}
 
void mlir::registerRISCVIMEDialectTranslation(MLIRContext &context) {
  DialectRegistry registry;
  registerRISCVIMEDialectTranslation(registry);
  context.appendDialectRegistry(registry);
}
