# Exception Handling Design - TRY/CATCH/FINALLY

## Overview

This document describes the design and implementation plan for adding structured exception handling to the FasterBASIC compiler. This is a modern feature not present in classic BASIC, but essential for robust error handling in contemporary programs.

## Motivation

Currently, the runtime uses `exit(1)` when errors occur (division by zero, array bounds violations, type mismatches, etc.). This makes it impossible to:
- Gracefully handle errors and continue execution
- Clean up resources when errors occur
- Provide user-friendly error messages
- Write robust, production-quality BASIC programs

Classic BASIC has `ON ERROR GOTO`, but it's unstructured and error-prone. Modern structured exception handling with TRY/CATCH/FINALLY provides:
- **TRY block** - Code that might fail
- **CATCH block** - Handle specific errors
- **FINALLY block** - Cleanup code that always runs
- Clear control flow and scoping

## Syntax

### Basic TRY/CATCH
```basic
TRY
    ' Code that might fail
    LET X% = 10 / 0
CATCH ERR
    ' Handle the error
    PRINT "Error occurred: "; ERR.MESSAGE
    PRINT "Error code: "; ERR.CODE
    PRINT "Error line: "; ERR.LINE
END TRY
```

### TRY/CATCH/FINALLY
```basic
TRY
    OPEN "file.txt" FOR INPUT AS #1
    ' Read from file
CATCH ERR
    PRINT "Failed to read file: "; ERR.MESSAGE
FINALLY
    ' Always executed, even if error occurs
    CLOSE #1
END TRY
```

### TRY/FINALLY (no CATCH)
```basic
TRY
    ' Allocate resources
    LET F% = OPENFILE("data.txt")
FINALLY
    ' Cleanup always happens
    CLOSEFILE(F%)
END TRY
```

### Multiple CATCH blocks (future enhancement)
```basic
TRY
    ' Code
CATCH ERR WHEN ERR.CODE = 11
    ' Handle division by zero
    PRINT "Cannot divide by zero"
CATCH ERR WHEN ERR.CODE = 9
    ' Handle array bounds
    PRINT "Array index out of bounds"
CATCH ERR
    ' Handle all other errors
    PRINT "Unknown error: "; ERR.MESSAGE
END TRY
```

### Nested TRY blocks
```basic
TRY
    PRINT "Outer try"
    TRY
        PRINT "Inner try"
        LET X% = 1 / 0
    CATCH E1
        PRINT "Caught in inner: "; E1.MESSAGE
    END TRY
CATCH E2
    PRINT "Caught in outer: "; E2.MESSAGE
END TRY
```

## Error Object Structure

The CATCH clause receives an error object with these fields:

```basic
TYPE ErrorInfo
    CODE AS INTEGER        ' Error code (11 = div by zero, 9 = bounds, etc.)
    MESSAGE AS STRING      ' Human-readable error message
    LINE AS INTEGER        ' Line number where error occurred
    DESCRIPTION AS STRING  ' Detailed description (optional)
END TYPE
```

### Standard Error Codes

| Code | Name | Description |
|------|------|-------------|
| 5 | Illegal Function Call | Invalid argument to function |
| 6 | Overflow | Numeric overflow |
| 9 | Subscript Out of Range | Array bounds violation |
| 11 | Division By Zero | Attempted division by zero |
| 13 | Type Mismatch | Incompatible types |
| 52 | Bad File Number | Invalid file handle |
| 53 | File Not Found | Cannot open file |
| 61 | Disk Full | Out of disk space |
| 62 | Input Past End | Read beyond end of file |
| 71 | Disk Not Ready | Drive not accessible |

## Implementation Strategy

### Phase 1: Runtime Exception Infrastructure

#### 1.1 Exception Context Structure

Add to `basic_runtime.h`:
```c
typedef struct BasicError {
    int32_t code;           // Error code
    char message[256];      // Error message
    int32_t line;           // Line number
    char description[512];  // Detailed description
} BasicError;

typedef struct ExceptionContext {
    jmp_buf jump_buffer;           // setjmp/longjmp buffer
    struct ExceptionContext* prev; // Previous context (for nesting)
    BasicError error;              // Error information
    int32_t has_finally;           // Whether this context has FINALLY
    void* finally_data;            // Data for FINALLY block
} ExceptionContext;
```

#### 1.2 Exception Stack

