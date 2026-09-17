add_mlir_dialect_library(MLIRRISCVIMETransforms
 LowerContractionToRISCVIMEVMADOTPattern.cpp
 TestLowerToRISCVIME.cpp
  ADDITIONAL_HEADER_DIRS
  ${MLIR_MAIN_INCLUDE_DIR}/mlir/Dialect/RISCVIME

  LINK_LIBS PUBLIC
  MLIRRISCVIMEDialect
  MLIRArithDialect
  MLIRVectorDialect
  MLIRIR
  MLIRPass
  )
