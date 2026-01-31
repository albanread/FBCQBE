# MADD/FMADD Fusion Analysis for QBE ARM64 Backend

**Date:** 2026-01-31  
**Status:** Research & Planning  
**Related:** `docs/MADD_ROLLBACK_SUMMARY.md`

## Executive Summary

After reviewing other QBE forks and the upstream codebase, **no existing implementation of MADD/FMADD fusion was found**. The official QBE ARM64 backend currently does not perform multiply-add fusion, though it does use MSUB (multiply-subtract) for modulo operations.

This document outlines the research findings and proposes implementation strategies for adding MADD/FMADD fusion to our QBE integration.

---

## Background

### What is MADD/FMADD?

ARM64 provides fused multiply-add/subtract instructions:

- **MADD** (Multiply-Add): `rd = ra + (rn * rm)`
- **MSUB** (Multiply-Subtract): `rd = ra - (rn * rm)`
- **FMADD** (Floating Multiply-Add): `fd = fa + (fn * fm)`
- **FMSUB** (Floating Multiply-Subtract): `fd = fa - (fn * fm)`
- **FNMADD** (Floating Negate Multiply-Add): `fd = -fa - (fn * fm)`
- **FNMSUB** (Floating Negate Multiply-Subtract): `fd = -fa + (fn * fm)`

### Why Fusion Matters

1. **Performance**: Single instruction instead of two (multiply + add/sub)
2. **Precision**: Floating-point FMADD uses full intermediate precision (no rounding between multiply and add)
3. **Register Pressure**: Eliminates temporary register for multiply result
4. **Common Pattern**: `a + b * c` is extremely common in numeric code

### Current QBE State

Looking at `arm64/emit.c` (lines 52-58):

```c
{ Omul,    Ki, "mul %=, %0, %1" },
{ Omul,    Ka, "fmul %=, %0, %1" },
{ Odiv,    Ki, "sdiv %=, %0, %1" },
{ Odiv,    Ka, "fdiv %=, %0, %1" },
{ Oudiv,   Ki, "udiv %=, %0, %1" },
{ Orem,    Ki, "sdiv %?, %0, %1\n\tmsub\t%=, %?, %1, %0" },
{ Ourem,   Ki, "udiv %?, %0, %1\n\tmsub\t%=, %?, %1, %0" },
```

**Key observation**: QBE already uses MSUB for modulo! The pattern is:
```
temp = a / b
result = a - (temp * b)  // using MSUB
```

This proves QBE's infrastructure can handle 3-operand instructions in the emitter.

---

## Research: Other QBE Forks

### Forks Reviewed

1. **tobhe/qbe** - PowerPC backend port
   - Focus: Adding PowerPC architecture support
   - No MADD/FMADD work found
   - Not relevant to ARM64 fusion

2. **michaelforney/qbe** - Personal branch
   - Various cleanups and fixes
   - No MADD/FMADD fusion found

3. **Official c9x.me/qbe** - Upstream repository
   - No MADD/FMADD fusion in ARM64 backend
   - MSUB already used for modulo operations
   - No mailing list discussions about MADD fusion found

### Conclusion

**We are pioneering this feature.** No existing implementation exists to reference, which means:
- ✅ We have freedom to design it properly
- ⚠️ We must be extra careful about correctness
- 📚 We should document our approach thoroughly

---

## Why Our Previous Attempt Failed

See `docs/MADD_ROLLBACK_SUMMARY.md` for full details. Key issues:

### 1. **Overly Invasive Changes**
- Changed `Ins.arg[2]` to `Ins.arg[3]` globally
- Modified loop bounds from `n < 2` to `n < 3` in too many places
- Broke PHI node handling and other 2-arg assumptions

### 2. **Incomplete Instruction Lifecycle**
- New 3-arg instructions weren't properly copied by `icpy()`
- Register allocator didn't understand 3-arg semantics
- Spiller didn't preserve all arguments
- Third operand was "lost" during compilation, resulting in trailing commas

### 3. **Opcode Renumbering**
- Inserted new opcodes in the middle of the enum
- Shifted all existing opcode values
- Broke binary compatibility and debugging

### 4. **Lack of Incremental Approach**
- Changed everything at once
- No unit tests at each step
- Couldn't isolate which change broke what

---

## Implementation Strategies

### Strategy A: Backend Peephole Optimization (RECOMMENDED)

**Approach**: Pattern-match in the ARM64 emitter just before assembly output.

