# SpacemiT IME Integration with LLVM and IREE

<p align="center">
  <img alt="LLVM" src="https://img.shields.io/badge/LLVM-262D3A?style=for-the-badge&logo=llvm&logoColor=white">
  <img alt="MLIR" src="https://img.shields.io/badge/MLIR-4B32C3?style=for-the-badge&logo=llvm&logoColor=white">
  <img alt="RISC-V" src="https://img.shields.io/badge/RISC--V-283272?style=for-the-badge&logo=riscv&logoColor=white">
  <img alt="C++" src="https://img.shields.io/badge/C%2B%2B-00599C?style=for-the-badge&logo=cplusplus&logoColor=white">
  <img alt="IREE" src="https://img.shields.io/badge/IREE-FF6F00?style=for-the-badge">
</p>

Teaching the LLVM/IREE compiler stack to recognize an ordinary matrix
multiply and automatically use **SpacemiT's Integer Matrix Extension (IME)**
— a RISC-V hardware instruction (`vmadot`) purpose-built for int8 matrix
multiply-accumulate — with no changes to the source code being compiled.

## The result

```text
  linalg.matmul  →  vector.contract  →  riscv_ime.vmadot  →  smt.vmadot
  (ordinary code)                       (this project)        (real hardware)
```

Compiled the exact same, unmodified matrix multiply two ways — with the
hardware extension enabled and without — and measured it on a real RISC-V
board:

| | With `vmadot` | Without `vmadot` |
|---|---|---|
| Time per call (64×64 int8 matmul) | ~0.252 ms | ~0.510 ms |

**~2× faster, automatically, with zero code changes.**

## Why this is three phases

| Phase | What it does |
|---|---|
| [Phase 1 — LLVM](phase-1-llvm/) | Teaches the compiler the instruction exists at all — builtin, intrinsic, RISC-V backend support |
| [Phase 2 — IREE ukernel](phase-2-iree/) | Proves the instruction actually works, by hand-writing one call to it |
| [Phase 3 — MLIR/IREE](phase-3-mlir/) | Teaches the compiler to *find* opportunities to use it on its own, in ordinary code — this is what produced the benchmark above |

Each folder contains only the files that were added or changed, with a
README covering what was built, what broke, and how it was fixed.

## Hardware

SpacemiT X60 (RISC-V, RVV), tested on a Banana Pi BPI-F3.
