# Phase 3 — Automatic vector.contract → vmadot Lowering in MLIR/IREE

## Objective

Teach the IREE/MLIR compiler stack to *automatically* recognize a
generic `vector.contract` (matrix multiplication) operation and lower
it directly into a call to the Phase 1 `smt.vmadot` intrinsic family —
with no hand-written ukernel call site, no builtin call, and no source
code changes required from the programmer.

This closes the gap identified at the end of Phase 2: code that lowers
through `linalg.generic` instead of `linalg.mmt4d` never reached the
Phase 2 ukernel. Phase 3 removes that dependency entirely by matching
on `vector.contract` shape directly, wherever it appears.

Structural reference throughout: ArmNeon's own `vector.contract` →
Neon-intrinsic lowering
(`mlir/lib/Dialect/ArmNeon/Transforms/LowerContractToNeonPatterns.cpp`),
used as a pattern to study and adapt, not copy — SpacemiT IME differs
from ArmNeon's I8MM path in two deliberate ways: all four
signed/unsigned combinations map to real hardware instructions (no
`swapOperands` emulation needed), and RVV vectors are scalable by
construction (ArmNeon's fixed-width scalable-vector rejection does not
apply here).

---

## Task Breakdown

| Task | Goal |
|------|------|
| Task 1 | Define the `RISCVIME` MLIR dialect and its four `vmadot`-family ops |
| Task 2 | Write the `vector.contract` → `riscv_ime.vmadot` lowering pattern |
| Task 3 | Wire LLVM IR translation (`riscv_ime.vmadot` → `llvm.riscv.smt.vmadot`) |
| Task 4 | Wire the pattern into IREE's real production compiler pipeline |

---

## Supported Operations

| Op | Sign combination (vs1 × vs2) | Confirmed via SpacemiT IME spec |
|----|-------------------------------|----------------------------------|
| `riscv_ime.vmadot`   | signed × signed     | Table 3, §3.1.1 |
| `riscv_ime.vmadotu`  | unsigned × unsigned | Table 3, §3.1.1 |
| `riscv_ime.vmadotsu` | signed × unsigned   | Table 3, §3.1.1 |
| `riscv_ime.vmadotus` | unsigned × signed   | Table 3, §3.1.1 |

---

## Files Added / Modified

### Task 1 — Dialect and Ops (`llvm-project/mlir`)

    mlir/include/mlir/Dialect/RISCVIME/RISCVIMEDialect.td
    mlir/include/mlir/Dialect/RISCVIME/RISCVIME.td
    mlir/include/mlir/Dialect/RISCVIME/RISCVIMEDialect.h
    mlir/include/mlir/Dialect/RISCVIME/CMakeLists.txt
    mlir/lib/Dialect/RISCVIME/IR/RISCVIMEDialect.cpp
    mlir/lib/Dialect/RISCVIME/IR/CMakeLists.txt
    mlir/lib/Dialect/RISCVIME/CMakeLists.txt
    mlir/lib/RegisterAllDialects.cpp (2-line edit: dialect registration)
    mlir/lib/Dialect_CMakeLists.txt (real path: mlir/lib/Dialect/CMakeLists.txt — 1-line edit)

### Task 2 — Lowering Pattern

    mlir/include/mlir/Dialect/RISCVIME/Transforms.h
    mlir/lib/Dialect/RISCVIME/Transforms/LowerContractionToRISCVIMEVMADOTPattern.cpp
    mlir/lib/Dialect/RISCVIME/Transforms/TestLowerToRISCVIME.cpp
    mlir/lib/Dialect/RISCVIME/Transforms/CMakeLists.txt
    mlir/tools/mlir-opt/mlir-opt.cpp (2-line edit: test pass registration)

### Task 3 — LLVM IR Translation

    mlir/include/mlir/Target/LLVMIR/Dialect/RISCVIME/RISCVIMEToLLVMIRTranslation.h
    mlir/lib/Target/LLVMIR/Dialect/RISCVIME/RISCVIMEToLLVMIRTranslation.cpp
    mlir/lib/Target/LLVMIR/Dialect/RISCVIME/CMakeLists.txt
    mlir/lib/Target/LLVMIR/CMakeLists.txt (1-line edit: link ordering fix)
    mlir/lib/Target/LLVMIR/Dialect_CMakeLists.txt (real path: mlir/lib/Target/LLVMIR/Dialect/CMakeLists.txt — 1-line edit)

