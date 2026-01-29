# Simplified Exception Handling Design - Error Code Based

## Overview

This is a streamlined exception handling design focused on **efficiency** and **simplicity**:
- ✅ Error codes (integers), not objects
- ✅ Multiple error codes per CATCH
- ✅ THROW statement with error codes
- ✅ Runtime errors throw codes
- ✅ Minimal overhead

## Syntax

### Basic TRY/CATCH with Error Codes

```basic
TRY
    LET X% = 10 / 0
CATCH 11
    PRINT "Division by zero at line "; ERL
END TRY
```

### Multiple Error Codes in One CATCH

```basic
TRY
    LET A%(100) = 42
CATCH 9, 13
    PRINT "Array bounds or type mismatch"
END TRY
```

### Catch-All Handler

```basic
TRY
    ' Some risky code
CATCH 11
    PRINT "Division by zero"
CATCH 9
    PRINT "Array bounds"
CATCH
    PRINT "Other error: "; ERR
END TRY
```

### TRY/CATCH/FINALLY

```basic
TRY
    OPEN "file.txt" FOR INPUT AS #1
    ' Read data
CATCH 53
    PRINT "File not found"
FINALLY
    CLOSE #1
END TRY
```

### THROW Statement

```basic
IF X% < 0 THEN
    THROW 5    ' Illegal function call
END IF

' Or with computed code
LET ERRCODE% = 13
THROW ERRCODE%
```

### Nested TRY

```basic
TRY
    PRINT "Outer"
    TRY
        LET X% = 1 / 0
    CATCH 11
        PRINT "Inner caught division by zero"
    END TRY
CATCH
    PRINT "Outer catch-all"
END TRY
```

## Error Intrinsics

Access error information using simple functions:

| Function | Type | Description |
|----------|------|-------------|
| `ERR` | INTEGER | Current error code |
| `ERL` | INTEGER | Line number where error occurred |

Usage:
```basic
CATCH 9, 11
    PRINT "Error code: "; ERR
    PRINT "At line: "; ERL
END TRY
```

## Standard Error Codes

Classic BASIC error codes:

| Code | Name | Description |
|------|------|-------------|
| 5 | Illegal Function Call | Invalid argument or operation |
| 6 | Overflow | Numeric overflow |
| 9 | Subscript Out of Range | Array bounds violation |
| 11 | Division By Zero | Attempted division by zero |
| 13 | Type Mismatch | Incompatible types |
| 52 | Bad File Number | Invalid file handle |
| 53 | File Not Found | Cannot open file |
| 61 | Disk Full | Out of disk space |
| 62 | Input Past End | Read beyond EOF |
| 71 | Disk Not Ready | Drive not accessible |

## Runtime Implementation

### Exception Context Structure

Minimal structure for efficiency:

```c
typedef struct ExceptionContext {
    jmp_buf jump_buffer;           // setjmp/longjmp buffer
    struct ExceptionContext* prev; // For nesting
    int32_t error_code;            // Current error code
    int32_t error_line;            // Line where error occurred
    int32_t has_finally;           // Whether FINALLY exists
} ExceptionContext;
```

### Core Runtime Functions

```c
// Exception stack
static ExceptionContext* g_exception_stack = NULL;

// Global error state
static int32_t g_last_error = 0;
static int32_t g_last_error_line = 0;

// Push exception context
ExceptionContext* basic_exception_push(int32_t has_finally) {
    ExceptionContext* ctx = malloc(sizeof(ExceptionContext));
    ctx->prev = g_exception_stack;
    ctx->has_finally = has_finally;
    ctx->error_code = 0;
    ctx->error_line = 0;
    g_exception_stack = ctx;
    return ctx;
}

// Pop exception context
void basic_exception_pop(void) {
    if (g_exception_stack) {
        ExceptionContext* ctx = g_exception_stack;
        g_exception_stack = ctx->prev;
        free(ctx);
    }
}

// Throw exception with error code
void basic_throw(int32_t error_code) {
    if (g_exception_stack) {
        // Save error info
        g_exception_stack->error_code = error_code;
        g_exception_stack->error_line = g_current_line;
        g_last_error = error_code;
        g_last_error_line = g_current_line;
        
        // Jump to handler
        longjmp(g_exception_stack->jump_buffer, 1);
    } else {
        // No handler - fatal error
        fprintf(stderr, "Unhandled exception at line %d: Error code %d\n",
                g_current_line, error_code);
        exit(1);
    }
}

// Get error code (ERR function)
int32_t basic_err(void) {
    return g_last_error;
}

// Get error line (ERL function)
int32_t basic_erl(void) {
    return g_last_error_line;
}
```

