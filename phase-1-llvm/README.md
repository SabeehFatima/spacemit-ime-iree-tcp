# Phase 1 — Enabling SpacemiT IME in LLVM

## Objective

The goal of Phase 1 is to enable support for the SpacemiT Integer Matrix
Extension (IME) integer dot-product matrix multiply-accumulate instructions
in the LLVM compiler infrastructure.

The work establishes the low-level compiler support required for later
integration with IREE.

---

## Supported Instructions

Phase 1 targets the following SpacemiT IME instructions:

| Instruction | LHS Operand | RHS Operand | Accumulator |
|---|---|---|---|
| `vmadot` | `int8` | `int8` | `int32` |
| `vmadotu` | `uint8` | `uint8` | `int32` |
| `vmadotus` | `uint8` | `int8` | `int32` |
| `vmadotsu` | `int8` | `uint8` | `int32` |

---

## Required Compiler Support

The implementation will cover the following layers:

```text
C / C++ source
      ↓
Clang builtin
      ↓
LLVM IR intrinsic
      ↓
LLVM RISC-V backend
      ↓
Instruction selection / lowering
      ↓
MC / assembler
      ↓
SpacemiT IME instruction
