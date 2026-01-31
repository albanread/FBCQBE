# QBE ARM64 Optimization Plan for FasterBASIC

**Date:** 2025-01-31  
**Status:** Planning Phase  
**Target:** Apple Silicon (ARM64/AArch64) Performance Enhancements

---

## Executive Summary

This document outlines a comprehensive plan to extend QBE's ARM64 backend with Apple Silicon-specific optimizations. By implementing hardware intrinsics for bit manipulation, SIMD operations, and fused multiply-add, FasterBASIC can achieve performance competitive with hand-written assembly.

**Expected Performance Gains:**
- Bit operations: **15-20x faster** (rbit vs C loops)
- Population count: **10-15x faster** (cnt vs branch-heavy C)
- Multiply-add: **2x faster + better precision** (CRITICAL for math-heavy code)
- Min/Max float: **3-5x faster** (single instruction vs library calls)

**Why MADD is Critical:**
- Most common pattern in numerical code
- Affects polynomials, physics, graphics, finance
- Automatic detection = no programmer burden
- Single rounding for floats = more accurate results

---

## QBE Extension Philosophy

QBE is **not** plugin-based. Extensions require **direct source modification**.

**Why this is good:**
- QBE is ~20k lines of readable C
- `arm64.c` backend can be understood in an afternoon
- Changes are surgical and well-defined
- Fork maintenance is straightforward

**Our Strategy:**
1. Fork QBE into our repository (already done: `qbe_basic_integrated/qbe_source/`)
2. Add target opcodes to `arm64/all.h`
3. Teach emitter (`arm64.c`) to print new instructions
4. Hook instruction selector (`isel`) to recognize patterns
5. Expose via call interception (no IL changes needed)

---

## Phase 1: Scalar Bit Manipulation (HIGH PRIORITY)

### 1.1 Bit Reverse (rbit)

**BASIC Function:** `BITREV(x)` or `REVERSEBITS(x)`

**Hardware:** ARM64 `rbit` instruction (1 cycle latency)

**Implementation Steps:**

1. **Add opcode** (`arm64/all.h`):
```c
enum Arm64Op {
    Oadd = 0,
    Osub,
    // ... existing ...
    Orbit,  // <-- Add this
    NArm64Op
};
```

2. **Teach emitter** (`arm64.c`):
```c
case Orbit:
    fprintf(f, "\trbit\t%s, %s\n", 
            rname(i.to.val, i.cls), 
            rname(i.arg[0].val, i.cls));
    break;
```

3. **Hook isel** (`arm64/isel.c`):
```c
if (i->op == Ocall && i->arg[1].type == Con) {
    char *name = str(i->arg[1].val);
    if (strcmp(name, "$__fb_bitrev") == 0) {
        emit(Orbit, Kx, i.to, i.arg[0], R);
        return;
    }
}
```

4. **Frontend codegen** (FasterBASIC):
```basic
LET x = BITREV(y)
```
Emits:
```qbe
%x =l call $__fb_bitrev(l %y)
```

**Benefits:**
- 1 cycle vs ~15 instructions for C implementation
- Used in: crypto, endian swapping, FFT bit-reversal

---

### 1.2 Count Leading Zeros (clz)

**BASIC Function:** `LEADZERO(x)` or `LOG2INT(x)`

**Hardware:** ARM64 `clz` instruction

**Implementation:**
```c
// Opcode
Oclz

// Emitter
case Oclz:
    fprintf(f, "\tclz\t%s, %s\n", 
            rname(i.to.val, i.cls), 
            rname(i.arg[0].val, i.cls));
    break;

// isel hook
if (strcmp(name, "$__fb_clz") == 0) {
    emit(Oclz, Kx, i.to, i.arg[0], R);
    return;
}
```

**Use Cases:**
- Integer log₂: `63 - clz(x)`
- Binary heap operations
- Finding highest set bit
- Normalization in floating-point emulation

**Edge Case:** `clz(0)` returns 64 on ARM64. Document this behavior.