**Pros:**
- ✅ Minimal changes to core QBE
- ✅ No changes to instruction structure
- ✅ No changes to register allocator
- ✅ Similar to how MSUB already works
- ✅ Easy to enable/disable
- ✅ No risk to existing code

**Cons:**
- ⚠️ Limited to local patterns (within basic block)
- ⚠️ May miss some opportunities
- ⚠️ Operates on already-allocated registers

**Implementation Sketch:**

```c
// In arm64/emit.c, before emitting each instruction:

static int
trymaddfusion(Ins *i, Ins *prev, E *e)
{
    // Pattern: ADD dest, src1, temp
    //          where temp = MUL arg1, arg2 (single use)
    if (i->op == Oadd && prev->op == Omul) {
        if (req(i->arg[1], prev->to) && 
            fn->tmp[prev->to.val].nuse == 1) {
            // Emit: MADD dest, arg1, arg2, src1
            fprintf(e->f, "\tmadd\t%s, %s, %s, %s\n",
                rname(i->to.val, i->cls),
                rname(prev->arg[0].val, i->cls),
                rname(prev->arg[1].val, i->cls),
                rname(i->arg[0].val, i->cls));
            return 1; // Skip both instructions
        }
    }
    return 0;
}
```

**Where to hook:**
- In `emitins()` function before the main switch
- Look back at previous instruction(s)
- Check if pattern matches and registers allow fusion
- Emit fused instruction and mark originals as consumed

---

### Strategy B: Post-Register-Allocation Pass

**Approach**: Add a new pass after `rega()` that rewrites instruction sequences.

**Pros:**
- ✅ Registers already allocated
- ✅ Can look across multiple instructions
- ✅ Can use dataflow analysis
- ✅ Clean separation of concerns

**Cons:**
- ⚠️ Requires new pass infrastructure
- ⚠️ Must handle register liveness correctly
- ⚠️ More complex than peephole

**Implementation Location:**
- Add `arm64/fuse.c` with `arm64_fuse(Fn *fn)` function
- Call between `rega()` and emitter in `main.c`
- Rewrite IR sequences before emission

---

### Strategy C: Pre-Instruction-Selection Pass

**Approach**: Add fusion logic in `isel.c` or earlier optimization passes.

**Pros:**
- ✅ Can influence register allocation
- ✅ Virtual registers still available
- ✅ Can use SSA properties

**Cons:**
- ⚠️ Must add 3-arg instruction support throughout pipeline
- ⚠️ This is what we tried before and failed
- ⚠️ High risk of regressions

**Not Recommended** - This is the invasive approach that failed before.

---

## Recommended Implementation Plan

### Phase 1: Peephole in Emitter (Low Risk)

1. **Add pattern matching in `arm64/emit.c`**
   ```c
   static Ins *lastmul = NULL;
   
   // In emitins(), before Table lookup:
   if (lastmul && i->op == Oadd) {
       if (trymaddfusion(i, lastmul, e)) {
           lastmul = NULL;
           return; // Skip, already emitted
       }
   }
   
   if (i->op == Omul)
       lastmul = i;
   else
       lastmul = NULL;
   ```

2. **Implement `trymaddfusion()` carefully**
   - Check register liveness
   - Verify single use of multiply result
   - Emit MADD with correct operand order
   - Handle both integer and floating-point

3. **Add test cases**
   ```qbe
   # test/madd1.ssa
   function w $test(w %a, w %b, w %c) {
   @start
       %t =w mul %b, %c
       %r =w add %a, %t
       ret %r
   }
   # Should emit: madd w0, w1, w2, w0
   ```

4. **Validate against test suite**
   - Ensure all 104 tests still pass
   - Add new fusion-specific tests
   - Test with fusion disabled (feature flag)

### Phase 2: Extend to More Patterns

After Phase 1 is stable:
- SUB + MUL → MSUB (multiply-subtract)
- Floating-point FMADD/FMSUB
- NEG + MADD → FNMADD/FNMSUB patterns
- Cross-basic-block fusion (if profitable)

### Phase 3: Advanced Optimization (Future)

- Dataflow analysis for better fusion opportunities
- Cost model (when fusion actually helps)
- Integration with other ARM64 optimizations

---

## Technical Details

### ARM64 Instruction Format

**MADD**:
```assembly
madd <Xd>, <Xn>, <Xm>, <Xa>
# Xd = Xa + (Xn * Xm)
```

