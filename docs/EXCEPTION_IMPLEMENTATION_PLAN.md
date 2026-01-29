# Exception Handling Implementation Plan - Simplified Design

## Quick Recap

We're implementing **simple, efficient exception handling** using error codes (not objects):

### Key Features
- ✅ **Error codes only** - Integers (5, 9, 11, 13, etc.)
- ✅ **Multiple codes per CATCH** - `CATCH 9, 11` catches array bounds OR division by zero
- ✅ **THROW statement** - `THROW 5` to throw error codes
- ✅ **Runtime errors throw codes** - Division by zero, array bounds, etc.
- ✅ **ERR and ERL functions** - Get error code and line number
- ✅ **FINALLY support** - Cleanup code that always runs
- ✅ **Nested TRY blocks** - Full nesting support

### Syntax Examples

```basic
' Basic TRY/CATCH
TRY
    LET X% = 10 / 0
CATCH 11
    PRINT "Division by zero at line "; ERL
END TRY

' Multiple error codes
TRY
    LET A%(100) = 42
CATCH 9, 13
    PRINT "Array bounds or type error"
END TRY

' Catch-all handler
TRY
    ' Risky code
CATCH 11
    PRINT "Division by zero"
CATCH 9
    PRINT "Array bounds"
CATCH
    PRINT "Other error: "; ERR
END TRY

' With FINALLY
TRY
    OPEN "file.txt" FOR INPUT AS #1
CATCH 53
    PRINT "File not found"
FINALLY
    CLOSE #1
END TRY

' THROW statement
IF X% < 0 THEN THROW 5    ' Illegal function call
```

---

## Implementation Steps

### Phase 1: Runtime Foundation (2 days)

**Files to modify:**
- `fsh/FasterBASICT/runtime_c/basic_runtime.h`
- `fsh/FasterBASICT/runtime_c/basic_runtime.c`

**Tasks:**

1. **Add exception context structure** to `basic_runtime.h`:
```c
#include <setjmp.h>

typedef struct ExceptionContext {
    jmp_buf jump_buffer;
    struct ExceptionContext* prev;
    int32_t error_code;
    int32_t error_line;
    int32_t has_finally;
} ExceptionContext;
```

2. **Add global exception stack** to `basic_runtime.c`:
```c
static ExceptionContext* g_exception_stack = NULL;
static int32_t g_last_error = 0;
static int32_t g_last_error_line = 0;
```

3. **Implement exception functions**:
```c
ExceptionContext* basic_exception_push(int32_t has_finally);
void basic_exception_pop(void);
void basic_throw(int32_t error_code);
int32_t basic_err(void);
int32_t basic_erl(void);
```

4. **Replace all `exit(1)` calls** with `basic_throw(code)`:
   - Division by zero → `basic_throw(11)`
   - Array bounds → `basic_throw(9)`
   - Type mismatch → `basic_throw(13)`
   - File errors → `basic_throw(52)` or `basic_throw(53)`

**Testing:** Write C unit tests to verify exception stack works.

---

### Phase 2: Lexer Changes (0.5 days)

**Files to modify:**
- `fsh/FasterBASICT/src/fasterbasic_lexer.h`
- `fsh/FasterBASICT/src/fasterbasic_lexer.cpp`

**Tasks:**

1. **Add token types** to enum:
```cpp
TRY,
CATCH,
FINALLY,
THROW,
ERR,      // Error code function
ERL,      // Error line function
```

2. **Add keywords** to keyword table:
```cpp
{"TRY", TokenType::TRY},
{"CATCH", TokenType::CATCH},
{"FINALLY", TokenType::FINALLY},
{"THROW", TokenType::THROW},
{"ERR", TokenType::ERR},
{"ERL", TokenType::ERL},
```

**Testing:** Verify tokens are recognized correctly.

---

### Phase 3: Parser Changes (2 days)

**Files to modify:**
- `fsh/FasterBASICT/src/fasterbasic_parser.h`
- `fsh/FasterBASICT/src/fasterbasic_parser.cpp`

**Tasks:**