---

### 1.3 Count Trailing Zeros (ctz)

**BASIC Function:** `TRAILZERO(x)` or `FIRSTBIT(x)`

**Hardware:** Synthesized from `rbit + clz`

**Implementation:**
```c
// No new opcode needed - compose existing ones
if (strcmp(name, "$__fb_ctz") == 0) {
    Ref tmpReg = newtmp();
    emit(Orbit, Kx, tmpReg, i.arg[0], R);  // Reverse bits
    emit(Oclz, Kx, i.to, tmpReg, R);       // Count leading zeros
    return;
}
```

**Algorithm:** Trailing zeros become leading zeros after bit reversal.

**Use Cases:**
- Iterator over set bits in bitmask
- Find next power of 2
- Ruler function in divide-and-conquer algorithms

**Performance:** 2 cycles (rbit + clz) vs 15+ cycles for C loop

---

### 1.4 Population Count (cnt)

**BASIC Function:** `BITCOUNT(x)` or `POPCOUNT(x)`

**Hardware:** ARM64 `cnt` (vector instruction)

**Challenge:** `cnt` operates on SIMD registers, requires special handling.

**Implementation:**
```c
// Opcode
Opcnt

// Emitter (requires fmov to/from vector registers)
case Opcnt:
    fprintf(f, "\tfmov\td0, %s\n", rname(i.arg[0].val, Kx));
    fprintf(f, "\tcnt\tv0.8b, v0.8b\n");
    fprintf(f, "\taddv\tb0, v0.8b\n");
    fprintf(f, "\tfmov\t%s, d0\n", rname(i.to.val, Kw));
    break;
```

**Sequence:**
1. Move integer to vector register (fmov)
2. Count bits in 8-byte lanes (cnt)
3. Sum all lanes (addv)
4. Move result back to integer register

**Use Cases:**
- Hamming weight (AI/ML)
- Bitboard operations (chess engines)
- Set cardinality in databases
- DNA sequence analysis

**Performance:** ~4 cycles vs 20+ cycles for C implementation

---

## Phase 2: Conditional & Arithmetic Optimization (MEDIUM PRIORITY)

### 2.1 Conditional Select (csel)

**BASIC Pattern:**
```basic
LET x = IF a > b THEN a ELSE b
```

**Hardware:** ARM64 `csel` (conditional select)

**Benefit:** Branchless selection eliminates branch misprediction penalties.

**Implementation:**
```c
// Detect pattern in isel:
// %cond = cmp %a, %b
// jnz %cond, @then, @else
// @then: %x = copy %a
// @else: %x = copy %b

// Transform to:
case Ocsel:
    fprintf(f, "\tcsel\t%s, %s, %s, %s\n",
            rname(i.to.val, i.cls),
            rname(i.arg[0].val, i.cls),
            rname(i.arg[1].val, i.cls),
            condition_name(i.arg[2]));
    break;
```

**Performance:** Predictable 1-cycle operation vs potential 15-20 cycle misprediction penalty.

---

### 2.2 Floating-Point Min/Max (fmaxnm / fminnm)

**BASIC Functions:** `MAX#(a, b)` and `MIN#(a, b)`

**Hardware:** ARM64 `fmaxnm`/`fminnm` (IEEE 754-2008 compliant)

**Current Problem:** C's `fmax()` often generates slow library calls.

**Implementation:**
```c
// Opcodes
Ofmax, Ofmin

// Emitter
case Ofmax:
    fprintf(f, "\tfmaxnm\t%s, %s, %s\n",
            rname(i.to.val, Kd),
            rname(i.arg[0].val, Kd),
            rname(i.arg[1].val, Kd));
    break;

// isel hook
if (strcmp(name, "$__fb_fmax") == 0) {
    emit(Ofmax, Kd, i.to, i.arg[0], i.arg[1]);
    return;
}
```

**NaN Handling:** `nm` suffix = "number" - prefers non-NaN values.

