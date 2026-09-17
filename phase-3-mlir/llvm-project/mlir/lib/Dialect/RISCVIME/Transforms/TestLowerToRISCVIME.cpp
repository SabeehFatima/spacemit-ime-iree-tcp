//===- TestLowerToRISCVIME.cpp - Test pass for riscv_ime lowering -------===//
//
// Minimal test pass: runs LowerContractionToRISCVIMEVMADOTPattern via the
// greedy pattern rewrite driver, so the Task 2 pattern can be exercised
// directly from mlir-opt without the full Transform-dialect extension.
//
//===----------------------------------------------------------------------===//

#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/RISCVIME/RISCVIMEDialect.h"
#include "mlir/Dialect/RISCVIME/Transforms.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Transforms/GreedyPatternRewriteDriver.h"

using namespace mlir;

namespace {
struct TestLowerToRISCVIMEPass
    : public PassWrapper<TestLowerToRISCVIMEPass,
                          OperationPass<func::FuncOp>> {
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(TestLowerToRISCVIMEPass)

  StringRef getArgument() const final { return "test-lower-to-riscv-ime"; }
  StringRef getDescription() const final {
    return "Test pass: lower vector.contract to riscv_ime.vmadot-family ops";
  }

  // This pass creates riscv_ime ops even when the input IR contains none
  // (e.g. our test input has only vector.contract/arith ops). Without this
  // override, the riscv_ime dialect is never loaded into the MLIRContext
  // and op creation fails with "isn't known in this MLIRContext".
  void getDependentDialects(DialectRegistry &registry) const override {
    registry.insert<riscv_ime::RISCVIMEDialect>();
  }

  void runOnOperation() override {
    RewritePatternSet patterns(&getContext());
    riscv_ime::populateLowerContractionToRISCVIMEVMADOTPatterns(patterns);
    FrozenRewritePatternSet frozenPatterns(std::move(patterns));
    (void)applyPatternsGreedily(getOperation(), frozenPatterns);
  }
};
} // namespace

namespace mlir {
namespace riscv_ime {
void registerTestLowerToRISCVIMEPass() {
  PassRegistration<TestLowerToRISCVIMEPass>();
}
} // namespace riscv_ime
} // namespace mlir
