# Exception Handling - Quick Summary

## What We're Building

Simple, efficient exception handling for FasterBASIC using **error codes only** (not objects).

## Syntax at a Glance

```basic
TRY
    LET X% = 10 / 0
CATCH 11
    PRINT "Division by zero"
END TRY
```

## Key Features

✅ **Error codes** - Integers (5, 9, 11, 13, etc.)
✅ **Multiple codes per CATCH** - `CATCH 9, 11`  
✅ **Catch-all** - `CATCH` with no codes
✅ **THROW statement** - `THROW 5`
✅ **ERR function** - Returns error code
✅ **ERL function** - Returns error line
✅ **FINALLY block** - Always executes
✅ **Nested TRY** - Full support

## Error Codes

| Code | Error |
|------|-------|
| 5 | Illegal function call |
| 9 | Subscript out of range |
| 11 | Division by zero |
| 13 | Type mismatch |
| 53 | File not found |

## Examples

### Multiple error codes
```basic
TRY
    LET A%(100) = 42
CATCH 9, 13
    PRINT "Array or type error: "; ERR
END TRY
```

### With FINALLY
```basic
TRY
    OPEN "file.txt" FOR INPUT AS #1
CATCH 53
    PRINT "File not found"
FINALLY
    CLOSE #1
END TRY
```

### THROW statement
```basic
IF X% < 0 THEN THROW 5
```

### Nested TRY
```basic
TRY
    TRY
        LET X% = 1 / 0
    CATCH 11
        PRINT "Inner caught"
    END TRY
CATCH
    PRINT "Outer caught"
END TRY
```

## Implementation: 5 Phases, ~2 weeks

1. **Runtime** - Exception stack, basic_throw()
2. **Lexer** - TRY/CATCH/FINALLY/THROW tokens
3. **Parser** - Parse statements, build AST
4. **Codegen** - Generate QBE IL with setjmp/longjmp
5. **Testing** - Comprehensive test suite

## Documents

- `EXCEPTION_HANDLING_SIMPLE.md` - Full design (717 lines)
- `EXCEPTION_IMPLEMENTATION_PLAN.md` - Step-by-step plan (445 lines)
- This summary

## Next Step

Start Phase 1: Implement exception runtime in C.