**Use Cases:**
- Bounding box calculations (graphics)
- Clamping values (UI, physics)
- Safe comparisons without branches

**Performance:** 1-2 cycles vs 10+ cycles for library call

---

### 2.3 Fused Multiply-Add (madd, fmadd) ⚡ HIGH PRIORITY

**BASIC Pattern:**
```basic
LET total = total + (price * quantity)
LET position# = position# + (velocity# * dt#)
```

**Hardware:** 
- `madd` - Integer multiply-add: `d = a + (b * c)`
- `fmadd` - Float multiply-add: `d = a + (b * c)` with single rounding

**Critical:** This MUST be automatic - programmers shouldn't need to call a special `MADD()` function.

**Auto-Optimization Strategy:** Pattern matching in instruction selector (isel).

**Implementation Approaches:**

**1. Backend Peephole (RECOMMENDED)** - Modify `arm64/isel.c`:

```c
// When processing Oadd instruction:
if (i->op == Oadd && i->cls == Kl) {
    // Look for pattern: %tmp = mul %a, %b; %res = add %tmp, %c
    Ins *mul_ins = find_def(i->arg[0]);  // Check if arg0 is from mul
    
    if (mul_ins && mul_ins->op == Omul) {
        // CRITICAL: Check that mul result is only used here
        if (single_use(mul_ins->to)) {
            // Transform to: %res = madd %a, %b, %c
            // ARM64: madd dest, src1, src2, accumulator
            // dest = (src1 * src2) + accumulator
            i->op = Omadd;
            i->arg[0] = mul_ins->arg[0];  // src1 (multiplier)
            i->arg[1] = mul_ins->arg[1];  // src2 (multiplicand)
            i->arg[2] = i->arg[1];         // accumulator (addend)
            
            // Mark mul as dead - don't emit it
            kill_ins(mul_ins);
            return;
        }
    }
    
    // Try other arg too: %res = add %c, %tmp
    mul_ins = find_def(i->arg[1]);
    if (mul_ins && mul_ins->op == Omul && single_use(mul_ins->to)) {
        i->op = Omadd;
        i->arg[0] = mul_ins->arg[0];
        i->arg[1] = mul_ins->arg[1];
        i->arg[2] = i->arg[0];  // accumulator is the other arg
        kill_ins(mul_ins);
    }
}

// Repeat for Kd (DOUBLE) to get fmadd
```

**Why Backend Approach is Better:**
- ✅ Works across multiple BASIC lines in same block
- ✅ Handles both INTEGER (madd) and DOUBLE (fmadd) automatically
- ✅ Keeps frontend simple - focus on language, not hardware
- ✅ Catches patterns the programmer didn't intend as optimization

**2. Frontend Expression Tree** (Alternative):

If not modifying QBE heavily, detect in FasterBASIC parser:

```c
// When parsing: x = a * b + c
if (expr->op == ADD && expr->left->op == MUL) {
    emit_call("$__fb_madd", expr->left->a, expr->left->b, expr->right);
} else {
    // Normal emission
}
```

Then hook in QBE isel:
```c
if (strcmp(name, "$__fb_madd") == 0) {
    emit(Omadd, Kx, i.to, i.arg[0], i.arg[1], i.arg[2]);
    return;
}
```

**ARM64 Instruction Format:**

```asm
madd  x0, x1, x2, x3   # x0 = (x1 * x2) + x3 (integer)
fmadd d0, d1, d2, d3   # d0 = (d1 * d2) + d3 (float, single rounding)
```

**Critical Detail:** The accumulator is the FOURTH register, not the third!

**Multiply-Subtract (MSUB) Bonus:**

While implementing madd, also detect subtract patterns:

```basic
' Pattern: x = c - (a * b)
LET x = c - (a * b)
```

Becomes:
```asm
msub x0, x1, x2, x3    # x0 = x3 - (x1 * x2)
```