Add global exception context stack:
```c
// In basic_runtime.c
static ExceptionContext* g_exception_stack = NULL;

// Push new exception context
ExceptionContext* basic_exception_push(int32_t has_finally) {
    ExceptionContext* ctx = malloc(sizeof(ExceptionContext));
    ctx->prev = g_exception_stack;
    ctx->has_finally = has_finally;
    ctx->finally_data = NULL;
    ctx->error.code = 0;
    ctx->error.message[0] = '\0';
    ctx->error.line = g_current_line;
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

// Throw exception
void basic_throw(int32_t code, const char* message) {
    if (g_exception_stack) {
        // We have a handler - populate error and longjmp
        g_exception_stack->error.code = code;
        strncpy(g_exception_stack->error.message, message, 255);
        g_exception_stack->error.line = g_current_line;
        longjmp(g_exception_stack->jump_buffer, 1);
    } else {
        // No handler - fatal error
        fprintf(stderr, "Unhandled exception at line %d: %s (code %d)\n",
                g_current_line, message, code);
        exit(1);
    }
}

// Get current exception info
BasicError* basic_get_exception(void) {
    if (g_exception_stack) {
        return &g_exception_stack->error;
    }
    return NULL;
}
```

#### 1.3 Modify Error Functions

Replace all `exit(1)` calls with `basic_throw()`:

```c
// OLD:
void basic_error_msg(const char* message) {
    fprintf(stderr, "Runtime error: %s\n", message);
    exit(1);
}

// NEW:
void basic_error_msg(const char* message) {
    basic_throw(5, message); // 5 = Illegal function call
}

// OLD:
void basic_array_bounds_error(...) {
    snprintf(msg, sizeof(msg), "Array subscript out of bounds...");
    basic_error_msg(msg);
}

// NEW:
void basic_array_bounds_error(int64_t index, int64_t lower, int64_t upper) {
    char msg[256];
    snprintf(msg, sizeof(msg), 
             "Array subscript out of bounds: index %lld not in [%lld, %lld]",
             index, lower, upper);
    basic_throw(9, msg); // 9 = Subscript out of range
}
```

### Phase 2: Lexer Changes

Add new token types to `fasterbasic_lexer.h`:

```cpp
enum class TokenType {
    // ... existing tokens ...
    
    // Exception handling
    TRY,
    CATCH,
    FINALLY,
    END_TRY,    // or handle as END + TRY
    THROW,      // optional: explicit throw
    
    // ... rest of tokens ...
};
```

Update keyword table in `fasterbasic_lexer.cpp`:

```cpp
{"TRY", TokenType::TRY},
{"CATCH", TokenType::CATCH},
{"FINALLY", TokenType::FINALLY},
{"THROW", TokenType::THROW},
// END TRY is handled as END + TRY
```

### Phase 3: Parser Changes

#### 3.1 AST Node Structures

Add to parser header:

```cpp
// TRY/CATCH/FINALLY statement
class TryCatchStatement : public Statement {
public:
    std::vector<std::unique_ptr<Statement>> tryBlock;
    
    struct CatchClause {
        std::string errorVarName;                    // Variable name (e.g., "ERR")
        std::unique_ptr<Expression> condition;       // Optional WHEN clause
        std::vector<std::unique_ptr<Statement>> block;
    };
    std::vector<CatchClause> catchClauses;
    
    std::vector<std::unique_ptr<Statement>> finallyBlock;
    bool hasCatch;
    bool hasFinally;
    
    TryCatchStatement() : hasCatch(false), hasFinally(false) {}
};

// THROW statement (optional)
class ThrowStatement : public Statement {
public:
    std::unique_ptr<Expression> errorCode;    // Optional error code
    std::unique_ptr<Expression> errorMessage; // Optional message
};
```

#### 3.2 Parser Method

Add to `fasterbasic_parser.cpp`:

