# QBE ARM64 Optimization - Documentation Index

## Overview

Complete documentation for extending QBE with Apple Silicon hardware intrinsics.

---

## Planning Documents

### 1. **QBE_ARM64_OPTIMIZATION_PLAN.md** (647 lines)
**Purpose:** Comprehensive technical implementation guide

**Contents:**
- Executive summary with performance projections
- Phase-by-phase implementation roadmap
- Detailed code examples for each intrinsic
- Testing strategy and success metrics
- Timeline: 7-9 days of focused work

**Audience:** Developers implementing the optimizations

---

### 2. **WHY_QBE_INTRINSICS.md**
**Purpose:** Justification and comparison with alternatives

**Contents:**
- Why not inline assembly?
- Why not C runtime functions?
- Why not LLVM?
- Real-world performance comparisons
- The "Goldilocks" argument for QBE

**Audience:** Technical decision makers, skeptics

---

### 3. **INTRINSICS_REFERENCE.md**
**Purpose:** User-facing API documentation

**Contents:**
- Quick reference table of all intrinsics
- Detailed function descriptions
- Usage examples
- Derived functions (LOG2INT, ISPOW2)

**Audience:** BASIC programmers using FasterBASIC

---

## Implementation Tracking

### 4. **TODO_LIST.md** (updated)
**Purpose:** Project management and priorities

**Status:** QBE optimization listed as HIGH priority

**Includes:**
- SUB implementation (blocker)
- QBE ARM64 intrinsics (next major focus)
- Phase 1 & 2 checklists

---

## Summary Documents

### 5. **QBE_OPTIMIZATION_SUMMARY.md**
**Purpose:** Quick executive summary

**Contents:**
- Documents created
- Planned intrinsics with speedups
- Timeline estimate
- Next steps

---

## Planned Intrinsics Summary

| Function | Hardware | Speedup | Phase |
|----------|----------|---------|-------|
| BITREV | rbit | 15-20x | 1 |
| BITCOUNT | cnt | 5x | 1 |
| LEADZERO | clz | 12x | 1 |
| TRAILZERO | rbit+clz | 7.5x | 1 |
| FMAX/FMIN | fmaxnm/fminnm | 5x | 2 |
| (auto MADD) | madd/fmadd | 1.5-2x | 2 |
| (auto CSEL) | csel | varies | 2 |

**Expected overall gain:** 3-5x on bit/math-heavy code

---

## Implementation Phases

### Phase 1: Core Bit Operations (2-3 days)
- Add rbit, clz, cnt instructions
- Synthesize ctz from rbit+clz
- Create test suite
- Update FasterBASIC codegen

### Phase 2: Arithmetic (2-3 days)
- Add fmaxnm/fminnm
- Pattern detection for csel
- Multiply-add fusion
- Performance benchmarks

### Phase 3: Integer ABS (1 day)
- Branchless bit manipulation
- Replace branching version

### Phase 4: Testing & Docs (2 days)
- Comprehensive tests
- Edge cases
- User documentation
- QBE fork maintenance guide

---

## Repository Structure

```
docs/
├── QBE_ARM64_OPTIMIZATION_PLAN.md  ← Main technical guide
├── WHY_QBE_INTRINSICS.md           ← Rationale document
├── INTRINSICS_REFERENCE.md         ← User API reference
├── QBE_OPTIMIZATION_INDEX.md       ← This file
└── QBE_OPTIMIZATION_SUMMARY.md     ← Quick summary

qbe_basic_integrated/
├── qbe_source/                     ← Our QBE fork
│   └── arm64/                      ← Target for modifications
└── patches/                        ← (To be created)
    ├── 01-rbit.patch
    ├── 02-clz-ctz.patch
    └── 03-popcount.patch

tests/
└── intrinsics/                     ← (To be created)
    ├── test_bitrev.bas
    ├── test_popcount.bas
    └── bench_intrinsics.bas
```

---

## Quick Start Guide

### For Implementers

1. Read **QBE_ARM64_OPTIMIZATION_PLAN.md** (comprehensive guide)
2. Review **WHY_QBE_INTRINSICS.md** (understand approach)
3. Check **TODO_LIST.md** (current priorities)
4. Begin Sprint 1 when ready

### For Users (After Implementation)

1. Read **INTRINSICS_REFERENCE.md** (API docs)
2. Try examples in your BASIC code
3. Compare performance before/after

---

## Status

**Current:** ✅ Planning complete, ready for implementation  
**Priority:** HIGH (after SUB implementation)  
**Estimated Timeline:** 7-9 days  
**Estimated Impact:** 3-5x performance gain on relevant code

---

## Questions?

- Technical details → QBE_ARM64_OPTIMIZATION_PLAN.md
- Why this approach → WHY_QBE_INTRINSICS.md
- User API → INTRINSICS_REFERENCE.md
- Project status → TODO_LIST.md