**Implementation:**
```c
// In isel for Osub:
if (i->op == Osub) {
    Ins *mul_ins = find_def(i->arg[1]);  // Check if subtracting a mul
    if (mul_ins && mul_ins->op == Omul && single_use(mul_ins->to)) {
        i->op = Omsub;  // Target-specific msub opcode
        i->arg[0] = mul_ins->arg[0];  // multiplier
        i->arg[1] = mul_ins->arg[1];  // multiplicand
        i->arg[2] = i->arg[0];         // what we subtract from
        kill_ins(mul_ins);
    }
}
```

**Use Cases for MSUB:**
- Distance calculations: `dist = base - (scale * offset)`
- Coordinate geometry
- Financial adjustments
- Physics: `force = mass - (friction * velocity)`

**Benefits:**
- **Integer:** 2 cycles saved per operation
- **Float:** Better precision (single rounding) + 2 cycles saved
- **Code density:** Smaller binaries = better cache usage
- **Automatic:** Programmer writes natural math, gets optimization free

**Common Patterns Detected:**

```basic
' All these patterns should be caught:
total = total + price * quantity      ' madd
y = y + m * x                         ' madd (linear equation)
pos = pos + vel * dt                  ' madd (physics integration)
result = base - scale * offset        ' msub (adjustment)

' Even works across lines:
temp = a * b
result = result + temp                ' madd (if temp not used elsewhere)
```

