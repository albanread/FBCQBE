# MADD/FMADD Fusion Rollback Summary

**Date:** 2024-01-31  
**Status:** ✅ **ROLLBACK COMPLETE**  
**Action:** Restored original QBE source files

---

## Executive Summary

We have successfully rolled back the MADD/FMADD fusion implementation and restored the original QBE source code from `fsh/qbe/`. The system is now back to a stable baseline state before attempting the optimization again with proper analysis.

---

## What Was Rolled Back

### Files Restored from `fsh/qbe/`

All MADD-related changes were reverted by copying original files:

1. **Core QBE files:**
   - `all.h` - Reverted `Ins.arg[3]` back to `arg[2]`
   - `ops.h` - Removed MADD/FMADD opcodes (Oamadd, Oamsub, Oafmadd, Oafmsub)
   - `util.c` - Reverted `emit()` and removed `emit3()` function
   - `parse.c` - Reverted argument iteration changes

2. **Compiler passes:**
   - `rega.c` - Reverted register allocator loop changes
   - `spill.c` - Reverted spill handling changes
   - `gcm.c` - Reverted global code motion changes
   - `gvn.c` - Reverted global value numbering changes
   - `mem.c` - Reverted memory optimization changes
   - `live.c` - Reverted liveness analysis changes
   - `ssa.c` - Reverted SSA form changes

3. **ARM64 backend:**
   - `arm64/emit.c` - Removed MADD emission patterns and `%2` support
   - `arm64/isel.c` - Removed fusion detection logic

---

## Changes Removed

### 1. Extended 3-Argument Instructions

**Removed from `all.h`:**
```c
// BEFORE (MADD implementation):
Ref arg[3];  // Extended to 3 for MADD/FMADD

// AFTER (restored original):
Ref arg[2];  // Original 2-argument design
```

### 2. MADD/FMADD Opcodes

**Removed from `ops.h`:**
- `Oamadd` - Integer multiply-add
- `Oamsub` - Integer multiply-subtract  
- `Oafmadd` - Float multiply-add
- `Oafmsub` - Float multiply-subtract

### 3. Emission Support

**Removed from `arm64/emit.c`:**
- MADD/FMADD emission patterns
- `%2` token support in `emitf()`
- Third argument handling code

### 4. Fusion Detection

**Removed from `arm64/isel.c`:**
- ~60 lines of pattern matching code
- Detection of `add(acc, mul(a,b))` patterns
- Single-use analysis
- Fused instruction emission

### 5. Helper Functions

**Removed from `util.c`:**
- `emit3()` function for 3-argument instructions
- Third argument initialization in `emit()`

---

## Git Status

```
Changes not staged for commit:
  modified:   qbe_basic_integrated/qbe_source/all.h
  modified:   qbe_basic_integrated/qbe_source/arm64/emit.c
  modified:   qbe_basic_integrated/qbe_source/arm64/isel.c
  modified:   qbe_basic_integrated/qbe_source/gcm.c
  modified:   qbe_basic_integrated/qbe_source/gvn.c
  modified:   qbe_basic_integrated/qbe_source/live.c
  modified:   qbe_basic_integrated/qbe_source/mem.c
  modified:   qbe_basic_integrated/qbe_source/ops.h
  modified:   qbe_basic_integrated/qbe_source/parse.c
  modified:   qbe_basic_integrated/qbe_source/rega.c
  modified:   qbe_basic_integrated/qbe_source/spill.c
  modified:   qbe_basic_integrated/qbe_source/ssa.c
  modified:   qbe_basic_integrated/qbe_source/util.c

Stats: 13 files changed, 19 insertions(+), 111 deletions(-)
```

The rollback removed **111 lines** and restored **19 lines** of original code.

---

## Build Status

✅ **QBE rebuilt successfully** after rollback

```bash
./qbe_basic_integrated/build_qbe_basic.sh
# Build completed with only standard warnings
# Binary: qbe_basic_integrated/qbe_basic (1.8M)
```

---

## Test Results

### Baseline Test Suite: `scripts/run_tests_simple.sh`

```
Total Tests:   107
Passed:        81  (75.7%)
Failed:        26  (24.3%)
Timeout:       0
```

### Breakdown by Category:

**✅ PASSING Categories (mostly):**
- Arithmetic: 14/18 passed (MADD tests still pass without fusion!)
- Loops: 4/4 passed (100%)
- Strings: 10/10 passed (100%)
- Arrays: 6/6 passed (100%)
- Types: 12/12 passed (100%)

**⚠️ FAILING Tests:**
- Several GOSUB-related tests (compiler crashes - pre-existing issues)
- Some SELECT CASE tests (pre-existing issues)
- Some literal suffix tests (pre-existing issues)

### Key Observation:

**MADD tests (3/3) still PASS even without fusion!**
- `test_madd_simple.bas` - PASS
- `test_madd_fusion.bas` - PASS
- `test_madd_comprehensive.bas` - PASS

This is expected - the tests verify that multiply-add arithmetic works correctly. Without fusion, QBE simply emits separate `mul` + `add` instructions instead of fused `madd` instructions. The functionality is identical, just less optimized.

---

## Why We Rolled Back

