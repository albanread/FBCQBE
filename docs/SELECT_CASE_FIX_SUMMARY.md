# SELECT CASE Fix Summary

## Issues Found and Fixed

### 1. SELECT CASE with floating-point values was broken

**Problem:** The compiler generated invalid QBE IL when using DOUBLE variables in SELECT CASE.

**Root Cause:** The type conversion logic was **inverted** - when the SELECT expression was a DOUBLE, the code tried to convert it FROM integer TO double using `sltof`, but it was already a double!

**Fix:** Corrected the logic to:
- Check the actual type of the SELECT expression
- Check the actual type of each CASE value
- Only convert when types don't match
- Always use values directly when they're already the right type

### 2. Type glyphs on literals NOT supported (and that's OK)

**What we found:**
- ✅ Type glyphs work perfectly on VARIABLES: `DIM x%`, `DIM y#`, etc.
- ❌ Type glyphs do NOT work on LITERALS: `42%`, `3.14#` are not supported

**Why `42%` appears to work:** The `%` is parsed as modulo operator, not a type suffix!

**Why this is low priority:**
- FasterBASIC defaults to DOUBLE for numeric literals (appropriate for 64-bit)
- Type conversion happens automatically anyway
- Users can use `CINT()`, `CDBL()`, `CSNG()` for explicit typing
- The `%` operator creates ambiguity (is it modulo or type suffix?)

## Files Changed

- `fsh/FasterBASICT/src/codegen/qbe_codegen_main.cpp` (lines ~1030-1180)

## Tests Pass

All these now work correctly:
- Integer SELECT CASE with integer values ✅
- Double SELECT CASE with double values ✅
- Integer SELECT with double CASE values (auto-converts) ✅
- Double SELECT with integer CASE values (auto-converts) ✅
- Range tests (CASE x TO y) for both types ✅
- Multiple values (CASE 1, 2, 3) ✅
- Conditional tests (CASE IS > x) ✅

## Documentation

Created comprehensive analysis: `docs/SELECT_CASE_TYPE_GLYPH_ANALYSIS.md`

## Recommendation

**Type glyphs on literals:** LOW PRIORITY
- Current workaround is sufficient: use type conversion functions
- If implemented, start with `%` only (integers)
- Document that `#` and `!` are NOT supported on literals

The compiler is now mature and handles SELECT CASE correctly with all numeric types!