**Use Cases:**
- Polynomial evaluation (Horner's method) - CRITICAL for performance
- Matrix operations (dot products, transformations)
- Physics integration (position/velocity updates)
- Financial calculations (compound interest, amortization)
- 3D graphics (transformation matrices)

---

## Phase 3: Advanced Optimizations (LOW PRIORITY)

### 3.1 Integer ABS Optimization

**Current:** 8 instructions with branches  
**Proposed:** Branchless bit manipulation

**Algorithm:**
```c
mask = x >> 31;      // -1 if negative, 0 if positive
abs = (x + mask) ^ mask;
```

**QBE Implementation:**
```c
// In isel for ABS on integers:
emit(Osar, Kw, mask, x, const_31);  // Arithmetic shift right
emit(Oadd, Kw, temp, x, mask);      // Add mask
emit(Oxor, Kw, result, temp, mask); // XOR with mask
```

**Benefit:** 3 instructions vs 8 + branches

---

### 3.2 SIMD / NEON Support

**Status:** NOT RECOMMENDED for QBE

**Reason:** QBE has no vector types in IL (no `v4f32`, etc.)

**Alternative Approach:**
- Write SIMD kernels in separate `.s` files or C intrinsics
- Compile separately with `clang -march=native`
- Link with QBE-generated code
- Call via standard ABI

**Example Use Cases:**
- Image processing (pixel operations)
- Audio DSP (sample processing)
- Vector/matrix math (3D graphics)

**Recommendation:** Create `fsh/FasterBASICT/runtime_asm/` directory for hand-written SIMD routines.

---

### 3.3 Apple Matrix Extension (AMX)

**Status:** NOT RECOMMENDED

**Reason:** 
- Undocumented by Apple
- Requires system register manipulation
- Uses registers QBE knows nothing about
- High risk of breaking with OS updates

**Alternative:** Use Apple's Accelerate framework via C FFI.

---

## Implementation Roadmap

### Sprint 1: Core Bit Operations (2-3 days)
- [ ] Add `rbit` (BITREV)
- [ ] Add `clz` (LEADZERO)
- [ ] Add `ctz` (TRAILZERO) using rbit+clz
- [ ] Add `cnt` (BITCOUNT)
- [ ] Write tests for each intrinsic
- [ ] Update FasterBASIC codegen to emit calls

### Sprint 2: Arithmetic Optimizations (2-3 days) ⚡ CRITICAL
- [ ] **Add `madd`/`fmadd`/`msub` fusion** (HIGHEST PRIORITY)
  - [ ] Implement peephole in `arm64/isel.c`
  - [ ] Detect `add(x, mul(a,b))` pattern
  - [ ] Detect `sub(x, mul(a,b))` pattern  
  - [ ] Handle both integer (madd) and float (fmadd)
  - [ ] Test single-use verification
  - [ ] Benchmark polynomial evaluation
- [ ] Add `fmaxnm`/`fminnm` (MIN#/MAX#)
- [ ] Add `csel` pattern detection
- [ ] Write benchmark comparing before/after
- [ ] Document automatic optimizations

### Sprint 3: Integer ABS (1 day)
- [ ] Implement branchless integer ABS
- [ ] Compare with current branching version
- [ ] Update existing ABS codegen

### Sprint 4: Testing & Documentation (2 days)
- [ ] Comprehensive test suite for all intrinsics
- [ ] Edge case tests (0, -1, max/min values)
- [ ] Performance benchmarks
- [ ] Update START_HERE.md with new intrinsics
- [ ] Document QBE fork maintenance process

**Total Estimated Time:** 7-9 days

---

## Testing Strategy

### Unit Tests

Create `tests/intrinsics/` directory:

```basic
' test_bitrev.bas
PRINT BITREV(0) = 0
PRINT BITREV(1) = 9223372036854775808  ' 2^63
PRINT BITREV(-1) = -1

' test_popcount.bas
PRINT BITCOUNT(0) = 0
PRINT BITCOUNT(15) = 4
PRINT BITCOUNT(-1) = 64

' test_clz.bas
PRINT LEADZERO(1) = 63
PRINT LEADZERO(0) = 64
```

### Performance Benchmarks

Create `tests/benchmarks/bench_intrinsics.bas`:

```basic
' Compare intrinsic vs C implementation
DIM i AS INTEGER
DIM start AS DOUBLE
DIM result AS INTEGER

start = TIMER
FOR i = 1 TO 1000000
  result = BITREV(i)
NEXT i
PRINT "BITREV: "; TIMER - start

' Repeat for BITCOUNT, LEADZERO, etc.
```

### Expected Results

| Operation | C Implementation | Intrinsic | Speedup |
|-----------|-----------------|-----------|---------|
| BITREV    | 15 cycles       | 1 cycle   | 15x     |
| BITCOUNT  | 20 cycles       | 4 cycles  | 5x      |
| LEADZERO  | 12 cycles       | 1 cycle   | 12x     |
| TRAILZERO | 15 cycles       | 2 cycles  | 7.5x    |
| FMAX      | 10 cycles       | 2 cycles  | 5x      |
| MADD      | 6 cycles        | 4 cycles  | 1.5x*   |

*Plus improved precision for floating-point (single rounding vs double rounding)

---

## FasterBASIC API Design

### New Keywords

```basic
' Bit manipulation
x = BITREV(value)         ' Reverse all bits
x = BITCOUNT(value)       ' Count set bits (population count)
x = LEADZERO(value)       ' Count leading zeros
x = TRAILZERO(value)      ' Count trailing zeros

' Math (floating point)
x# = FMAX(a#, b#)         ' Maximum (NaN-safe)
x# = FMIN(a#, b#)         ' Minimum (NaN-safe)

' Integer log2 (derived from LEADZERO)
FUNCTION LOG2INT(x AS INTEGER) AS INTEGER
  RETURN 63 - LEADZERO(x)
END FUNCTION

' Check if power of 2
FUNCTION ISPOW2(x AS INTEGER) AS INTEGER
  RETURN BITCOUNT(x) = 1
END FUNCTION
```

### Automatic Optimizations (No Keywords Needed)

These patterns are automatically optimized:

```basic
' Multiply-add: automatically uses MADD/FMADD
total = total + (price * quantity)
pos# = pos# + (vel# * time#)

' Min/max: automatically uses FMAXNM/FMINNM  
result# = IF a# > b# THEN a# ELSE b#
```

---

## Maintenance Considerations

### QBE Fork Management

**Repository Structure:**
```
qbe_basic_integrated/
├── qbe_source/          # Our fork of QBE
│   ├── arm64/          # Modified backend
│   └── VERSION.txt     # Track upstream version
└── patches/            # Our modifications as patches
    ├── 01-rbit.patch
    ├── 02-clz-ctz.patch
    └── 03-popcount.patch
```

**Workflow:**
1. Make changes to `qbe_source/arm64/`
2. Generate patch: `git diff > patches/feature.patch`
3. Document in `patches/README.md`
4. Commit both source and patch

**Merging Upstream Updates:**
1. Check QBE releases: https://c9x.me/git/qbe.git
2. Apply our patches to new version
3. Test thoroughly
4. Update VERSION.txt

---

## Documentation Deliverables

### For Users

- [ ] `docs/INTRINSICS.md` - Complete reference of all intrinsics
- [ ] `START_HERE.md` update - Add "Advanced Features" section
- [ ] Example programs showcasing performance gains

### For Developers

- [ ] `docs/QBE_BACKEND_GUIDE.md` - How to add new instructions
- [ ] `docs/QBE_FORK_MAINTENANCE.md` - Upstream merge process
- [ ] Inline comments in `arm64/isel.c` explaining patterns

---

## Risk Assessment

### Low Risk ✅
- Adding new opcodes (isolated changes)
- Call interception (pattern matching)
- Bit manipulation intrinsics (well-defined hardware)

### Medium Risk ⚠️
- Automatic pattern optimization (madd fusion)
  - Risk: May miss edge cases
  - Mitigation: Extensive testing, conservative matching
  
- Population count (SIMD register usage)
  - Risk: Register allocation conflicts
  - Mitigation: Use scratch registers, careful register management

### High Risk 🔴
- NEON/vector support (not planned)
- AMX support (not planned)
- Modifying register allocator

**Strategy:** Focus on low/medium risk items with high value.

---

## Success Metrics

### Performance Benchmarks

Target: **3-5x overall speedup** on bit-manipulation heavy code.

**Test Suite:**
1. Bitboard chess engine
2. Cryptographic hash (bit reversal, rotation)
3. Image processing (population count for masking)
4. Physics simulation (multiply-add heavy)
5. Financial calculations (fused operations for precision)

### Code Quality

- [ ] All intrinsics have unit tests
- [ ] Edge cases documented and tested
- [ ] Performance benchmarks show expected gains
- [ ] No regressions in existing test suite
- [ ] Documentation complete and reviewed

---

## Future Considerations

### Phase 4: Advanced Features (6+ months out)

1. **Bit manipulation operators:**
   ```basic
   x = ROTLEFT(value, count)
   x = ROTRIGHT(value, count)
   x = PARITY(value)          ' Even/odd parity
   ```

2. **Saturating arithmetic:**
   ```basic
   x = SATADD(a, b)   ' Saturating add (clamps on overflow)
   x = SATSUB(a, b)   ' Saturating subtract
   ```

3. **CRC instructions:**
   ```basic
   crc = CRC32(data, length)
   ```

4. **Crypto extensions:**
   - AES round functions
   - SHA-256/SHA-1 rounds
   - (Requires careful design for security)

---

## Conclusion

This optimization plan targets **high-value, low-risk** enhancements to the QBE ARM64 backend. By focusing on hardware intrinsics for bit manipulation and arithmetic optimization, FasterBASIC can achieve performance comparable to hand-tuned assembly for critical operations.

**Key Advantages:**
1. ✅ Minimal QBE changes (surgical modifications)
2. ✅ No IL changes (use call interception)
3. ✅ Easy to test and validate
4. ✅ Measurable performance gains (5-15x for bit ops)
5. ✅ Natural BASIC API (BITREV, BITCOUNT, etc.)

**Next Steps:**
1. Review this plan with team
2. Set up QBE fork infrastructure
3. Begin Sprint 1 (core bit operations)
4. Iterate based on results

---

**Document Version:** 1.0  
**Last Updated:** 2025-01-31  
**Author:** FasterBASIC Development Team