**Operand mapping**:
- `Xd` (destination) ← result of ADD
- `Xn` (first multiply operand) ← MUL arg[0]
- `Xm` (second multiply operand) ← MUL arg[1]
- `Xa` (addend) ← ADD arg[0] (the non-multiply operand)

### Conditions for Safe Fusion

1. **Single-use multiply**: Multiply result used exactly once
2. **Register availability**: No conflicts in operand mapping
3. **No side effects**: Instructions are adjacent or can be reordered
4. **Type compatibility**: All operands same width (W or L for integers, S or D for floats)

### Edge Cases to Handle

- **Multiply result used multiple times**: Cannot fuse
- **Add has constant operand**: May not be fusible depending on constant
- **Commutative operations**: Try both operand orders
- **Phi nodes**: Be careful with basic block boundaries
- **Spilled temporaries**: Cannot fuse if multiply result is spilled

---

## Measurements & Validation

### How to Measure Success

1. **Correctness**: All existing tests must pass
2. **Fusion rate**: Count fused vs. fusible patterns
3. **Performance**: Benchmark numeric-heavy BASIC programs
4. **Code size**: Measure assembly output size reduction

### Test Programs to Benchmark

```basic
' Matrix multiply (dense FMADD opportunities)
FOR i = 1 TO N
    FOR j = 1 TO N
        FOR k = 1 TO N
            C(i,j) = C(i,j) + A(i,k) * B(k,j)
        NEXT k
    NEXT j
NEXT i

' Polynomial evaluation
result = a0 + a1*x + a2*x^2 + a3*x^3

' Physics simulation
velocity = velocity + acceleration * dt
position = position + velocity * dt
```

### Validation Checklist

- [ ] All 104+ existing tests pass
- [ ] New MADD-specific tests pass
- [ ] Assembly output verified manually for sample programs
- [ ] No performance regression on non-fusible code
- [ ] Measurable speedup on numeric benchmarks
- [ ] Code works with fusion enabled and disabled (feature flag)

---

## Open Questions

1. **Should we fuse across basic blocks?**
   - Probably not initially (too complex)
   - Revisit in Phase 3

2. **What about multiply-negate-add patterns?**
   - FNMADD: `-(a + b*c)` or `-a - b*c`
   - FNMSUB: `-a + b*c`
   - Worth supporting for completeness

3. **How to handle constant operands?**
   - ARM64 MADD doesn't take immediates
   - Need register for all operands
   - Existing constant-loading logic should handle this

4. **Should this be optional?**
   - Add `-G madd` flag to enable/disable
   - Useful for debugging and comparison
   - Default: enabled (after proven stable)

---

## References

### ARM64 ISA Documentation

- **ARM Architecture Reference Manual ARMv8**
  - Section C3.5.10: Multiply-add/subtract instructions
  - MADD, MSUB, SMADDL, SMSUBL, UMADDL, UMSUBL

- **Floating-point fused operations**
  - FMADD, FMSUB, FNMADD, FNMSUB
  - Full precision intermediate result (no rounding)

### QBE Code Locations

- `arm64/emit.c` - Instruction emission (lines 37-687)
- `arm64/isel.c` - Instruction selection (lines 1-316)
- `ops.h` - Opcode definitions
- `all.h` - Instruction structure (`Ins`)

### Relevant QBE Tests

- `test/abi1.ssa` - ABI testing
- `test/abi2.ssa` - More ABI tests
- `test/*.ssa` - General instruction tests

---

## Conclusion

**Recommended Approach:** Start with Strategy A (Backend Peephole Optimization)

1. **Low risk** - Changes isolated to emitter
2. **Quick win** - Can implement and test in a day
3. **Proven pattern** - Similar to existing MSUB usage
4. **Easy to extend** - Foundation for more sophisticated fusion later

**Timeline:**
- Phase 1 (Peephole): 1-2 days implementation + testing
- Phase 2 (Extended patterns): 2-3 days
- Phase 3 (Advanced): Future work, as needed

**Success Criteria:**
- Zero test regressions
- Measurable fusion rate (target: 60%+ of fusible patterns)
- 5-10% speedup on numeric benchmarks

Let's implement this incrementally, with testing at each step, avoiding the mistakes of the previous attempt.

---

**Next Steps:**
1. Create `arm64_madd_peephole.c` with initial fusion logic
2. Add feature flag to enable/disable
3. Write 5-10 test cases
4. Integrate into `arm64/emit.c`
5. Run full test suite
6. Benchmark and measure

**Status:** Ready to implement Phase 1