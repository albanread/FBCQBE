# Exception Handling and Dynamic Array Implementation Summary

## Document Purpose

This document provides a high-level summary of the exception handling (TRY/CATCH/FINALLY/THROW) and dynamic array memory operations (ERASE/REDIM/REDIM PRESERVE) implementation in the FasterBASIC → QBE compiler pipeline. For detailed technical information, see [CRITICAL_IMPLEMENTATION_NOTES.md](CRITICAL_IMPLEMENTATION_NOTES.md).

---

## Table of Contents

1. [Overview](#overview)
2. [Exception Handling](#exception-handling)
3. [Dynamic Array Operations](#dynamic-array-operations)
4. [Test Coverage](#test-coverage)
5. [Key Fixes Applied](#key-fixes-applied)
6. [Validation Status](#validation-status)

---

## Overview

Both exception handling and dynamic array operations are now fully implemented and validated end-to-end:

- **Parser** → **Semantic Analyzer** → **CFG** → **QBE IL Codegen** → **Runtime** → **Native Assembly**

All tests pass on macOS ARM64 and x86_64. The implementation follows traditional BASIC semantics while leveraging modern C runtime facilities (setjmp/longjmp for exceptions, malloc/realloc/free for arrays).

---

## Exception Handling

### Language Constructs

```basic
' TRY/CATCH with specific error codes
TRY
    ' Protected code
    IF condition THEN THROW 10, 42
CATCH 5
    PRINT "Caught error 5"
CATCH 10
    PRINT "Caught error 10"
FINALLY
    PRINT "Cleanup always runs"
END TRY

' Catch-all handler
TRY
    ' Protected code
CATCH
    PRINT "Caught any error: "; ERR(); " at line "; ERL()
END TRY
```

### Implementation Architecture

1. **Runtime Functions**
   - `basic_exception_push(ExceptionContext*)` — Push exception context onto stack
   - `basic_exception_pop()` — Pop context from stack
   - `basic_throw(int errorCode, int lineNumber)` — Throw exception
   - `basic_err()` — Get current error code (ERR() builtin)
   - `basic_erl()` — Get current error line number (ERL() builtin)
   - `basic_rethrow()` — Rethrow current exception when no CATCH matches

2. **Codegen Flow**
   - Allocate `ExceptionContext` on stack (256 bytes for jmp_buf + metadata)
   - Push context onto runtime exception stack
   - **Call setjmp directly** (not through wrapper) with jmp_buf pointer
   - Immediately branch based on setjmp return value:
     - 0 → Normal execution (enter TRY block)
     - Non-zero → Exception occurred (enter dispatch block)
   - Dispatch block compares `ERR()` to CATCH error codes and jumps to matching handler
   - CATCH blocks pop exception context and execute handler code
   - FINALLY block always executes on both normal and exceptional paths
   - Unmatched exceptions call `basic_rethrow()` to propagate up the stack

3. **Critical Implementation Details**
   - ⚠️ **Must call setjmp directly** from generated QBE IL, not through a C wrapper
   - ⚠️ **Must branch immediately** after setjmp returns — no intermediate instructions
   - ERR() and ERL() return `int` (32-bit) → treated as `w` type in QBE IL
   - See [CRITICAL_IMPLEMENTATION_NOTES.md](CRITICAL_IMPLEMENTATION_NOTES.md#exception-handling-setjmplongjmp-calling-convention) for full details

### Example QBE IL

```qbe
# Allocate exception context
%ctx =l alloc8 256

# Push onto runtime stack
call $basic_exception_push(l %ctx)

# Call setjmp directly with jmp_buf pointer (offset 0)
%setjmp_result =w call $setjmp(l %ctx)

# IMMEDIATE branch based on return value
jnz %setjmp_result, @dispatch, @try_body

@try_body
    # ... protected code ...
    call $basic_exception_pop()
    jmp @finally

@dispatch
    # Exception occurred - get error code
    %err =w call $basic_err()
    
    # Check against CATCH codes
    %match =w ceqw %err, 10
    jnz %match, @catch_10, @rethrow
    
@catch_10
    call $basic_exception_pop()
    # ... handler code ...
    jmp @finally

@rethrow
    call $basic_rethrow()

@finally
    # ... cleanup code ...
    jmp @after_try
```

---

## Dynamic Array Operations

### Language Constructs

```basic
' Declare array
DIM arr%(1 TO 10)

' Free memory (descriptor remains, empty state)
ERASE arr%

' Reallocate with new bounds
REDIM arr%(5 TO 20)

' Reallocate while preserving existing elements
REDIM PRESERVE arr%(5 TO 25)
```

### ArrayDescriptor Structure

```c
typedef struct {
    void* data;              // Offset 0:  Pointer to array data
    int64_t lowerBound1;     // Offset 8:  Lower bound dimension 1
    int64_t upperBound1;     // Offset 16: Upper bound dimension 1
    int64_t lowerBound2;     // Offset 24: Lower bound dimension 2
    int64_t upperBound2;     // Offset 32: Upper bound dimension 2
    size_t elementSize;      // Offset 40: Element size in bytes ⚠️
    int dimensions;          // Offset 48: Number of dimensions
    int typeSuffix;          // Offset 56: Type suffix (%, $, etc.)
} ArrayDescriptor;
```

**⚠️ CRITICAL**: `elementSize` is at offset 40, NOT 24. Loading from wrong offset causes memory corruption.

### ERASE Operation

**Runtime Function**: `void array_descriptor_erase(ArrayDescriptor* desc)`

**Behavior**:
1. If string array (`typeSuffix == '$'`), iterate elements and call `string_release()` on each
2. Free data pointer: `free(desc->data)`
3. Set `data = NULL`
4. Set `dimensions = 0`
5. Reset bounds to empty state

**Important**: ERASE does **not** remove the array from the symbol table. The descriptor remains in an empty state. Use REDIM to reallocate.

### REDIM Operation (Without PRESERVE)

**Codegen Steps**:
1. Call `array_descriptor_erase()` to free old data and release strings
2. Load `elementSize` from offset 40
3. Compute size: `(upperBound - lowerBound + 1) * elementSize`
4. Allocate new memory: `malloc(size)`
5. Zero-fill: `memset(newData, 0, size)`
6. Update descriptor:
   - `data` = new pointer
   - `lowerBound1` = new lower bound
   - `upperBound1` = new upper bound
   - `dimensions` = 1 (restore after erase set it to 0)

**⚠️ CRITICAL**: Must restore `dimensions` field after erase, otherwise descriptor appears empty even with allocated data.

### REDIM PRESERVE Operation

**Codegen Steps**:
1. Load old data pointer and bounds
2. Compute old and new sizes
3. Reallocate: `realloc(oldData, newSize)`
4. If growing, zero-fill new region: `memset(newData + oldSize, 0, newSize - oldSize)`
5. Update descriptor with new bounds

**Note**: `realloc` automatically preserves existing data. No need to call `array_descriptor_erase`.

### Example QBE IL (REDIM)

```qbe
# Call erase to free old data
call $array_descriptor_erase(l %desc)

# Load element size from offset 40
%elem_ptr =l add %desc, 40
%elem_size =l loadl %elem_ptr

# Compute size = (upper - lower + 1) * elementSize
%range =l sub %upper, %lower
%count =l add %range, 1
%bytes =l mul %count, %elem_size

# Allocate
%data =l call $malloc(l %bytes)

# Zero-fill
call $memset(l %data, w 0, l %bytes)

# Store data pointer (offset 0)
storel %data, %desc

# Store lowerBound1 (offset 8)
%lb_ptr =l add %desc, 8
storel %lower, %lb_ptr

# Store upperBound1 (offset 16)
%ub_ptr =l add %desc, 16
storel %upper, %ub_ptr

# Restore dimensions = 1 (offset 48)
%dims_ptr =l add %desc, 48
storew 1, %dims_ptr
```

---

## Test Coverage

### Exception Tests (`tests/exceptions/`)

| Test File | Description |
|-----------|-------------|
| `test_try_catch_basic.bas` | Basic TRY/CATCH with specific error codes |
| `test_catch_all.bas` | Catch-all handler (no error code specified) |
| `test_finally.bas` | FINALLY block execution on both normal and exceptional paths |
| `test_nested_try.bas` | Nested TRY blocks and exception propagation |
| `test_err_erl.bas` | ERR() and ERL() builtins |
| `test_comprehensive.bas` | Complex scenario with multiple CATCH blocks and FINALLY |
| `test_throw_no_handler.bas` | Unhandled exception propagation |

### Array Tests (`tests/arrays/`)

| Test File | Description |
|-----------|-------------|
| `test_array_basic.bas` | Basic array operations (DIM, access, bounds) |
| `test_array_2d.bas` | 2D array operations |
| `test_erase.bas` | ERASE operation on numeric and string arrays |
| `test_redim.bas` | REDIM without PRESERVE |
| `test_redim_preserve.bas` | REDIM PRESERVE (growing and shrinking) |
| `test_array_memory.bas` | Memory stress test (repeated ERASE/REDIM cycles) |

### Running Tests

```bash
# Run full test suite
./test_basic_suite.sh

# Run specific category
./test_basic_suite.sh exceptions
./test_basic_suite.sh arrays

# Run single test
cd fsh
./basic --run ../tests/exceptions/test_try_catch_basic.bas
```

---

## Key Fixes Applied

### 1. Exception Handling: setjmp Calling Convention

**Problem**: Calling setjmp through a C wrapper saved the wrapper's stack frame. When longjmp restored state, it attempted to restore a destroyed frame, causing hangs/crashes.

**Fix**: Generate direct call to `setjmp` in QBE IL with jmp_buf pointer. Branch immediately after setjmp return.

**Files Changed**:
- `fsh/FasterBASICT/src/codegen/QBECodeGenerator.cpp` (exception codegen)

### 2. Array Descriptor: Wrong elementSize Offset

**Problem**: Codegen loaded `elementSize` from offset 24 (which is actually `lowerBound2`), causing wrong allocation sizes and memory corruption.

**Fix**: Load `elementSize` from correct offset 40.

**Files Changed**:
- `fsh/FasterBASICT/src/codegen/QBECodeGenerator.cpp` (array codegen)

### 3. Array Descriptor: Missing String Cleanup

**Problem**: REDIM on string arrays directly called `free()` without releasing individual strings first, causing memory leaks.

**Fix**: Call `array_descriptor_erase()` before allocating new memory. This helper releases strings properly.

**Files Changed**:
- `fsh/FasterBASICT/src/codegen/QBECodeGenerator.cpp` (REDIM codegen)

### 4. Array Descriptor: Missing Field Restoration

**Problem**: After `array_descriptor_erase()` sets `dimensions = 0`, codegen didn't restore it. Subsequent operations saw an empty descriptor even with allocated data.

**Fix**: After allocating new memory, restore `lowerBound1`, `upperBound1`, and `dimensions` fields.

**Files Changed**:
- `fsh/FasterBASICT/src/codegen/QBECodeGenerator.cpp` (REDIM codegen)

### 5. ERR() and ERL() Return Type Classification

**Problem**: ERR() and ERL() return `int` (32-bit), but codegen treated them as returning `l` (64-bit), causing incorrect type conversion ops in QBE IL.

**Fix**: Add ERR and ERL to intrinsics list that return `w` type.

**Files Changed**:
- `fsh/FasterBASICT/src/codegen/QBECodeGenerator.cpp` (intrinsic classification)

### 6. Test Harness Updates

**Problem**: Test harness was missing some runtime C files in linker command.

**Fix**: Ensured `array_descriptor_runtime.c` and `string_pool.c` are included in test compilation.

**Files Changed**:
- `test_basic_suite.sh`

---

## Validation Status

### ✅ All Tests Passing

As of the latest run:
- Exception handling tests: **7/7 passing**
- Dynamic array tests: **6/6 passing**
- Full test suite: **All tests passing**

### Platforms Tested

- macOS ARM64 (Apple Silicon M1/M2)
- macOS x86_64 (Intel)

### CI Integration

GitHub Actions workflow updated to run comprehensive test suite on all commits and PRs:
- Build compiler on both x86_64 and ARM64
- Run full test suite (`test_basic_suite.sh`)
- Upload test results as artifacts

See: `.github/workflows/build.yml`

---

## Semantic Rules

### DIM vs REDIM After ERASE

**Rule**: You may only `DIM` an array once. After `ERASE`, use `REDIM` to reallocate.

**Rationale**: `ERASE` frees memory but keeps the descriptor in the symbol table (in empty state). This is consistent with traditional BASIC semantics where `DIM` declares and `REDIM` resizes.

**Example**:
```basic
DIM arr%(1 TO 10)      ' ✅ Declare and allocate
ERASE arr%             ' ✅ Free memory (descriptor remains)
REDIM arr%(5 TO 15)    ' ✅ Reallocate with new bounds

DIM arr%(1 TO 10)      ' ✅ Initial declaration
ERASE arr%             ' ✅ Free memory
DIM arr%(5 TO 15)      ' ❌ ERROR: "Array already declared"
```

**Enforcement**: Semantic analyzer rejects duplicate `DIM` statements (not a backend limitation).

---

## Next Steps

### Completed ✅

- [x] Exception handling end-to-end implementation
- [x] Dynamic array operations (ERASE/REDIM/REDIM PRESERVE)
- [x] Comprehensive test coverage
- [x] All tests passing
- [x] CI integration
- [x] Critical implementation notes documented

### Future Enhancements

1. **Multi-dimensional REDIM**: Extend codegen to support REDIM for 2D arrays
2. **Static Analysis**: Add linter to detect incorrect descriptor field offset usage
3. **Performance Optimization**: Consider wrapping repeated allocation patterns in runtime helpers
4. **Cross-platform Testing**: Test on Linux x86_64, ARM64, and RISC-V
5. **Error Messages**: Improve semantic analyzer messages for array redeclaration errors

---

## References

- [CRITICAL_IMPLEMENTATION_NOTES.md](CRITICAL_IMPLEMENTATION_NOTES.md) — **Essential reading** for technical details
- [EXCEPTION_HANDLING_DESIGN.md](EXCEPTION_HANDLING_DESIGN.md) — High-level exception handling design
- [arraysinfasterbasic.md](../arraysinfasterbasic.md) — Array system architecture
- [START_HERE.md](../START_HERE.md) — Developer quick start guide

---

## Contributors

If you're working on exception handling or array operations:

1. **Read** [CRITICAL_IMPLEMENTATION_NOTES.md](CRITICAL_IMPLEMENTATION_NOTES.md) first
2. **Understand** the setjmp/longjmp calling convention requirements
3. **Verify** ArrayDescriptor field offsets when modifying array codegen
4. **Run** the full test suite before committing: `./test_basic_suite.sh`
5. **Add tests** for any new edge cases or bug fixes

---

**Last Updated**: January 2024  
**Status**: ✅ Complete and Validated