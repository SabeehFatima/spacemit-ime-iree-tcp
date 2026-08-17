# SpacemiT IME Integration with LLVM and IREE

## Training Completion Project — Compilers

This repository documents my Training Completion Project (TCP), focused on
enabling end-to-end support for the **SpacemiT Integer Matrix Extension (IME)**
in the LLVM and IREE compiler stack for RISC-V.

The SpacemiT IME is a vendor-specific RISC-V extension designed to accelerate
matrix operations for AI/ML workloads. It integrates with the RISC-V Vector
(RVV) extension and uses vector registers for matrix operations.

The project is divided into three phases, progressing from low-level LLVM
support to IREE compiler integration.

---

## Project Objective

The primary objective is to enable the compiler stack to recognize and
generate SpacemiT IME matrix operations, ultimately allowing matrix
multiplication workloads to utilize the `vmadot` instruction family.

The project covers the path from:

```text
High-level matrix operation
        ↓
IREE / MLIR
        ↓
Custom code generation
        ↓
LLVM IR / intrinsic
        ↓
RISC-V backend
        ↓
SpacemiT IME instruction
        ↓
vmadot