### Task 4 — IREE Pipeline Integration (`iree`)

    compiler/src/iree/compiler/Codegen/LLVMCPU/Utils.h
    compiler/src/iree/compiler/Codegen/LLVMCPU/Utils.cpp
    compiler/src/iree/compiler/Codegen/LLVMCPU/LLVMCPUVirtualVectorLowering.cpp
    compiler/src/iree/compiler/Codegen/LLVMCPU/BUILD.bazel (2-line edit: dialect deps)
    compiler/plugins/target/LLVMCPU/LLVMCPUTarget.cpp
    compiler/plugins/target/LLVMCPU/BUILD.bazel (2-line edit: dialect deps)
    compiler/src/iree/compiler/Tools/init_mlir_dialects.h
    compiler/src/iree/compiler/Tools/init_llvmir_translations.h
    compiler/src/iree/compiler/Tools/CMakeLists.txt

---

## Issues Resolved

| # | Issue | Resolution |
|---|-------|------------|
| 1 | `assemblyFormat` only declared one operand's type (`type($vs1)`), causing "type of operand #2 is not buildable" TableGen error | Added `type($vs2)` to the format string for all four ops |
| 2 | Missing `useStrictPropertiesInAssemblyFormat = 1` on the dialect record caused cascading "no member `getProperties`" compile errors | Added the flag, matching ArmNeon's real dialect definition |
| 3 | `RISCVIMEDialect.h` never included `mlir/IR/BuiltinTypes.h`, causing "'VectorType' is not a member of 'mlir'" | Added the missing include |
| 4 | `RISCVIMEDialect.cpp` never included `mlir/Dialect/Vector/IR/VectorOps.h`, causing incomplete-type errors for `OpBuilder`/`Builder` | Added the missing include |
| 5 | Base `matchAndInit` hardcoded RHS as `(N, K)` (dim0=N, dim1=K) rather than deriving layout from the real indexing maps, silently rejecting valid `(K, N)`-ordered contractions | Rewrote to derive `dimM`/`dimN`/`dimK` from the operands' actual `AffineMap`s |
| 6 | `VmadotOp` only modeled 3 operands (`acc`, `vs1`, `vs2`); the real `llvm.riscv.smt.vmadot` intrinsic (`RISCVTernaryWideUnMasked`) takes 5 arguments across 4 overload groups (`acc`, `vs1`, `vs2`, `vl`, `policy`), causing an LLVM assertion crash during intrinsic type resolution | Extended `RISCVIME_IntrOp` to forward `immArgPositions`/`immArgAttrNames`; added `$vl`/`$policy` to all four ops; updated `overloadedOperands` to `[1, 2, 3]` |
| 7 | Adding the `$policy` attribute made `VmadotOp` the first op in the dialect with an attribute, triggering a bytecode-interface code path that needed two previously-unused includes | Added `mlir/Bytecode/BytecodeOpInterface.h` and `mlir/Bytecode/BytecodeImplementation.h` |
| 8 | `flattenTo1D` passed the *original* (multi-element) `scalableDims` array into a `VectorType::get` call for a *new*, differently-ranked 1D shape, causing an assertion crash ("number of dims must match") | Computed a single correctly-sized scalable flag (`llvm::any_of` over the original dims) for the new 1D shape |
| 9 | TableGen generation for `RISCVIMEConversions.inc` was placed in the `lib/Target/LLVMIR/Dialect/RISCVIME/CMakeLists.txt` translation-library file rather than the dialect's own `include/` CMakeLists.txt, causing an incorrect relative path and "file not found" | Moved the `mlir_tablegen(...)`/`set(LLVM_TARGET_DEFINITIONS ...)` lines into the dialect's own `include/mlir/Dialect/RISCVIME/CMakeLists.txt`, matching ArmNeon's real structure exactly |
| 10 | `mlir-translate --mlir-to-llvmir` cannot translate the `func` dialect directly (only `llvm` dialect and dialects with a registered translation interface); smoke tests written with `func.func` failed with "missing LLVMTranslationDialectInterface registration for dialect: func.func" | Rewrote smoke tests using `llvm.func`/`llvm.return` directly, matching ArmNeon's own real lit test convention |
| 11 | `MLIRRISCVIMEToLLVMIRTranslation` was never added to `MLIRToLLVMIRTranslationRegistration`'s `LINK_LIBS`, causing "undefined reference to `registerRISCVIMEDialectTranslation`" at final link time | Added the library to the `LINK_LIBS PUBLIC` list in `mlir/lib/Target/LLVMIR/CMakeLists.txt` |
| 12 | `LLVMCPUVirtualVectorLowering.cpp` was missing includes for `RISCVIMEDialect.h`/`Transforms.h`, causing "undeclared" errors when calling into the pattern | Added both includes |
| 13 | Pass-construction call used the wrong struct type (`LLVMCPUVectorLoweringPassOptions` instead of `LLVMCPUVirtualVectorLoweringPassOptions`) with designated initializers, causing a "no member named `enableXsmtvdot`" compile error | Corrected the struct type name |
| 14 | `enableXsmtvdot` was referenced in `LLVMCPUVirtualVectorLowering.cpp` before being declared as a formal pass `Option` in `Passes.td`, and before being threaded through `Passes.h`'s two pipeline-options structs and all 6 pipeline-construction call sites in `Passes.cpp` | Added the `Option` declaration, the two struct fields, and the 6 copy-sites |
| 15 | **Root cause of the pattern never firing in the real pipeline:** `LLVMCPUVirtualVectorLoweringPass` is invoked *twice* per function across different pipeline stages (confirmed via direct debug instrumentation); the `enableXsmtvdot` boolean, computed once at pass-construction time and threaded through the options-struct chain above, was only correctly set on one of the two invocations — and the invocation with the flag correctly set was never the one where `vector.contract` was still present in the IR | Replaced the threaded-boolean check with a direct `targetConfig && isRISCV(targetConfig) && hasXsmtvdotFeature(targetConfig)` check, re-evaluated fresh on every invocation of `runOnOperation()` — eliminating the dependency on which of several pipeline paths correctly threaded a precomputed value |

