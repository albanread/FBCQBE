# Critical Implementation Notes

## Overview

This document describes critical implementation details and gotchas discovered during development of exception handling and dynamic array operations in the FasterBASIC → QBE compiler. These notes are **essential reading** for anyone modifying codegen, runtime, or debugging related issues.

---

## Table of Contents

1. [Exception Handling: setjmp/longjmp Calling Convention](#exception-handling-setjmplongjmp-calling-convention)
2. [ArrayDescriptor Memory Layout](#arraydescriptor-memory-layout)
3. [Exception Handling Implementation](#exception-handling-implementation)
4. [Dynamic Array Operations](#dynamic-array-operations)
5. [Semantic Rules vs Backend Behavior](#semantic-rules-vs-backend-behavior)

---

## Exception Handling: setjmp/longjmp Calling Convention

### ⚠️ CRITICAL: Do NOT Call setjmp Through a Wrapper

**Problem**: Calling `setjmp` via a C wrapper function will cause crashes and hangs when `longjmp` is called.

**Root Cause**: When you call `setjmp` through a wrapper:
1. The wrapper's stack frame is created
2. `setjmp` saves the wrapper's stack frame state (registers, stack pointer, etc.)
3. Control returns through the wrapper back to the caller
4. The wrapper's stack frame is destroyed
5. Later, when `longjmp` is called, it attempts to restore the saved state
6. **CRASH/HANG**: The stack frame that was saved no longer exists

**Solution**: Generate a **direct call** to the platform `setjmp` from QBE IL, passing the `jmp_buf` pointer directly.

### Example: Incorrect Implementation

```c
// WRONG: Do not do this in runtime
int basic_exception_setup_wrapper(ExceptionContext* ctx) {
    return setjmp(ctx->jmp_buf);  // Saves wrapper's stack frame!
}
```

```qbe
# WRONG: Calling through wrapper
%result =w call $basic_exception_setup_wrapper(l %ctx_ptr)
```

### Example: Correct Implementation

```qbe
# CORRECT: Direct call to setjmp with jmp_buf pointer
%jmp_buf_ptr =l add %ctx_ptr, 0  # jmp_buf is at offset 0 in ExceptionContext
%setjmp_result =w call $setjmp(l %jmp_buf_ptr)
jnz %setjmp_result, @dispatch_block, @protected_code
```

### Additional Requirement: Immediate Branching

After `setjmp` returns, you **must** immediately branch based on its return value. Do not emit other instructions before checking the result.

**Why**: `longjmp` restores registers to the state saved by `setjmp`. Any instructions between the `setjmp` return and the branch check can have their register state corrupted when `longjmp` executes.

```qbe
# CORRECT: Immediate branch after setjmp
%setjmp_result =w call $setjmp(l %jmp_buf_ptr)
jnz %setjmp_result, @dispatch_block, @protected_code

# WRONG: Do not emit instructions here before branching
# %temp =w add %setjmp_result, 0  # <-- NO! Can be corrupted by longjmp
```

---

## ArrayDescriptor Memory Layout

### Structure Definition

Dynamic arrays in FasterBASIC use an `ArrayDescriptor` "dope vector" that contains metadata and a pointer to the actual array data.

```c
typedef struct {
    void* data;              // Offset 0:  Pointer to array data
    int64_t lowerBound1;     // Offset 8:  Lower bound of dimension 1
    int64_t upperBound1;     // Offset 16: Upper bound of dimension 1
    int64_t lowerBound2;     // Offset 24: Lower bound of dimension 2
    int64_t upperBound2;     // Offset 32: Upper bound of dimension 2
    size_t elementSize;      // Offset 40: Size of each element in bytes
    int dimensions;          // Offset 48: Number of dimensions (1 or 2)
    int typeSuffix;          // Offset 56: Type suffix (%, $, !, #, etc.)
} ArrayDescriptor;
```

### ⚠️ CRITICAL: Field Offsets

**Always use the correct offsets when generating QBE IL to access descriptor fields.**

Common mistakes:
- ❌ Loading `elementSize` from offset 24 → **WRONG** (that's `lowerBound2`)
- ✅ Loading `elementSize` from offset 40 → **CORRECT**

### Example: Correct Field Access in QBE IL

```qbe
# Load element size (offset 40)
%elem_size_ptr =l add %desc_ptr, 40
%elem_size =l loadl %elem_size_ptr

# Load upper bound (offset 16)
%upper_ptr =l add %desc_ptr, 16
%upper =l loadl %upper_ptr

# Load dimensions (offset 48)
%dims_ptr =l add %desc_ptr, 48
%dims =w loadw %dims_ptr

# Load type suffix (offset 56)
%type_ptr =l add %desc_ptr, 56
%type =w loadw %type_ptr
```

### Quick Reference Table

| Field         | Offset | Type    | QBE Load  |
|---------------|--------|---------|-----------|
| data          | 0      | void*   | loadl     |
| lowerBound1   | 8      | int64_t | loadl     |
| upperBound1   | 16     | int64_t | loadl     |
| lowerBound2   | 24     | int64_t | loadl     |
| upperBound2   | 32     | int64_t | loadl     |
| elementSize   | 40     | size_t  | loadl     |
| dimensions    | 48     | int     | loadw     |
| typeSuffix    | 56     | int     | loadw     |

---

## Exception Handling Implementation

### Runtime Functions

The runtime provides these exception-handling functions:

```c
void basic_exception_push(ExceptionContext* ctx);  // Push context onto stack
void basic_exception_pop(void);                     // Pop context from stack
void basic_throw(int errorCode, int lineNumber);    // Throw exception
int basic_err(void);                                // Get current error code
int basic_erl(void);                                // Get current error line
void basic_rethrow(void);                           // Rethrow current exception
```

### Codegen Flow for TRY/CATCH/FINALLY

1. **Allocate Exception Context**
   ```qbe
   %ctx_ptr =l alloc8 256  # Allocate ExceptionContext on stack
   ```

2. **Push Context onto Runtime Stack**
   ```qbe
   call $basic_exception_push(l %ctx_ptr)
   ```

3. **Call setjmp Directly**
   ```qbe
   %setjmp_result =w call $setjmp(l %ctx_ptr)
   jnz %setjmp_result, @dispatch_block, @try_body
   ```

4. **Execute Protected Code**
   ```qbe
   @try_body
       # ... TRY block code ...
       call $basic_exception_pop()  # Pop on normal exit
       jmp @finally_block           # Execute FINALLY
   ```

5. **Dispatch Block (Exception Occurred)**
   ```qbe
   @dispatch_block
       %err =w call $basic_err()
       # Compare against CATCH codes
       %match1 =w ceqw %err, 5
       jnz %match1, @catch_5, @try_next
   @try_next
       %match2 =w ceqw %err, 10
       jnz %match2, @catch_10, @rethrow
   @rethrow
       call $basic_rethrow()  # No matching CATCH
   ```

6. **CATCH Blocks**
   ```qbe
   @catch_5
       call $basic_exception_pop()  # Pop context
       # ... CATCH 5 handler code ...
       jmp @finally_block
   ```

7. **FINALLY Block**
   ```qbe
   @finally_block
       # ... FINALLY code (always executes) ...
       jmp @after_try
   ```

### ERR() and ERL() Builtins

These functions return `int` (32-bit) values in C, but must be treated as returning `w` type in QBE IL.

**Codegen Classification**: Add `ERR` and `ERL` to the list of intrinsics that return `w`:

```cpp
// In QBECodeGenerator::emitExpression or similar
if (funcName == "ERR" || funcName == "ERL") {
    resultType = "w";  // 32-bit int
}
```

**Incorrect codegen** (missing classification):
```qbe
%err =l call $basic_err()  # WRONG: Should be 'w' not 'l'
%f =s sltof %err           # WRONG: Can't sltof on 'w', causes QBE error
```

**Correct codegen**:
```qbe
%err =w call $basic_err()  # CORRECT: Returns 'w'
%f =s swtof %err           # CORRECT: Convert w to s using swtof
```

---

## Dynamic Array Operations

### ERASE Operation

`ERASE arrayName` frees the array's data memory and resets the descriptor to an empty state.

#### Runtime Function

```c
void array_descriptor_erase(ArrayDescriptor* desc);
```

**What it does**:
1. If the array is a string array (`typeSuffix == '$'`), iterate through all elements and call `string_release()` on each to free string memory
2. Free the `data` pointer with `free(desc->data)`
3. Set `data` to `NULL`
4. Set `dimensions` to `0`
5. Set bounds to empty (e.g., `lowerBound1 = 0`, `upperBound1 = -1`)

#### Codegen for ERASE

```qbe
# Get descriptor pointer
%desc_ptr =l ... 

# Call erase helper
call $array_descriptor_erase(l %desc_ptr)

# Descriptor now in empty state (data=NULL, dimensions=0)
```

#### ⚠️ IMPORTANT: ERASE Does Not Remove Declaration

After `ERASE`, the `ArrayDescriptor` remains in the symbol table. The descriptor exists but represents an empty/unallocated array. You must use `REDIM` to allocate memory again — you **cannot** use `DIM` a second time (semantic analyzer will reject it).

---

### REDIM Operation

`REDIM arrayName(newLower TO newUpper)` allocates or reallocates memory for an existing array.

#### Codegen Steps (Without PRESERVE)

1. **Call `array_descriptor_erase` First** (to free old data and release strings if needed)
   ```qbe
   call $array_descriptor_erase(l %desc_ptr)
   ```

2. **Compute New Size**
   ```qbe
   # Load element size from offset 40 (CRITICAL!)
   %elem_ptr =l add %desc_ptr, 40
   %elem_size =l loadl %elem_ptr
   
   # Compute count = (upper - lower + 1)
   %range =l sub %new_upper, %new_lower
   %count =l add %range, 1
   
   # Total bytes = count * elementSize
   %total_bytes =l mul %count, %elem_size
   ```

3. **Allocate New Memory**
   ```qbe
   %new_data =l call $malloc(l %total_bytes)
   ```

4. **Zero-Fill New Memory**
   ```qbe
   call $memset(l %new_data, w 0, l %total_bytes)
   ```

5. **Update Descriptor Fields**
   ```qbe
   # Store new data pointer (offset 0)
   storel %new_data, %desc_ptr
   
   # Store new lowerBound1 (offset 8)
   %lb_ptr =l add %desc_ptr, 8
   storel %new_lower, %lb_ptr
   
   # Store new upperBound1 (offset 16)
   %ub_ptr =l add %desc_ptr, 16
   storel %new_upper, %ub_ptr
   
   # Restore dimensions = 1 (offset 48)
   # (erase set it to 0, so restore it)
   %dims_ptr =l add %desc_ptr, 48
   storew 1, %dims_ptr
   ```

#### ⚠️ CRITICAL: Restore Descriptor Fields After Erase

`array_descriptor_erase` sets `dimensions = 0` and resets bounds. After allocating new memory, you **must** restore:
- `lowerBound1`
- `upperBound1`
- `dimensions`

Otherwise, subsequent array operations will fail or behave incorrectly because the descriptor appears empty even though data is allocated.

---

### REDIM PRESERVE Operation

`REDIM PRESERVE arrayName(newLower TO newUpper)` reallocates memory while preserving existing elements.

#### Codegen Steps

1. **Load Old Data Pointer and Bounds**
   ```qbe
   %old_data =l loadl %desc_ptr
   %lb_ptr =l add %desc_ptr, 8
   %old_lower =l loadl %lb_ptr
   %ub_ptr =l add %desc_ptr, 16
   %old_upper =l loadl %ub_ptr
   ```

2. **Compute Old and New Sizes**
   ```qbe
   %elem_ptr =l add %desc_ptr, 40
   %elem_size =l loadl %elem_ptr
   
   %old_count =l sub %old_upper, %old_lower
   %old_count =l add %old_count, 1
   %old_bytes =l mul %old_count, %elem_size
   
   %new_count =l sub %new_upper, %new_lower
   %new_count =l add %new_count, 1
   %new_bytes =l mul %new_count, %elem_size
   ```

3. **Reallocate with realloc**
   ```qbe
   %new_data =l call $realloc(l %old_data, l %new_bytes)
   ```

4. **Zero-Fill Newly Allocated Region** (if growing)
   ```qbe
   # If new_bytes > old_bytes, zero the extra region
   %is_growing =w csgtl %new_bytes, %old_bytes
   jnz %is_growing, @zero_fill, @skip_zero
   
   @zero_fill
       %fill_start =l add %new_data, %old_bytes
       %fill_size =l sub %new_bytes, %old_bytes
       call $memset(l %fill_start, w 0, l %fill_size)
       jmp @skip_zero
   
   @skip_zero
   ```

5. **Update Descriptor**
   ```qbe
   storel %new_data, %desc_ptr
   %lb_ptr =l add %desc_ptr, 8
   storel %new_lower, %lb_ptr
   %ub_ptr =l add %desc_ptr, 16
   storel %new_upper, %ub_ptr
   ```

#### Notes on PRESERVE

- `realloc` automatically preserves the old data up to the smaller of old/new sizes
- You only need to zero-fill the **newly allocated portion** when growing
- For string arrays, `PRESERVE` keeps the old string pointers intact (no special handling needed)
- No need to call `array_descriptor_erase` for PRESERVE — we're keeping the data

---

## Semantic Rules vs Backend Behavior

### DIM vs REDIM

**Semantic Rule** (enforced by semantic analyzer):
- `DIM arrayName(lower TO upper)` declares an array **once**
- Attempting to `DIM` the same array name again is an **error** ("array already declared")
- This is a language-level / semantic rule, consistent with traditional BASIC semantics

**Backend Behavior**:
- The QBE backend and SSA form have no inherent restriction on re-allocation
- The semantic analyzer's check happens **before** codegen, so codegen never sees duplicate `DIM` attempts

### Correct Usage After ERASE

After `ERASE`, the correct operation to allocate memory is `REDIM`, not `DIM`:

```basic
DIM arr%(10 TO 20)      ' Declare and allocate
ERASE arr%              ' Free memory (descriptor remains, empty state)
REDIM arr%(1 TO 15)     ' Reallocate with new bounds
```

**Incorrect usage**:
```basic
DIM arr%(10 TO 20)      ' Declare and allocate
ERASE arr%              ' Free memory
DIM arr%(1 TO 15)       ' ERROR: Semantic analyzer rejects (already declared)
```

### Why This Matters

- `ERASE` does not remove the array from the symbol table
- The descriptor remains in an "empty" state (data=NULL, dimensions=0)
- `REDIM` operates on existing descriptors and is the correct operation for reallocation
- This is not a backend/SSA limitation — it's a deliberate language design consistent with BASIC semantics

---

## Testing and Validation

### Exception Handling Tests

Located in `tests/exceptions/`:
- `test_try_catch_basic.bas` — Basic TRY/CATCH
- `test_try_catch_specific.bas` — Specific error codes
- `test_try_catch_all.bas` — Catch-all handler
- `test_try_finally.bas` — FINALLY block execution
- `test_try_nested.bas` — Nested exception handling
- `test_err_erl.bas` — ERR() and ERL() builtins

### Dynamic Array Tests

Located in `tests/arrays/`:
- `test_erase_basic.bas` — Basic ERASE functionality
- `test_redim_basic.bas` — REDIM without PRESERVE
- `test_redim_preserve.bas` — REDIM PRESERVE
- `test_erase_strings.bas` — ERASE with string arrays
- `test_redim_cycles.bas` — Repeated ERASE/REDIM cycles

### Debugging Tips

1. **Generate QBE IL** to inspect codegen:
   ```bash
   ./qbe_basic -i test.bas > test.qbe
   ```

2. **Check field offsets** in QBE IL — ensure elementSize loaded from offset 40, not 24

3. **Check setjmp calls** — ensure direct call to `$setjmp`, not a wrapper

4. **Verify immediate branching** after setjmp — no instructions between call and branch

5. **For array issues**, add print statements in runtime to log:
   - Descriptor field values (data pointer, bounds, elementSize, dimensions)
   - Allocation sizes
   - Whether string cleanup is called

6. **For exception issues**, add logging to runtime:
   - Log setjmp return values
   - Log exception context push/pop
   - Log ERR() and ERL() values in handlers

---

## Platform-Specific Notes

### setjmp/longjmp and ABIs

Different platforms have different calling conventions and register-saving behavior for `setjmp`:
- **x86_64**: Standard System V ABI on Linux/macOS
- **ARM64**: AAPCS64 — different register set, different jmp_buf size
- **RISC-V**: May require different jmp_buf alignment

**Recommendation**: Always test exception handling on target platforms. The direct `setjmp` call approach works across platforms, but wrapper approaches may fail differently on different ABIs.

### jmp_buf Size

The `ExceptionContext` allocates 256 bytes for `jmp_buf`. This is sufficient for most platforms, but verify:
- Linux x86_64: 200 bytes
- macOS x86_64: 208 bytes  
- ARM64: varies by platform (typically 280-312 bytes)

If targeting ARM64 or other platforms, increase the allocation size if needed.

---

## Summary Checklist

When implementing or debugging exception handling:
- [ ] Direct call to `setjmp` (not through wrapper)
- [ ] Immediate branch after `setjmp` return
- [ ] Push exception context before `setjmp`
- [ ] Pop exception context on normal exit and in CATCH blocks
- [ ] ERR() and ERL() treated as returning `w` type
- [ ] Dispatch logic compares `ERR()` to CATCH codes correctly

When implementing or debugging dynamic arrays:
- [ ] Load `elementSize` from offset 40 (not 24)
- [ ] Call `array_descriptor_erase` before reallocating (non-PRESERVE REDIM)
- [ ] Restore descriptor fields (bounds, dimensions) after erase
- [ ] Use `realloc` for PRESERVE
- [ ] Zero-fill newly allocated regions
- [ ] For string arrays, `array_descriptor_erase` handles string cleanup

---

## References

- [EXCEPTION_HANDLING_DESIGN.md](EXCEPTION_HANDLING_DESIGN.md) — High-level design
- [arraysinfasterbasic.md](../arraysinfasterbasic.md) — Array system overview
- `fsh/FasterBASICT/runtime/` — Runtime source code
- `fsh/FasterBASICT/src/codegen/` — Codegen source code

---

## Change Log

| Date       | Change |
|------------|--------|
| 2024-01-XX | Initial document created after exception handling and array descriptor fixes |