```cpp
std::unique_ptr<Statement> Parser::parseTryStatement() {
    advance(); // consume TRY
    
    auto stmt = std::make_unique<TryCatchStatement>();
    
    // Parse TRY block
    while (!check(TokenType::CATCH) && 
           !check(TokenType::FINALLY) && 
           !check(TokenType::END)) {
        if (isAtEnd()) {
            error("Unexpected end of file in TRY block");
            return nullptr;
        }
        auto s = parseStatement();
        if (s) stmt->tryBlock.push_back(std::move(s));
    }
    
    // Parse CATCH clause(s)
    while (check(TokenType::CATCH)) {
        advance(); // consume CATCH
        
        TryCatchStatement::CatchClause clause;
        
        // Get error variable name
        if (!check(TokenType::IDENTIFIER)) {
            error("Expected identifier after CATCH");
            return nullptr;
        }
        clause.errorVarName = current().value;
        advance();
        
        // Optional WHEN condition (future enhancement)
        if (match(TokenType::WHEN)) {
            clause.condition = parseExpression();
        }
        
        // Parse CATCH block
        while (!check(TokenType::CATCH) && 
               !check(TokenType::FINALLY) && 
               !check(TokenType::END)) {
            if (isAtEnd()) {
                error("Unexpected end of file in CATCH block");
                return nullptr;
            }
            auto s = parseStatement();
            if (s) clause.block.push_back(std::move(s));
        }
        
        stmt->catchClauses.push_back(std::move(clause));
        stmt->hasCatch = true;
    }
    
    // Parse FINALLY clause
    if (match(TokenType::FINALLY)) {
        stmt->hasFinally = true;
        
        while (!check(TokenType::END)) {
            if (isAtEnd()) {
                error("Unexpected end of file in FINALLY block");
                return nullptr;
            }
            auto s = parseStatement();
            if (s) stmt->finallyBlock.push_back(std::move(s));
        }
    }
    
    // Expect END TRY
    if (!match(TokenType::END)) {
        error("Expected END TRY");
        return nullptr;
    }
    if (!match(TokenType::TRY)) {
        error("Expected TRY after END");
        return nullptr;
    }
    
    // Validate: must have at least CATCH or FINALLY
    if (!stmt->hasCatch && !stmt->hasFinally) {
        error("TRY must have at least one CATCH or FINALLY clause");
        return nullptr;
    }
    
    return stmt;
}
```

### Phase 4: Semantic Analysis

Add to semantic analyzer:

```cpp
void SemanticAnalyzer::analyzeTryCatch(TryCatchStatement* stmt) {
    // Analyze TRY block
    for (auto& s : stmt->tryBlock) {
        analyzeStatement(s.get());
    }
    
    // Analyze CATCH blocks
    for (auto& clause : stmt->catchClauses) {
        // Create error variable in CATCH scope
        // Error variable is a UDT with CODE, MESSAGE, LINE fields
        VariableInfo errorVar;
        errorVar.name = clause.errorVarName;
        errorVar.type = VariableType::UDT; // Special error type
        errorVar.udtName = "ERROR_INFO";
        // ... add to symbol table
        
        // Analyze CATCH block
        for (auto& s : clause.block) {
            analyzeStatement(s.get());
        }
        
        // Remove error variable from scope
    }
    
    // Analyze FINALLY block
    for (auto& s : stmt->finallyBlock) {
        analyzeStatement(s.get());
    }
}
```

### Phase 5: QBE Code Generation

#### 5.1 TRY Block Structure

Generated QBE IL for:
```basic
TRY
    LET X% = 10 / Y%
CATCH ERR
    PRINT ERR.MESSAGE
FINALLY
    PRINT "Cleanup"
END TRY
```

Becomes (pseudocode):
```qbe
@block_start
    # Push exception context
    %ctx =l call $basic_exception_push(w 1)  # 1 = has FINALLY
    
    # setjmp - returns 0 on first call, non-zero if exception thrown
    %jmpval =w call $setjmp(l %ctx)
    %is_exception =w ceqw %jmpval, 0
    jnz %is_exception, @try_block, @catch_block

@try_block
    # Execute TRY block code
    %y =w loadw %y_addr
    %x =w divw 10, %y        # May throw division by zero
    storew %x, %x_addr
    
    # If we get here, no exception - jump to FINALLY
    jmp @finally_block

@catch_block
    # Get exception info
    %err_ptr =l call $basic_get_exception()
    
    # Access error fields for ERR variable
    %err_code_addr =l add %err_ptr, 0       # offset 0 = code
    %err_msg_addr =l add %err_ptr, 8        # offset 8 = message
    
    # Execute CATCH block code
    call $basic_print_string(l %err_msg_addr)
    call $basic_print_newline()
    
    # Fall through to FINALLY

@finally_block
    # Execute FINALLY block - ALWAYS runs
    %str_cleanup =l copy $string_literal_cleanup
    call $basic_print_string(l %str_cleanup)
    call $basic_print_newline()
    
    # Pop exception context
    call $basic_exception_pop()
    
    # Continue execution
    jmp @after_try

@after_try
    # Rest of program
```