1. **Add AST node classes**:
```cpp
class TryCatchStatement : public Statement {
public:
    std::vector<std::unique_ptr<Statement>> tryBlock;
    
    struct CatchClause {
        std::vector<int32_t> errorCodes;  // Empty = catch-all
        std::vector<std::unique_ptr<Statement>> block;
    };
    std::vector<CatchClause> catchClauses;
    
    std::vector<std::unique_ptr<Statement>> finallyBlock;
    bool hasFinally;
};

class ThrowStatement : public Statement {
public:
    std::unique_ptr<Expression> errorCode;
};
```

2. **Implement parser methods**:
   - `parseTryStatement()` - Parse TRY/CATCH/FINALLY/END TRY
   - `parseThrowStatement()` - Parse THROW <expression>

3. **Add to statement parsing switch**:
```cpp
case TokenType::TRY:
    return parseTryStatement();
case TokenType::THROW:
    return parseThrowStatement();
```

4. **Handle ERR and ERL as intrinsic functions** in expression parsing.

**Testing:** Parse sample programs and verify AST structure.

---

### Phase 4: Semantic Analysis (1 day)

**Files to modify:**
- `fsh/FasterBASICT/src/fasterbasic_semantic.cpp`

**Tasks:**

1. **Validate TRY/CATCH/FINALLY structure**:
   - Must have at least one CATCH or FINALLY
   - Error codes must be numeric literals
   - Catch-all (no codes) must be last CATCH

2. **Validate THROW statement**:
   - Expression must evaluate to INTEGER
   
3. **Register ERR and ERL as intrinsic functions**:
   - Return type: INTEGER

**Testing:** Test semantic validation with valid and invalid programs.

---

### Phase 5: QBE Code Generation (3 days)

**Files to modify:**
- `fsh/FasterBASICT/src/codegen/qbe_codegen_statements.cpp`
- `fsh/FasterBASICT/src/codegen/qbe_codegen_expressions.cpp`

**Tasks:**

1. **Implement `emitTryCatch()`**:
```cpp
void QBECodeGenerator::emitTryCatch(const TryCatchStatement* stmt) {
    // 1. Push exception context
    // 2. Call setjmp
    // 3. Branch: 0 = try block, non-zero = catch dispatch
    // 4. Generate TRY block
    // 5. Generate CATCH dispatch (check error codes)
    // 6. Generate each CATCH block
    // 7. Generate FINALLY block
    // 8. Pop exception context
}
```

2. **Implement `emitThrow()`**:
```cpp
void QBECodeGenerator::emitThrow(const ThrowStatement* stmt) {
    // Evaluate error code expression
    // Call basic_throw(code)
}
```

3. **Handle ERR and ERL intrinsics**:
```cpp
case IntrinsicFunction::ERR:
    return "%temp =w call $basic_err()";
case IntrinsicFunction::ERL:
    return "%temp =w call $basic_erl()";
```

**Testing:** 
- Inspect generated QBE IL manually
- Compile and run simple test cases

---

### Phase 6: Integration Testing (2 days)

**Files to create:**
- `tests/exceptions/test_exception_basic.bas`
- `tests/exceptions/test_exception_multiple_catch.bas`
- `tests/exceptions/test_exception_finally.bas`
- `tests/exceptions/test_exception_throw.bas`
- `tests/exceptions/test_exception_nested.bas`
- `tests/exceptions/test_exception_uncaught.bas`

**Test categories:**

1. **Basic exception handling**
   - Division by zero
   - Array bounds
   - Type mismatch

2. **Multiple CATCH clauses**
   - Multiple error codes in one CATCH
   - Multiple CATCH blocks
   - Catch-all handler

3. **FINALLY block**
   - FINALLY without exception
   - FINALLY with exception
   - FINALLY always executes

4. **THROW statement**
   - Throw literal code
   - Throw computed code
   - User-thrown exceptions

5. **Nested TRY blocks**
   - Inner catches exception
   - Outer catches exception
   - Both have FINALLY

6. **Uncaught exceptions**
   - No matching CATCH
   - Fatal error behavior

7. **ERR and ERL functions**
   - Access in CATCH block
   - Correct values

**Update test_basic_suite.sh** to include exception tests.

---

## Error Code Reference

