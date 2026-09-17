#ifndef MLIR_DIALECT_RISCVIME_TRANSFORMS_H
#define MLIR_DIALECT_RISCVIME_TRANSFORMS_H

#include "mlir/IR/PatternMatch.h"

namespace mlir {
namespace riscv_ime {

void populateLowerContractionToRISCVIMEVMADOTPatterns(RewritePatternSet &patterns);
void registerTestLowerToRISCVIMEPass();

} // namespace riscv_ime
} // namespace mlir

#endif // MLIR_DIALECT_RISCVIME_TRANSFORMS_H
