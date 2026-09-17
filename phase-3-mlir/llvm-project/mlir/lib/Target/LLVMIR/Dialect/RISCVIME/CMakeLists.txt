add_mlir_translation_library(MLIRRISCVIMEToLLVMIRTranslation
  RISCVIMEToLLVMIRTranslation.cpp
  DEPENDS
  MLIRRISCVIMEConversionsIncGen
  LINK_COMPONENTS
  Core
  LINK_LIBS PUBLIC
  MLIRRISCVIMEDialect
  MLIRIR
  MLIRLLVMDialect
  MLIRSupport
  MLIRTargetLLVMIRExport
  )