---

## Verification

**Task 1** — `mlir-opt` round-trip (parse → print, byte-identical) confirmed for all four ops.

**Task 2** — `mlir-opt --test-lower-to-riscv-ime` confirmed correct match/rewrite for all four sign combinations on hand-written `vector.contract` inputs, each correctly dispatching to its corresponding op.

**Task 3** — `mlir-translate --mlir-to-llvmir` confirmed correct `llvm.riscv.smt.vmadot*` intrinsic calls (correct mangled type suffix, correct `immarg` marking on `policy`) for all four ops.

**Task 4** — Full pipeline verification: an ordinary `linalg.matmul`, with no `riscv_ime`/`vmadot`-specific code anywhere in the source, compiled via `iree-compile --iree-llvmcpu-target-cpu-features="+zvl256b,+v,+xsmtvdot"`, produces real `smt.vmadot` instructions in the final RISC-V `.s` assembly file. Confirmed on real hardware (BPI-F3 board, SpacemiT X60).

**Benchmarking** — 64×64 int8 matrix multiply, run via `iree-benchmark-module` on the BPI-F3 board, comparing identical `iree-compile` invocations with and without `+xsmtvdot`:

| | With `vmadot` | Without `vmadot` |
|---|---|---|
| Mean time/call | ~0.252 ms | ~0.510 ms |
| Speedup | **~2.0×** | — |

Confirmed consistent across 3 independent runs each (std. deviation <3% of mean). At smaller sizes (4×8 × 8×4), the difference is negligible — the matmul is too small for the actual compute time to exceed fixed per-call overhead.

---

## Known Remaining Gaps

- The `(4, 4, 8)` tile shape and `vscale=4` assumption in `createIMEOp` are hardcoded for `VLEN=256` only; generalizing to other VLEN values is separate follow-up work.
- `policy=0` is a working placeholder, confirmed to compile and produce correct results for the tested (non-partial-tile) cases, but not yet confirmed against the real RVV tail/mask-policy encoding for edge cases.
- Only `vmadot` (signed×signed) has been exercised through the full real `iree-compile` → hardware pipeline, including benchmarking. `vmadotu`/`vmadotsu`/`vmadotus` are confirmed correct through Task 3's LLVM IR translation level, but not yet independently re-confirmed through the full Task 4 pipeline on real hardware. Note: the TCP brief's stated deliverable scope is `vmadot` only; the other three variants were implemented as an extension beyond minimum scope.
- A 256×256 matmul currently triggers a separate compilation failure unrelated to the `vmadot` lowering pattern itself (confirmed working correctly up to at least 64×64); root cause not yet isolated.
- No formal `mlir/test/...` lit regression test was written. This was not a stated requirement of the TCP brief, whose only testing requirement (Task 4.1–4.3) was direct compilation and inspection of the output assembly — satisfied by the verification above. A formal lit test remains worthwhile future work.

---

## Status

**Phase 3 — Completed**

All four stated deliverables are met: the `RISCVIME` dialect, the `vector.contract` → `riscv_ime.vmadot` transformation pattern with tiling, and proof of `smt.vmadot` in generated assembly — confirmed both via direct compilation and via measured ~2× performance improvement on real RISC-V hardware.