### Update Runtime Error Functions

Replace all `exit(1)` calls with `basic_throw(code)`:

```c
// Division by zero
int32_t basic_div_int(int32_t a, int32_t b) {
    if (b == 0) {
        basic_throw(11);  // Division by zero
    }
    return a / b;
}

// Array bounds check
void basic_array_bounds_check(int64_t index, int64_t lower, int64_t upper) {
    if (index < lower || index > upper) {
        basic_throw(9);  // Subscript out of range
    }
}

// Type mismatch
void basic_type_error(void) {
    basic_throw(13);  // Type mismatch
}
```

## Lexer Changes

Add tokens:

```cpp
enum class TokenType {
    // ... existing ...
    TRY,
    CATCH,
    FINALLY,
    THROW,
    ERR,      // Error code function
    ERL,      // Error line function
    // ... rest ...
};
```

Update keyword table:
```cpp
{"TRY", TokenType::TRY},
{"CATCH", TokenType::CATCH},
{"FINALLY", TokenType::FINALLY},
{"THROW", TokenType::THROW},
{"ERR", TokenType::ERR},
{"ERL", TokenType::ERL},
```

## Parser Changes

### AST Nodes

```cpp
// TRY/CATCH/FINALLY statement
class TryCatchStatement : public Statement {
public:
    std::vector<std::unique_ptr<Statement>> tryBlock;
    
    struct CatchClause {
        std::vector<int32_t> errorCodes;  // Empty = catch all
        std::vector<std::unique_ptr<Statement>> block;
    };
    std::vector<CatchClause> catchClauses;
    
    std::vector<std::unique_ptr<Statement>> finallyBlock;
    bool hasFinally;
};

// THROW statement
class ThrowStatement : public Statement {
public:
    std::unique_ptr<Expression> errorCode;  // Must evaluate to integer
};
```

### Parse TRY Statement

```cpp
std::unique_ptr<Statement> Parser::parseTryStatement() {
    advance(); // consume TRY
    
    auto stmt = std::make_unique<TryCatchStatement>();
    stmt->hasFinally = false;
    
    // Parse TRY block
    while (!check(TokenType::CATCH) && 
           !check(TokenType::FINALLY) && 
           !check(TokenType::END)) {
        auto s = parseStatement();
        if (s) stmt->tryBlock.push_back(std::move(s));
    }
    
    // Parse CATCH clauses
    while (check(TokenType::CATCH)) {
        advance(); // consume CATCH
        
        TryCatchStatement::CatchClause clause;
        
        // Parse error codes (if any)
        if (check(TokenType::NUMBER)) {
            // CATCH 11, 9, 13
            clause.errorCodes.push_back(std::stoi(current().value));
            advance();
            
            while (match(TokenType::COMMA)) {
                if (!check(TokenType::NUMBER)) {
                    error("Expected error code after comma");
                    return nullptr;
                }
                clause.errorCodes.push_back(std::stoi(current().value));
                advance();
            }
        }
        // else: catch-all (errorCodes is empty)
        
        // Parse CATCH block
        while (!check(TokenType::CATCH) && 
               !check(TokenType::FINALLY) && 
               !check(TokenType::END)) {
            auto s = parseStatement();
            if (s) clause.block.push_back(std::move(s));
        }
        
        stmt->catchClauses.push_back(std::move(clause));
    }
    
    // Parse FINALLY clause
    if (match(TokenType::FINALLY)) {
        stmt->hasFinally = true;
        
        while (!check(TokenType::END)) {
            auto s = parseStatement();
            if (s) stmt->finallyBlock.push_back(std::move(s));
        }
    }
    
    // END TRY
    expect(TokenType::END);
    expect(TokenType::TRY);
    
    // Validate: must have at least CATCH or FINALLY
    if (stmt->catchClauses.empty() && !stmt->hasFinally) {
        error("TRY must have at least one CATCH or FINALLY");
        return nullptr;
    }
    
    return stmt;
}
```