#### 5.2 Code Generation Method

Add to `qbe_codegen_statements.cpp`:

```cpp
void QBECodeGenerator::emitTryCatch(const TryCatchStatement* stmt) {
    std::string try_label = generateLabel("try_block");
    std::string catch_label = generateLabel("catch_block");
    std::string finally_label = generateLabel("finally_block");
    std::string after_label = generateLabel("after_try");
    
    // Push exception context
    int has_finally = stmt->hasFinally ? 1 : 0;
    std::string ctx_temp = generateTemp();
    emitLine(ctx_temp + " =l call $basic_exception_push(w " + 
             std::to_string(has_finally) + ")");
    
    // setjmp - returns 0 first time, non-zero on exception
    std::string jmp_temp = generateTemp();
    emitLine(jmp_temp + " =w call $setjmp(l " + ctx_temp + ")");
    
    std::string is_zero_temp = generateTemp();
    emitLine(is_zero_temp + " =w ceqw " + jmp_temp + ", 0");
    emitLine("jnz " + is_zero_temp + ", @" + try_label + 
             ", @" + catch_label);
    
    // TRY block
    emitLabel(try_label);
    for (const auto& s : stmt->tryBlock) {
        emitStatement(s.get());
    }
    // If no exception, skip CATCH and go to FINALLY
    if (stmt->hasFinally) {
        emitLine("jmp @" + finally_label);
    } else {
        emitLine("jmp @" + after_label);
    }
    
    // CATCH block(s)
    if (stmt->hasCatch) {
        emitLabel(catch_label);
        
        for (const auto& clause : stmt->catchClauses) {
            // Get exception pointer
            std::string err_ptr = generateTemp();
            emitLine(err_ptr + " =l call $basic_get_exception()");
            
            // Store in symbol table for error variable access
            // The error variable is accessed like a UDT
            
            // Emit CATCH block statements
            for (const auto& s : clause.block) {
                emitStatement(s.get());
            }
        }
        
        // Fall through to FINALLY
        if (stmt->hasFinally) {
            emitLine("jmp @" + finally_label);
        } else {
            emitLine("jmp @" + after_label);
        }
    }
    
    // FINALLY block
    if (stmt->hasFinally) {
        emitLabel(finally_label);
        for (const auto& s : stmt->finallyBlock) {
            emitStatement(s.get());
        }
    }
    
    // Pop exception context
    emitLine("call $basic_exception_pop()");
    
    emitLabel(after_label);
}
```

### Phase 6: Runtime Implementation

#### 6.1 Add setjmp/longjmp wrapper

Since QBE IL needs to call setjmp, add wrapper:

```c
// In basic_runtime.c
#include <setjmp.h>

int basic_setjmp(jmp_buf* env) {
    return setjmp(*env);
}

void basic_longjmp(jmp_buf* env, int val) {
    longjmp(*env, val);
}
```

#### 6.2 Update all error-throwing code

Replace all `exit(1)` with `basic_throw(code, message)`:

```c
// Division by zero
if (divisor == 0) {
    basic_throw(11, "Division by zero");
}

// Array bounds
if (index < lower || index > upper) {
    basic_throw(9, "Subscript out of range");
}

// Type mismatch
if (expected_type != actual_type) {
    basic_throw(13, "Type mismatch");
}
```

### Phase 7: Testing

#### Test 1: Basic TRY/CATCH
```basic
REM test_exception_basic.bas
PRINT "Before TRY"
TRY
    PRINT "In TRY block"
    LET X% = 10 / 0
    PRINT "After division (should not print)"
CATCH ERR
    PRINT "Caught error: "; ERR.MESSAGE
    PRINT "Error code: "; ERR.CODE
END TRY
PRINT "After TRY/CATCH"
END
```

Expected output:
```
Before TRY
In TRY block
Caught error: Division by zero
Error code: 11
After TRY/CATCH
```

#### Test 2: TRY/FINALLY without exception
```basic
LET CLEANUP% = 0
TRY
    PRINT "Normal execution"
    LET X% = 10 / 2
    PRINT "X% = "; X%
FINALLY
    PRINT "FINALLY executed"
    LET CLEANUP% = 1
END TRY
PRINT "Cleanup flag: "; CLEANUP%
END
```