Standard BASIC error codes to implement:

| Code | Constant Name | Description | Thrown By |
|------|---------------|-------------|-----------|
| 5 | ERR_ILLEGAL_CALL | Illegal function call | Invalid arguments |
| 6 | ERR_OVERFLOW | Overflow | Numeric overflow |
| 9 | ERR_SUBSCRIPT | Subscript out of range | Array bounds check |
| 11 | ERR_DIV_ZERO | Division by zero | Division/MOD operators |
| 13 | ERR_TYPE_MISMATCH | Type mismatch | Type conversions |
| 52 | ERR_BAD_FILE | Bad file number | File I/O |
| 53 | ERR_FILE_NOT_FOUND | File not found | OPEN statement |

---

## Runtime Modifications Checklist

Files that currently call `exit(1)` and need to throw error codes:

- [ ] `basic_runtime.c` - Memory allocation errors → 6 (overflow)
- [ ] `basic_data.c` - DATA/READ type mismatches → 13
- [ ] `array_ops.c` - Array bounds → 9
- [ ] `math_ops.c` - Division by zero → 11
- [ ] `io_ops.c` - File errors → 52, 53
- [ ] `string_ops.c` - String operation errors → 5

---

## QBE IL Pattern

Standard pattern for TRY/CATCH generation:

```qbe
# Push exception context
%ctx =l call $basic_exception_push(w has_finally)

# setjmp (returns 0 first time, 1 on exception)
%jmp =w call $setjmp(l %ctx)
%is_zero =w ceqw %jmp, 0
jnz %is_zero, @try_block, @catch_dispatch

@try_block
    # TRY block code here
    jmp @finally_or_after

@catch_dispatch
    %err =w call $basic_err()
    
    # Check each CATCH clause
    %match1 =w ceqw %err, 11
    jnz %match1, @catch_1, @check_next
    
@catch_1
    # CATCH block code
    jmp @finally_or_after
    
@check_next
    # More CATCH checks...
    
@finally_or_after
    # FINALLY block (if exists)
    call $basic_exception_pop()
    # Continue
```

---

## Documentation Updates

After implementation:

1. **Update START_HERE.md**
   - Add exception handling section
   - Include example programs
   - Update test count

2. **Create EXCEPTION_HANDLING_GUIDE.md**
   - Full language reference
   - All error codes documented
   - Best practices
   - Migration guide

3. **Update README.md**
   - Mention exception handling feature
   - Link to guide

---

## Timeline

| Phase | Days | Deliverable |
|-------|------|-------------|
| 1. Runtime | 2 | Exception functions implemented |
| 2. Lexer | 0.5 | Tokens recognized |
| 3. Parser | 2 | AST nodes and parsing complete |
| 4. Semantic | 1 | Validation implemented |
| 5. Codegen | 3 | QBE IL generation working |
| 6. Testing | 2 | Full test suite passing |
| **TOTAL** | **10.5 days** | **Feature complete** |

---

## Success Criteria

- [ ] All runtime errors throw exception codes
- [ ] TRY/CATCH/FINALLY syntax parses correctly
- [ ] Multiple error codes per CATCH work
- [ ] THROW statement works
- [ ] ERR and ERL functions return correct values
- [ ] FINALLY always executes
- [ ] Nested TRY blocks work
- [ ] Uncaught exceptions terminate gracefully
- [ ] No memory leaks in exception handling
- [ ] Test suite: 15+ tests, 100% pass rate
- [ ] Documentation complete
- [ ] Zero breaking changes to existing code

---

## First Steps (Start Here)

1. **Create feature branch**
   ```bash
   git checkout -b feature/exception-handling
   ```

2. **Start with runtime implementation**
   - Open `fsh/FasterBASICT/runtime_c/basic_runtime.h`
   - Add exception context structure
   - Implement exception functions
   - Test in isolation

3. **Update one error function as proof-of-concept**
   - Pick division by zero (simple case)
   - Replace `exit(1)` with `basic_throw(11)`
   - Test without TRY/CATCH (should still exit)

4. **Continue with lexer, parser, etc.**

---

**Next action:** Begin Phase 1 - Runtime implementation