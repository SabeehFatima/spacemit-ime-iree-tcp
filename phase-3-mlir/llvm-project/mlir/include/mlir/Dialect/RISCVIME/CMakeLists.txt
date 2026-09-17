set(LLVM_TARGET_DEFINITIONS RISCVIME.td)
mlir_tablegen(RISCVIMEConversions.inc -gen-llvmir-conversions)
add_mlir_dialect_tablegen_target(MLIRRISCVIMEConversionsIncGen)
add_mlir_dialect(RISCVIME riscv_ime)
add_mlir_doc(RISCVIMEDialect RISCVIMEDialect Dialects/ -gen-dialect-doc)

