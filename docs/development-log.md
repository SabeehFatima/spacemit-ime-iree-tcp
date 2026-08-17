# Development Log

This document records the progress, implementation decisions, problems,
experiments, and verification results throughout the Training Completion
Project.

The purpose is to maintain a chronological engineering record rather than
reconstructing the project retrospectively.

---

## 2026-08-17 — Project Documentation Initialized

### Project

**SpacemiT IME Integration with LLVM and IREE**

### Objective

Enable end-to-end compiler support for the SpacemiT Integer Matrix Extension
(IME), progressing from LLVM backend support to IREE microkernel integration
and finally IREE code generation.

### Project Phases

- Phase 1 — Enable SpacemiT IME extension in LLVM
- Phase 2 — Add an MMT4D microkernel using SpacemiT IME
- Phase 3 — Enable MMT4D code generation using SpacemiT IME in IREE

### Development Environment

Primary development is performed on an HPC environment.

The actual LLVM and IREE source trees and builds are maintained separately
from this documentation repository.

### GitHub Repository

Repository:

`SabeehFatima/spacemit-ime-iree-tcp`

The repository is used as the permanent project documentation and
reproducibility record.

### Phase 1 Starting Point

Phase 1 has not yet been implemented.

The first task is to investigate existing vendor-specific RISC-V vector
extensions in LLVM, particularly the XandesVDOT implementation suggested
by the TCP specification.

### Initial Investigation Goals

- Understand how RISC-V vendor extensions are represented in LLVM.
- Identify the Clang builtin definition mechanism.
- Identify the LLVM intrinsic definition mechanism.
- Understand the RISC-V backend instruction definition structure.
- Understand instruction encoding and MC support.
- Understand DAG pattern matching and instruction lowering.
- Identify the existing regression test structure.

### Current Status

Phase 1: **In Progress**

### Next Step

Begin investigation of the existing XandesVDOT implementation and identify
the corresponding LLVM files that can serve as structural references for
SpacemiT IME.