Expected output:
```
Normal execution
X% = 5
FINALLY executed
Cleanup flag: 1
```

#### Test 3: TRY/FINALLY with exception (uncaught)
```basic
TRY
    PRINT "About to fail"
    LET X% = 10 / 0
FINALLY
    PRINT "FINALLY executes even on error"
END TRY
PRINT "This should not print"
END
```

Expected output:
```
About to fail
FINALLY executes even on error
Unhandled exception at line XX: Division by zero (code 11)
```

#### Test 4: Nested TRY blocks
```basic
TRY
    PRINT "Outer TRY"
    TRY
        PRINT "Inner TRY"
        LET X% = 1 / 0
    CATCH E1
        PRINT "Inner CATCH: "; E1.MESSAGE
    END TRY
    PRINT "Between TRY blocks"
CATCH E2
    PRINT "Outer CATCH: "; E2.MESSAGE
END TRY
PRINT "Done"
END
```

Expected output:
```
Outer TRY
Inner TRY
Inner CATCH: Division by zero
Between TRY blocks
Done
```

#### Test 5: Array bounds exception
```basic
DIM A%(10)
TRY
    LET A%(20) = 42
CATCH ERR
    PRINT "Array error: "; ERR.MESSAGE
    PRINT "Code: "; ERR.CODE
END TRY
END
```

Expected output:
```
Array error: Subscript out of range
Code: 9
```

## Implementation Phases

### Phase 1: Foundation (Week 1)
- [ ] Implement runtime exception infrastructure
- [ ] Add setjmp/longjmp support
- [ ] Update all error functions to use basic_throw()
- [ ] Test basic exception throwing/catching in C

### Phase 2: Parser (Week 2)
- [ ] Add TRY/CATCH/FINALLY/END TRY tokens
- [ ] Implement parser for TRY statements
- [ ] Add AST nodes
- [ ] Test parsing with sample programs

### Phase 3: Semantic Analysis (Week 3)
- [ ] Validate TRY/CATCH/FINALLY structure
- [ ] Handle error variable scoping
- [ ] Test semantic validation

### Phase 4: Code Generation (Week 4)
- [ ] Implement QBE IL generation for TRY blocks
- [ ] Implement CATCH block generation
- [ ] Implement FINALLY block generation
- [ ] Test generated QBE IL

### Phase 5: Integration Testing (Week 5)
- [ ] Create comprehensive test suite
- [ ] Test all error types
- [ ] Test nested TRY blocks
- [ ] Test TRY/CATCH/FINALLY combinations
- [ ] Performance testing

### Phase 6: Documentation (Week 6)
- [ ] Update language reference
- [ ] Create exception handling guide
- [ ] Add examples to START_HERE.md
- [ ] Document error codes

## Alternative: ON ERROR GOTO (Classic BASIC)

If structured exception handling is too complex initially, consider implementing classic `ON ERROR GOTO` first:

```basic
ON ERROR GOTO 1000
LET X% = 10 / 0
PRINT "This won't print"
END

1000 REM Error handler
PRINT "Error occurred: "; ERR; " at line "; ERL
RESUME NEXT
```

This is simpler but less structured. Could be implemented as a stepping stone.

## Open Questions

1. **Should we support THROW statement** for user code to throw exceptions?
2. **Error object lifetime** - How long does the error object live?
3. **RESUME statement** - Should we support RESUME/RESUME NEXT like classic BASIC?
4. **Multiple CATCH clauses** - Implement immediately or defer?
5. **Performance** - What's the overhead of setjmp/longjmp?
6. **Async operations** - How do exceptions interact with callbacks?

## References

- Visual Basic .NET exception handling
- Python try/except/finally
- C++ try/catch
- setjmp/longjmp in C (POSIX)
- Classic BASIC ON ERROR GOTO

## Success Criteria

1. All runtime errors can be caught and handled
2. FINALLY blocks always execute
3. Nested TRY blocks work correctly
4. No memory leaks in exception handling
5. Clear error messages and codes
6. Comprehensive test coverage (>90%)
7. Documentation complete
8. Performance impact < 5% when no exceptions thrown

---

**Status**: Design phase
**Next Steps**: Review design, gather feedback, start Phase 1 implementation