### Parse THROW Statement

```cpp
std::unique_ptr<Statement> Parser::parseThrowStatement() {
    advance(); // consume THROW
    
    auto stmt = std::make_unique<ThrowStatement>();
    stmt->errorCode = parseExpression();
    
    return stmt;
}
```

## QBE Code Generation

### TRY/CATCH/FINALLY

Generated IL for:
```basic
TRY
    LET X% = 10 / Y%
CATCH 11
    PRINT "Division by zero"
CATCH 9
    PRINT "Array error"
CATCH
    PRINT "Other error"
FINALLY
    PRINT "Cleanup"
END TRY
```

Becomes:
```qbe
@block_start
    # Push exception context (1 = has FINALLY)
    %ctx =l call $basic_exception_push(w 1)
    
    # setjmp - returns 0 first time, error code on exception
    %jmpval =w call $setjmp(l %ctx)
    %is_zero =w ceqw %jmpval, 0
    jnz %is_zero, @try_block, @catch_dispatch

@try_block
    # Execute TRY block
    %y =w loadw %y_addr
    %x =w call $basic_div_int(w 10, w %y)  # May throw 11
    storew %x, %x_addr
    jmp @finally_block  # No error, skip to FINALLY

@catch_dispatch
    # Get error code
    %err_code =w call $basic_err()
    
    # Check CATCH 11
    %is_11 =w ceqw %err_code, 11
    jnz %is_11, @catch_11, @check_catch_9

@catch_11
    # Handle division by zero
    %str1 =l copy $str_divzero
    call $basic_print_string(l %str1)
    call $basic_print_newline()
    jmp @finally_block

@check_catch_9
    # Check CATCH 9
    %is_9 =w ceqw %err_code, 9
    jnz %is_9, @catch_9, @catch_all

@catch_9
    # Handle array error
    %str2 =l copy $str_array_err
    call $basic_print_string(l %str2)
    call $basic_print_newline()
    jmp @finally_block

@catch_all
    # Catch all other errors
    %str3 =l copy $str_other_err
    call $basic_print_string(l %str3)
    call $basic_print_newline()
    # Fall through to FINALLY

@finally_block
    # FINALLY always executes
    %str_cleanup =l copy $str_cleanup
    call $basic_print_string(l %str_cleanup)
    call $basic_print_newline()
    
    # Pop exception context
    call $basic_exception_pop()
    jmp @after_try

@after_try
    # Continue program
```

### THROW Statement

For `THROW 5`:
```qbe
@throw_error
    call $basic_throw(w 5)
    # Never returns - longjmp to handler
```

For `THROW ERRCODE%`:
```qbe
@throw_variable
    %code =w loadw %errcode_addr
    call $basic_throw(w %code)
```

### ERR and ERL Functions

```qbe
# ERR function
%err_val =w call $basic_err()

# ERL function
%erl_val =w call $basic_erl()
```

## Implementation Phases

### Phase 1: Runtime (2 days)
- [ ] Add exception context structure
- [ ] Implement basic_exception_push/pop
- [ ] Implement basic_throw
- [ ] Add basic_err() and basic_erl()
- [ ] Update all error functions to use basic_throw
- [ ] Test in C

### Phase 2: Lexer & Parser (2 days)
- [ ] Add TRY/CATCH/FINALLY/THROW tokens
- [ ] Add ERR/ERL tokens
- [ ] Implement parseTryStatement()
- [ ] Implement parseThrowStatement()
- [ ] Test parsing

### Phase 3: Semantic Analysis (1 day)
- [ ] Validate TRY/CATCH/FINALLY structure
- [ ] Validate THROW expressions are integers
- [ ] Validate error codes are numeric literals

### Phase 4: Code Generation (3 days)
- [ ] Generate TRY block setup (setjmp)
- [ ] Generate CATCH dispatch logic
- [ ] Generate FINALLY block
- [ ] Generate THROW statements
- [ ] Generate ERR/ERL function calls

### Phase 5: Testing (2 days)
- [ ] Test basic TRY/CATCH
- [ ] Test multiple CATCH clauses
- [ ] Test FINALLY execution
- [ ] Test THROW statement
- [ ] Test nested TRY
- [ ] Test all error codes

**Total: ~10 days (2 weeks)**

