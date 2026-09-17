#ifndef MLIR_DIALECT_RISCVIME_RISCVIMEDIALECT_H
#define MLIR_DIALECT_RISCVIME_RISCVIMEDIALECT_H

#include "mlir/Bytecode/BytecodeOpInterface.h"
#include "mlir/Bytecode/BytecodeImplementation.h"
#include "mlir/IR/BuiltinTypes.h"
#include "mlir/IR/Dialect.h"
#include "mlir/IR/OpDefinition.h"
#include "mlir/Interfaces/SideEffectInterfaces.h"

#include "mlir/Dialect/RISCVIME/RISCVIMEDialect.h.inc"

#define GET_OP_CLASSES
#include "mlir/Dialect/RISCVIME/RISCVIME.h.inc"

#endif // MLIR_DIALECT_RISCVIME_RISCVIMEDIALECT_H
