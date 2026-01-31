# FasterBASIC TODO List

## High Priority

### ✅ SUB Implementation (FIXED)

**Status:** ✅ FIXED - SUBs now work correctly  
**Issue:** SUBs were failing with "GOTO target label : does not exist" error  
**Root cause:** Parser didn't handle implicit SUB calls (bare identifier without CALL keyword)

**Solution implemented:**
- Modified parser to treat bare identifiers as implicit SUB/FUNCTION calls when not assignments
- Added support for syntax: `MySub`, `MySub()`, `MySub(args)`
- Maintains backward compatibility with `CALL MySub(args)` syntax

**Test cases now passing:**
```basic
SUB test_it()
  PRINT "Hello"
END SUB
test_it          ' Works now!
END
```

**Fixed in:** `fsh/FasterBASICT/src/fasterbasic_parser.cpp` (lines 609-642)
- Bare identifiers now create CallStatement instead of falling through to error
- Both `SUB name` and `SUB name()` syntax supported
- FUNCTIONs and SUBs use same calling mechanism

### 🔴 MADD/FMADD Automatic Fusion (CRITICAL)

**Status:** Highest priority optimization  
**Priority:** CRITICAL (affects most numerical code)

**Why This is #1 Priority:**
- Most common pattern in ALL numerical code
- Affects polynomials, physics, graphics, finance, ML
- Must be AUTOMATIC - no special function calls needed
- Single rounding for floats = more accurate results
- 2x speedup + eliminates precision loss

**Implementation: Backend Peephole in `arm64/isel.c`**
- [ ] Detect `add(x, mul(a,b))` pattern automatically
- [ ] Detect `sub(x, mul(a,b))` pattern (MSUB bonus)
- [ ] Verify single-use of mul result
- [ ] Handle both integer (madd) and double (fmadd)
- [ ] Test with polynomial evaluation benchmark
- [ ] Test with physics integration code

**Key Insight:** Backend approach catches patterns across lines that programmer didn't intend as optimization.

**Examples that should be caught:**
```basic
total = total + price * quantity     ' madd
pos = pos + vel * dt                 ' fmadd (physics)
result = base - scale * offset       ' msub
temp = a * b
result = result + temp               ' madd (if temp unused elsewhere)
```

**See:** `docs/QBE_ARM64_OPTIMIZATION_PLAN.md` Section 2.3 for implementation details

---

### QBE ARM64 Backend Optimizations

**Status:** Planned (see `docs/QBE_ARM64_OPTIMIZATION_PLAN.md`)  
**Priority:** HIGH (significant performance gains)

**Phase 1: Core Bit Operations**
- [ ] Add `rbit` instruction (BITREV) - 15-20x speedup
- [ ] Add `clz` instruction (LEADZERO) - 12x speedup
- [ ] Add `ctz` synthesis (TRAILZERO) - 7.5x speedup using rbit+clz
- [ ] Add `cnt` instruction (BITCOUNT) - 5x speedup
- [ ] Create intrinsic test suite

**Phase 2: Other Arithmetic Optimizations**
- [ ] Add `fmaxnm`/`fminnm` (FMAX/FMIN) - 5x speedup
- [ ] Add `csel` pattern detection (branchless conditionals)
- [ ] Write performance benchmarks

**Expected Overall Gain:** 3-5x speedup on bit-manipulation and math-heavy code

**Documentation:** Complete plan in `docs/QBE_ARM64_OPTIMIZATION_PLAN.md`

---

## Medium Priority

### Performance Optimizations

1. **Integer ABS optimization**
   - Current: 8 instructions with branches
   - Proposed: Branchless bit manipulation (3-4 instructions)
   ```
   mask = x >> 31              # -1 if negative, 0 if positive
   abs = (x + mask) ^ mask
   ```

2. **OR-chain to MOD transformation**
   - Detect: `IF x = 1 OR x = 3 OR x = 5 OR x = 7 OR x = 9`
   - Transform to: `IF x MOD 2 = 1 AND x >= 1 AND x <= 9`
   - Benefit: Constant-time vs O(n) comparisons

3. **Constant pre-loading in loops**
   - Detect repeated constant loads (adrp/ldr) in hot loops
   - Hoist to loop preheader
   - Peephole optimization pass

### Language Features

1. **Division operator lint/warning**
   - Warn when `/` is used where `\` (integer division) might be intended
   - Help users write more efficient code
   - Make it optional/configurable

2. **Literal type suffixes**
   - Currently only work on variables: `x#`, `i%`
   - Consider adding for literals: `3.14#`, `42%`
   - Low priority - conversion functions work fine

---

## Low Priority

### Code Quality

1. **Temp variable limit documentation**
   - Document that very large single functions hit temp limit (~%t156-%t289)
   - Recommend modular design with FUNCTIONs/SUBs
   - Consider increasing limit or better error message

2. **Peephole optimizer**
   - Detect repeated ABS/SGN on same value
   - Detect redundant type conversions
   - Common subexpression elimination

3. **Better error messages**
   - Line number tracking in semantic errors
   - Show actual source line with error
   - Suggest fixes for common mistakes

---

## Planned Features

### QBE ARM64 Intrinsics (Next Major Focus)
Once SUBs are fixed, implement Apple Silicon hardware intrinsics:
- BITREV(x) - Bit reversal
- BITCOUNT(x) - Population count
- LEADZERO(x) - Count leading zeros
- TRAILZERO(x) - Count trailing zeros
- FMAX(a, b) - Fast floating-point max
- FMIN(a, b) - Fast floating-point min
- Automatic multiply-add fusion

See `docs/QBE_ARM64_OPTIMIZATION_PLAN.md` for complete roadmap.

---

## Completed ✅

### 2025-01-31

- ✅ **SELECT CASE float support** - Fixed type inference and conversion
- ✅ **ABS optimization** - Bit manipulation (3 instructions, 1.5x-3x speedup)
- ✅ **SGN optimization** - Branchless comparison (5 instructions, 2x-4x speedup)
- ✅ **Test suite organization** - Added conditionals, comparisons, data, io directories
- ✅ **Documentation cleanup** - Organized docs/, archived old tests

---

## Notes

### Testing Standards
- All new features should have tests in `tests/` subdirectories
- Use FUNCTIONs for modular tests (SUBs broken)
- Include expected output files for automated validation

### Documentation Standards
- Major features documented in `docs/`
- Keep `DOCUMENTATION_INDEX.md` updated
- Root directory: only essential docs (README, START_HERE, BUILD, etc.)

---

## Future Considerations

- Profile-guided optimization integration
- Better Unicode/string handling
- User-defined type improvements
- Array performance optimizations
- Inline assembly support (?)