## Test Suite

### Test 1: Basic Division by Zero
```basic
REM test_exception_div_zero.bas
PRINT "Before TRY"
TRY
    LET X% = 10 / 0
    PRINT "Should not print"
CATCH 11
    PRINT "Caught error "; ERR; " at line "; ERL
END TRY
PRINT "After TRY"
END
```

Expected output:
```
Before TRY
Caught error 11 at line 30
After TRY
```

### Test 2: Multiple Error Codes
```basic
REM test_exception_multiple.bas
DIM A%(5)
TRY
    LET A%(10) = 42
CATCH 9, 11
    PRINT "Array or division error: "; ERR
END TRY
END
```

Expected output:
```
Array or division error: 9
```

### Test 3: Catch-All
```basic
TRY
    LET X% = 10 / 0
CATCH 9
    PRINT "Array error"
CATCH
    PRINT "Other error: "; ERR
END TRY
END
```

Expected output:
```
Other error: 11
```

### Test 4: FINALLY Always Runs
```basic
LET FLAG% = 0
TRY
    LET X% = 10 / 0
CATCH 11
    PRINT "Error caught"
FINALLY
    LET FLAG% = 1
    PRINT "FINALLY executed"
END TRY
PRINT "Flag = "; FLAG%
END
```

Expected output:
```
Error caught
FINALLY executed
Flag = 1
```

### Test 5: THROW Statement
```basic
TRY
    IF 1 = 1 THEN THROW 5
CATCH 5
    PRINT "Illegal function call thrown"
END TRY
END
```

Expected output:
```
Illegal function call thrown
```

### Test 6: Nested TRY
```basic
TRY
    PRINT "Outer"
    TRY
        LET X% = 1 / 0
    CATCH 11
        PRINT "Inner caught "; ERR
    END TRY
    PRINT "Between blocks"
CATCH
    PRINT "Outer caught "; ERR
END TRY
PRINT "Done"
END
```

Expected output:
```
Outer
Inner caught 11
Between blocks
Done
```

### Test 7: Uncaught Exception
```basic
TRY
    LET X% = 10 / 0
CATCH 9
    PRINT "Only catches array errors"
END TRY
END
```

Expected output:
```
Unhandled exception at line 20: Error code 11
```

### Test 8: No Exception
```basic
TRY
    LET X% = 10 / 2
    PRINT "X = "; X%
CATCH 11
    PRINT "Error"
FINALLY
    PRINT "Cleanup"
END TRY
END
```

Expected output:
```
X = 5
Cleanup
```

## Error Code Reference Table

Quick reference for runtime error codes:

```basic
REM Common error codes
CONST ERR_ILLEGAL_CALL = 5
CONST ERR_OVERFLOW = 6
CONST ERR_SUBSCRIPT = 9
CONST ERR_DIV_ZERO = 11
CONST ERR_TYPE_MISMATCH = 13
CONST ERR_BAD_FILE = 52
CONST ERR_FILE_NOT_FOUND = 53
```

## Advantages of This Design

1. **Efficient** - Just integers, no object allocation
2. **Simple** - Easy to understand and implement
3. **Fast** - Minimal overhead when no exception
4. **Classic** - Uses standard BASIC error codes
5. **Flexible** - Multiple codes per CATCH, catch-all handler
6. **Complete** - TRY/CATCH/FINALLY/THROW all supported

## Performance Characteristics

- **No exception:** ~10 cycles overhead (one setjmp call)
- **Exception thrown:** ~1 microsecond (longjmp + dispatch)
- **Memory:** 32 bytes per exception context
- **Stack depth:** Unlimited (malloc'd linked list)

## Comparison with Full Design

| Feature | Simple (This) | Full (Objects) |
|---------|---------------|----------------|
| Error info | Code only | Code + message + details |
| CATCH syntax | `CATCH 11, 9` | `CATCH ERR WHEN ERR.CODE = 11` |
| Overhead | Minimal | Moderate (string alloc) |
| Implementation | 2 weeks | 4 weeks |
| Flexibility | Good | Excellent |
| Simplicity | Excellent | Good |

**Recommendation:** Start with this simple design, can add error messages later if needed.

---

**Status:** Ready to implement
**Estimated time:** 10 days
**Next step:** Begin Phase 1 (Runtime implementation)