# SUB Implementation Fix

**Date:** 2024-01-31  
**Status:** ✅ FIXED  
**Priority:** High (blocking feature)

## Problem Summary

SUB (void subroutine) calls were completely broken. Even the simplest SUB would fail to compile with a confusing error message:

```
Semantic Error at 0:0: GOTO target label : does not exist
```

**Test case that failed:**
```basic
SUB test_it()
  PRINT "Hello"
END SUB
test_it
END
```

This was a critical issue because:
- SUBs are fundamental to structured BASIC programming
- The workaround (using FUNCTIONs for everything) was awkward and semantically incorrect
- The error message gave no hint about the actual problem

## Root Cause Analysis

The issue was in the parser's handling of **implicit SUB calls** (calling a SUB without the `CALL` keyword).

### How FUNCTION vs SUB Differ

1. **FUNCTION** - Called in expressions, naturally handled by expression parser:
   ```basic
   result% = AddTwo(5, 3)  ' Function call in expression
   ```

2. **SUB** - Called as a statement, requires statement-level parsing:
   ```basic
   PrintHello              ' Implicit SUB call
   CALL PrintHello()       ' Explicit SUB call with CALL keyword
   ```

### The Bug

In `fasterbasic_parser.cpp`, when the parser encountered a bare identifier at statement level:

```cpp
case TokenType::IDENTIFIER:
    // Check if this is an implicit LET statement (variable assignment)
    if (isAssignment()) {
        return parseLetStatement();
    }
    // Fall through to error for bare identifiers
    [[fallthrough]];
```

It would:
1. Check if it's a variable assignment (e.g., `x = 10`)
2. If not, **fall through to the default error handler**

This meant `test_it` was treated as an error, not as a SUB call.

The confusing error message about "GOTO target label :" occurred because the parser couldn't handle the bare identifier and something in the error recovery path generated a malformed GOTO statement.

## Solution Implemented

Modified the parser to treat bare identifiers as **implicit SUB/FUNCTION calls** when they're not assignments.

### Code Changes

**File:** `fsh/FasterBASICT/src/fasterbasic_parser.cpp`  
**Lines:** 609-642

**Before:**
```cpp
case TokenType::IDENTIFIER:
    if (isAssignment()) {
        return parseLetStatement();
    }
    // Fall through to error for bare identifiers
    [[fallthrough]];
```

**After:**
```cpp
case TokenType::IDENTIFIER:
    if (isAssignment()) {
        return parseLetStatement();
    }
    // Otherwise, treat as implicit SUB/FUNCTION call (without CALL keyword)
    // This allows syntax like: MySub or MySub(args)
    {
        std::string subName = current().value;
        advance();

        auto stmt = std::make_unique<CallStatement>(subName);

        // Check for parentheses with arguments
        if (current().type == TokenType::LPAREN) {
            advance();

            // Parse argument list
            if (current().type != TokenType::RPAREN) {
                do {
                    auto arg = parseExpression();
                    if (arg) {
                        stmt->addArgument(std::move(arg));
                    }
                } while (match(TokenType::COMMA));
            }

            consume(TokenType::RPAREN, "Expected ')' after arguments");
        }

        return stmt;
    }
```

### Supported Syntax

The fix now supports all standard BASIC SUB call syntaxes:

1. **Bare identifier** (no parens, no args):
   ```basic
   MySub
   ```

2. **With parentheses** (no args):
   ```basic
   MySub()
   ```

3. **With arguments** (must use parens):
   ```basic
   MySub(x, y, z)
   ```

4. **Explicit CALL** (already worked):
   ```basic
   CALL MySub(x, y, z)
   ```

### Why This is Safe

1. **No ambiguity** - At statement level, a bare identifier can only be:
   - A variable assignment (has `=` or array subscript followed by `=`)
   - A label (has `:` after it)
   - A SUB/FUNCTION call (everything else)

2. **Backward compatible** - `CALL` syntax still works

