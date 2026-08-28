# Phase 1 — Enabling SpacemiT IME in LLVM

## Objective

The goal of Phase 1 is to enable support for the SpacemiT Integer Matrix Extension (IME) integer dot-product matrix multiply-accumulate instructions in the LLVM compiler infrastructure.

The implementation establishes the Clang builtin, LLVM IR intrinsic, and RISC-V backend support required for later integration with IREE.

---

## Supported Instructions

| Instruction | LHS Operand | RHS Operand | Accumulator |
|-------------|-------------|-------------|-------------|
| `vmadot`    | `int8`      | `int8`      | `int32`     |
| `vmadotu`   | `uint8`     | `uint8`     | `int32`     |
| `vmadotus`  | `uint8`     | `int8`      | `int32`     |
| `vmadotsu`  | `int8`      | `uint8`     | `int32`     |

---

## Compiler Support Implemented

| Component | Support Added |
|-----------|---------------|
| Clang Builtins | SpacemiT VMADOT builtin definitions |
| Clang Sema | Builtin lookup and semantic handling |
| Clang CodeGen | Builtin-to-LLVM intrinsic lowering |
| LLVM IR | XSMTVDot intrinsics for VMADOT variants |
| RISC-V Backend | SpacemiT VMADOT instruction definitions and lowering |
| TableGen | SpacemiT-specific builtin generation support |
| Testing | Clang CodeGen and LLVM RISC-V backend tests |

---

## LLVM Intrinsics

The following LLVM intrinsics were added:

    smt_vmadot
    smt_vmadotu
    smt_vmadotus
    smt_vmadotsu

---

## Files Added / Modified

### Clang

    clang/include/clang/Basic/CMakeLists.txt
    clang/include/clang/Basic/TargetBuiltins.h
    clang/include/clang/Basic/riscv_spacemit_vector.td
    clang/include/clang/Sema/RISCVIntrinsicManager.h
    clang/include/clang/Sema/SemaRISCV.h

    clang/lib/Basic/Builtins.cpp
    clang/lib/Basic/Targets/RISCV.cpp
    clang/lib/CodeGen/TargetBuiltins/RISCV.cpp
    clang/lib/Parse/ParsePragma.cpp
    clang/lib/Sema/SemaLookup.cpp
    clang/lib/Sema/SemaRISCV.cpp
    clang/utils/TableGen/TableGen.cpp

### LLVM

    llvm/include/llvm/IR/IntrinsicsRISCV.td
    llvm/include/llvm/IR/IntrinsicsRISCVXSMTVDot.td
    llvm/lib/Target/RISCV/RISCVInstrInfoXSpacemiT.td

### Tests

    clang/test/CodeGen/RISCV/spacemit-intrinsics/non-policy/overloaded/smt_vmadot.c
    clang/test/CodeGen/RISCV/spacemit-intrinsics/non-policy/overloaded/smt_vmadotu.c
    clang/test/CodeGen/RISCV/spacemit-intrinsics/non-policy/overloaded/smt_vmadotus.c
    clang/test/CodeGen/RISCV/spacemit-intrinsics/non-policy/overloaded/smt_vmadotsu.c

    llvm/test/CodeGen/RISCV/rvv/xsmtvdot-vmadot.ll
    llvm/test/CodeGen/RISCV/rvv/xsmtvdot-vmadotu.ll
    llvm/test/CodeGen/RISCV/rvv/xsmtvdot-vmadotus.ll
    llvm/test/CodeGen/RISCV/rvv/xsmtvdot-vmadotsu.ll

---

## Issues Resolved

| Issue | Resolution |
|-------|------------|
| SpacemiT TableGen flags were not recognized | Added the required SpacemiT TableGen actions |
| LLVM intrinsic type-set error | Corrected the VMADOT intrinsic and backend type definitions |
| VMADOT instruction naming | Updated instruction definitions with the correct VMADOT naming |
| Clang builtin integration | Added builtin definitions, semantic handling, and CodeGen support |

---

## Validation

Phase 1 was validated using:

- Clang CodeGen tests for all four VMADOT variants
- LLVM RISC-V backend CodeGen tests
- LLVM TableGen generation
- Hand-written C intrinsic smoke testing

---

## Status

**Phase 1 — Completed**

The four SpacemiT IME VMADOT variants are supported across the required Clang and LLVM compiler layers, providing the compiler-side foundation for Phase 2 IREE integration.
