# Phase 2 — IREE MMT4D Ukernel Integration

## Objective

Phase 2 hand-writes an IREE ukernel that explicitly calls the Phase 1
SpacemiT VMADOT builtin, proving the Phase 1 compiler support is real,
callable, and produces correct `smt.vmadot` instructions in compiled
IREE output.

This is a manual integration step: the ukernel function is called
directly by IREE's `linalg.mmt4d` dispatch path, not discovered
automatically by the compiler. Automatic discovery is Phase 3's goal.

---

## Files Added / Modified

    runtime/src/iree/builtins/ukernel/arch/riscv_64/mmt4d_riscv_64_xsmtvdot.c

This single file implements the MMT4D microkernel tile function using
the Phase 1 `smt_vmadot` family of builtins directly.

---

## Validation

- Hand-disassembled the compiled `.vmfb` artifact and confirmed the
  correct count of `smt.vmadot` instructions in the emitted RISC-V
  assembly.
- Ran on real hardware (BPI-F3 board, SpacemiT X60 SoC) via
  `iree-run-module`, confirming correct numerical output.
- Confirmed IME is only available on cores 0-3 of the heterogeneous
  BPI-F3 SoC (`taskset -c 0` required for correct execution).

## Known Limitation

Model-level (ResNet-18) benchmarking showed no measurable speedup at
the full-model level. Root cause: convolutions in the test model lower
to `linalg.generic`, not `linalg.mmt4d`, and therefore never invoke
this ukernel at all — a pre-existing IREE data-tiling limitation
(tracked upstream as IREE issue #18513), not a defect in this ukernel.
This finding directly motivated Phase 3's approach: teaching the
compiler to recognize `vector.contract` generically, rather than
depending on code happening to already be in `linalg.mmt4d` form.

---

## Status

**Phase 2 — Completed**

The SpacemiT VMADOT builtins are proven correct and usable from a real
IREE ukernel, with verified `smt.vmadot` output on real RISC-V
hardware.