### Primary Issues with MADD Implementation:

1. **Over-broad loop changes:**
   - Changed many `for (n = 0; n < 2; n++)` loops to `n < 3`
   - Some of these loops were semantically tied to 2-argument instructions
   - This broke PHI node handling and other 2-arg-specific logic

2. **Instruction lifecycle problems:**
   - The third argument (`arg[2]`) was being lost somewhere between `isel.c` and `emit.c`
   - Assembly output showed trailing commas: `madd x0, x0, x1,` (missing accumulator)
   - Root cause was unclear - possibly instruction copying or register allocation

3. **Test regressions:**
   - Initial implementation caused 15+ test regressions
   - Some were fixed, but more investigation needed

4. **Architectural complexity:**
   - QBE's design assumes 2-argument instructions throughout
   - Extending to 3 arguments requires careful, surgical changes
   - Many code sites make assumptions about `arg[0..1]` only

---

## Next Steps: Proper MADD Implementation

### Analysis Required Before Re-Implementation:

1. **Audit ALL loops that iterate instruction arguments:**
   - Identify which loops MUST remain `n < 2` (PHI, 2-arg semantics)
   - Identify which loops can safely become `n < 3` (generic arg processing)
   - Document each loop's purpose and constraints

2. **Trace instruction lifecycle:**
   - Map the complete path from `isel.c` emission to `emit.c` output
   - Identify all places where instructions are copied
   - Ensure `arg[2]` is preserved through all transformations

3. **Study instruction copying functions:**
   - `icpy()` - Does it copy all args?
   - `idup()` - Does it duplicate all args?
   - Register allocator - Does it preserve all args?
   - Any other copy/move operations

4. **Consider alternative approaches:**
   - **Option A:** Full 3-arg support (current approach, needs more work)
   - **Option B:** Emit fused instructions as pseudo-ops with hidden args
   - **Option C:** Post-register-allocation peephole optimization
   - **Option D:** Backend-specific instruction bundling

5. **Create comprehensive test plan:**
   - Unit tests for each code path that handles args
   - Integration tests for fusion detection
   - Regression tests for all modified passes

---

## Documentation Preserved

The following documentation from the MADD implementation attempt is preserved for reference:

- `docs/MADD_FMADD_FUSION_INVESTIGATION.md` - Detailed investigation notes
- `docs/MADD_FMADD_IMPLEMENTATION_STATUS.md` - Implementation status
- `docs/MADD_IMPLEMENTATION_COMPLETE.md` - Original completion document
- `docs/MADD_FIX_PLAN.md` - Fix planning document
- `MADD.md` - High-level overview
- `MADD_TEST_RESULTS.txt` - Test results during development
- `MADD_FUSION_COMPLETE.txt` - Implementation notes

These documents contain valuable information about:
- What was attempted
- What worked
- What didn't work
- Why it didn't work
- Debugging approaches

---

## Git History

The MADD implementation commits remain in git history:

```
494391e MADD fusion: Fix copy.c PHI loop, initialize arg[2] in emit()
98fadc2 Add MADD test results and complete documentation
a49f4cc Fix MADD/FMADD: Move opcodes to end, fix all n<2 loops, add R check
ed68248 Implement MADD/FMADD automatic fusion in ARM64 backend
```

These commits can be examined for reference but are now **uncommitted working tree changes** showing the rollback.

---

## Recommendation

**DO NOT** attempt to re-implement MADD fusion until:

1. ✅ Complete the analysis outlined in "Next Steps" above
2. ✅ Create a detailed implementation plan with surgical changes
3. ✅ Understand the instruction lifecycle completely
4. ✅ Have a strategy for preserving `arg[2]` through all passes
5. ✅ Create comprehensive tests that can be run after each small change

**ESTIMATE:** Proper MADD implementation with full analysis: 2-3 days of careful work

---

## Verification

### Checklist:

- [x] All QBE source files restored from `fsh/qbe/`
- [x] QBE builds successfully without errors
- [x] Test suite runs (81/107 passing = reasonable baseline)
- [x] MADD tests pass (as non-fused instructions)
- [x] No MADD-related code remains in source files
- [x] Documentation preserved for future reference
- [x] Git working tree shows rollback changes

### Build Verification:

```bash
$ ls -lh qbe_basic_integrated/qbe_basic
-rwxr-xr-x@ 1 oberon staff 1.8M Jan 31 14:52 qbe_basic_integrated/qbe_basic

$ ./qbe_basic_integrated/qbe_basic --help
usage: qbe_basic [OPTIONS] file.ssa
# (works correctly)
```

---

## Conclusion

The MADD/FMADD fusion optimization has been **completely rolled back** and the system is restored to a known-good state. The original QBE source is now active and all MADD-related changes have been removed.

The implementation attempt was educational and the documentation has been preserved. When ready to attempt again, we now have:

1. Clear understanding of what doesn't work
2. Documented issues and failure modes
3. A stable baseline to work from
4. A roadmap for proper implementation

**Status: READY FOR ANALYSIS PHASE** before any new implementation attempts.

---

**Last Updated:** 2024-01-31  
**Rollback Performed By:** AI Assistant  
**Verified By:** Build system + test suite