3. **Matches standard BASIC** - Most BASIC dialects support implicit SUB calls

4. **Type checking happens later** - The parser doesn't need to know if something is a SUB or FUNCTION; semantic analysis and codegen handle the distinction

## Testing

### Test File Created

**File:** `tests/functions/test_sub.bas`  
**Coverage:** 8 test cases covering:
- Implicit SUB calls (no parens, no CALL)
- Explicit SUB calls with CALL keyword
- SUBs with no parameters
- SUBs with typed parameters (INT, STRING, DOUBLE)
- SUBs with mixed parameter types
- SUBs calling other SUBs
- Nested SUB calls

### Test Results

```
✓ All 8 test cases pass
✓ Output matches expected output exactly
✓ No regression in existing test suite (56/66 tests passing - same as before fix)
```

### Manual Testing

**Simple test:**
```basic
SUB test_it()
  PRINT "Hello"
END SUB
test_it
END
```

**Result:** ✅ Compiles and runs correctly, outputs "Hello"

**Before fix:** ❌ Semantic Error at 0:0: GOTO target label : does not exist

## Impact

### What Works Now

✅ All SUB syntax variations  
✅ SUBs with any parameter types  
✅ SUBs calling other SUBs  
✅ Nested SUB calls  
✅ Mixed use of implicit and explicit (CALL) syntax

### What's Still the Same

- FUNCTION calls work exactly as before
- GOSUB/RETURN (legacy line-number subroutines) unchanged
- All other language features unaffected

### Performance

No performance impact - the change is purely at parse time.

## Comparison with FUNCTION

Now both FUNCTIONs and SUBs work consistently:

| Feature | FUNCTION | SUB |
|---------|----------|-----|
| Returns value | ✅ Yes | ❌ No (void) |
| Called in expressions | ✅ Yes | ❌ No |
| Called as statement | ✅ Yes (assigns to variable name) | ✅ Yes |
| Implicit call syntax | ✅ Yes | ✅ Yes (NOW FIXED) |
| CALL keyword | ✅ Optional | ✅ Optional |
| Parameters | ✅ Yes | ✅ Yes |

## Why This Matters

SUBs are essential for structured programming:

1. **Semantic correctness** - Procedures that don't return values should be SUBs, not FUNCTIONs
2. **Code clarity** - `PrintReport()` is clearer as a SUB than as a FUNCTION that returns a dummy value
3. **BASIC compatibility** - Most BASIC dialects distinguish SUB and FUNCTION
4. **Best practices** - Separation of commands (SUBs) from queries (FUNCTIONs)

## Next Steps

This fix unblocks:
- ✅ Writing modular test suites using SUBs
- ✅ Porting existing BASIC code that uses SUBs
- ✅ Clean separation of procedures and functions
- ✅ Following standard BASIC programming patterns

The TODO list has been updated to mark SUB implementation as FIXED.

## Files Changed

1. `fsh/FasterBASICT/src/fasterbasic_parser.cpp` - Parser fix (lines 609-642)
2. `tests/functions/test_sub.bas` - Comprehensive test suite
3. `tests/functions/test_sub.expected` - Expected output
4. `TODO_LIST.md` - Updated status from BROKEN to FIXED
5. `docs/SUB_IMPLEMENTATION_FIX.md` - This document

## Verification

To verify the fix works:

```bash
cd qbe_basic_integrated
./build_qbe_basic.sh
cd ..
./qbe_basic_integrated/qbe_basic tests/functions/test_sub.bas -o test_sub
./test_sub
```

Expected: All 8 test cases pass, output matches `tests/functions/test_sub.expected`

---

**Issue Resolution:** The mystery of why "SUB works but FUNCTION doesn't" was backwards - it was actually "FUNCTION works but SUB doesn't" because FUNCTIONs are naturally called in expressions (which the parser handles), but SUBs need statement-level call syntax support, which was missing.

The fix is simple, safe, and restores expected BASIC language behavior. ✅