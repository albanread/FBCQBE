# String Design Compliance Review
**Date:** 2024
**Reviewer:** Code Analysis
**Status:** ⚠️ CRITICAL ISSUES FOUND

## Executive Summary

The runtime string implementation is **mostly correct** and follows the design specification, but the **V2 code generator does NOT properly implement string assignment semantics**, leading to memory leaks and incorrect reference counting.

### Critical Issues
1. ❌ **Code generator doesn't handle string reference counting** during assignment
2. ❌ **Two conflicting `StringDescriptor` definitions** exist in the codebase
3. ❌ **String assignment leaks memory** - old values are not released
4. ❌ **No copy-on-write enforcement** in regular assignments
5. ⚠️ **Inconsistent use of string pool** vs direct allocation

---

## Design Specification (Target)

The strings are meant to work as follows:

1. ✅ **Dual encoding support**: ASCII (1 byte/char) or Unicode (32-bit codepoints)
2. ✅ **Direct character access**: O(1) array-like lookup
3. ✅ **UTF-8 I/O conversion**: Convert to UTF-8 only at system boundaries
4. ✅ **String descriptor fields**:
   - `length` - character count
   - `encoding` - ASCII or UTF32 flag
   - `data` - pointer to character array
   - `utf8_cache` - cached UTF-8 representation
   - `dirty` - cache invalidation flag
   - `refcount` - reference counting for sharing
   - `capacity` - allocated capacity
5. ⚠️ **In-place mutation**: When refcount == 1, can mutate directly
6. ✅ **UTF-8 cache invalidation**: Set `dirty=1` when mutating
7. ✅ **Copy-on-write**: Clone before mutation if `refcount > 1`

---

## Implementation Analysis

### ✅ Runtime Implementation (Correct)

**File:** `fsh/FasterBASICT/runtime_c/string_descriptor.h`

```c
typedef struct {
    void*     data;        // uint8_t* for ASCII, uint32_t* for UTF-32
    int64_t   length;      // Length in characters
    int64_t   capacity;    // Capacity in characters
    int32_t   refcount;    // Reference count
    uint8_t   encoding;    // STRING_ENCODING_ASCII or STRING_ENCODING_UTF32
    uint8_t   dirty;       // UTF-8 cache is invalid
    uint8_t   _padding[2]; // Alignment
    char*     utf8_cache;  // Cached UTF-8 string
} StringDescriptor;
```

**✅ Correct behaviors observed:**
- Dual encoding support (ASCII/UTF-32)
- Character access via `string_char_at()` checks encoding
- UTF-8 conversion in `string_to_utf8()` respects dirty flag
- Copy-on-write in `string_mid_assign()`:
  ```c
  if (str->refcount > 1) {
      StringDescriptor* new_str = string_clone(str);
      str->refcount--;  // Decrement old
      str = new_str;    // Work with copy
  }
  ```
- UTF-8 cache invalidation on mutation:
  ```c
  str->dirty = 1;
  if (str->utf8_cache) {
      free(str->utf8_cache);
      str->utf8_cache = NULL;
  }
  ```

### ❌ Problem #1: Duplicate StringDescriptor Definition

**File:** `fsh/FasterBASICT/runtime_c/string_pool.h`

```c
typedef struct StringDescriptor {
    uint32_t* data;        // ❌ WRONG: Always uint32_t*, no encoding support
    int64_t   length;
    int64_t   capacity;
    int32_t   refcount;
    uint8_t   dirty;
    uint8_t   _padding[3]; // ❌ WRONG: Missing encoding field
    char*     utf8_cache;
} StringDescriptor;
```

**Impact:** This creates a conflicting definition that doesn't match the design. The `string_pool.h` version is missing the `encoding` field and assumes only UTF-32 (`uint32_t*`).

**Resolution Required:**
- **Remove or align** `string_pool.h` definition
- Use a single source of truth from `string_descriptor.h`
- Or use `string_pool.h` only for pool management, not descriptor definition

---

### ❌ Problem #2: V2 Code Generator String Assignment (CRITICAL)

**File:** `fsh/FasterBASICT/src/codegen_v2/ast_emitter.cpp`

```cpp
void ASTEmitter::storeVariable(const std::string& varName, const std::string& value) {
    BaseType varType = getVariableType(varName);
    std::string qbeType = typeManager_.getQBEType(varType);
    
    // ... parameter and symbol lookup ...
    
    // All variables stored in memory
    std::string addr = getVariableAddress(varName);
    builder_.emitStore(qbeType, value, addr);  // ❌ WRONG: Just overwrites pointer!
}
```

**What's wrong:**
1. ❌ No reference count decrement on old value (memory leak)
2. ❌ No reference count increment on new value
3. ❌ No string release/retain calls
4. ❌ Doesn't distinguish string types from other types
5. ❌ Overwrites pointer without cleanup

**What SHOULD happen for string assignment `a$ = b$`:**

```cpp
// Pseudo-code for correct string assignment
if (varType == STRING) {
    // 1. Load old value
    oldPtr = loadVariable(varName);
    
    // 2. Release old string (decrements refcount, frees if 0)
    call string_desc_release(oldPtr);
    
    // 3. Retain new string (increments refcount)
    newPtr = call string_desc_retain(value);
    
    // 4. Store new pointer
    storePointer(addr, newPtr);
}
```

**OR** (copy semantic):
```cpp
if (varType == STRING) {
    // 1. Release old value
    oldPtr = loadVariable(varName);
    call string_desc_release(oldPtr);
    
    // 2. Clone new value (independent copy)
    newPtr = call string_desc_clone(value);
    
    // 3. Store new pointer
    storePointer(addr, newPtr);
}
```

---

### ❌ Problem #3: No Copy-on-Write for Regular Assignment

The design states:
> "When mutating strings by assignment they can be mutated in place"

This implies that if we do:
```basic
a$ = "hello"
a$ = "world"  ' Can mutate in place if refcount == 1
```

The code generator should:
1. Check if the string descriptor has `refcount == 1`
2. Check if new string fits in existing capacity
3. If yes, mutate in place (clear UTF-8 cache)
4. If no, allocate new descriptor

**Current behavior:** Always overwrites pointer, never mutates in place.

---

### ✅ What's Working Correctly

1. **String concatenation** (`string_concat`):
   - Always creates new string ✅
   - Handles ASCII + UTF-32 mixing ✅
   - Properly sets encoding flag ✅

2. **String slicing** (`string_mid`, `string_left`, `string_right`):
   - Creates new descriptors ✅
   - Copies character data ✅

3. **MID$ assignment** (`string_mid_assign`):
   - Implements copy-on-write ✅
   - Checks `refcount > 1` and clones ✅
   - Clears UTF-8 cache after mutation ✅

4. **UTF-8 conversion** (`string_to_utf8`):
   - Checks dirty flag ✅
   - Uses cache when valid ✅
   - Regenerates when dirty ✅

5. **Character access** (`string_char_at`, `string_set_char`):
   - Checks encoding field ✅
   - Handles ASCII and UTF-32 correctly ✅
   - Sets dirty flag on write ✅

---

## Required Fixes

### Fix #1: Unify StringDescriptor Definition

**Action:** Remove duplicate definition in `string_pool.h`

**File:** `fsh/FasterBASICT/runtime_c/string_pool.h`

**Change:**
```c
// OLD (WRONG):
typedef struct StringDescriptor {
    uint32_t* data;  // Wrong
    ...
} StringDescriptor;

// NEW (CORRECT):
#include "string_descriptor.h"  // Use canonical definition
// Remove duplicate typedef
```

### Fix #2: Implement String Assignment in Code Generator

**File:** `fsh/FasterBASICT/src/codegen_v2/ast_emitter.cpp`

**Function:** `storeVariable()`

**Add string-specific handling:**

```cpp
void ASTEmitter::storeVariable(const std::string& varName, const std::string& value) {
    BaseType varType = getVariableType(varName);
    std::string qbeType = typeManager_.getQBEType(varType);
    
    // ... existing parameter/symbol lookup ...
    
    std::string addr = getVariableAddress(varName);
    
    // *** NEW: Handle strings specially ***
    if (typeManager_.isString(varType)) {
        // String assignment requires reference counting
        
        // 1. Load old string pointer
        std::string oldPtr = builder_.newTemp();
        builder_.emitLoad(oldPtr, "l", addr);
        
        // 2. Release old string (decrements refcount, frees if 0)
        builder_.emitCall("", "", "string_desc_release", "l " + oldPtr);
        
        // 3. Clone new string (copy semantic for safety)
        // OR retain if we want shared semantics
        std::string newPtr = builder_.newTemp();
        builder_.emitCall(newPtr, "l", "string_desc_clone", "l " + value);
        
        // 4. Store new pointer
        builder_.emitStore("l", newPtr, addr);
    } else {
        // Non-string types: regular store
        builder_.emitStore(qbeType, value, addr);
    }
}
```

### Fix #3: Add Runtime Wrappers to RuntimeLibrary

**File:** `fsh/FasterBASICT/src/codegen_v2/runtime_library.h`

**Add methods:**
```cpp
// String lifecycle management
std::string emitStringClone(const std::string& stringPtr);
std::string emitStringRetain(const std::string& stringPtr);
void emitStringRelease(const std::string& stringPtr);
```

**File:** `fsh/FasterBASICT/src/codegen_v2/runtime_library.cpp`

**Implementation:**
```cpp
std::string RuntimeLibrary::emitStringClone(const std::string& stringPtr) {
    return emitRuntimeCall("string_desc_clone", "l", "l " + stringPtr);
}

std::string RuntimeLibrary::emitStringRetain(const std::string& stringPtr) {
    return emitRuntimeCall("string_desc_retain", "l", "l " + stringPtr);
}

void RuntimeLibrary::emitStringRelease(const std::string& stringPtr) {
    emitRuntimeCallVoid("string_desc_release", "l " + stringPtr);
}
```

### Fix #4: Add In-Place Mutation Optimization (Optional)

For performance, implement true in-place mutation when safe:

```cpp
// Pseudo-code for optimized string assignment
void emitStringAssignOptimized(addr, newValue) {
    // Check if new value is a string literal
    if (isLiteral(newValue)) {
        // Replace entire string - use clone
        oldPtr = load(addr);
        string_desc_release(oldPtr);
        newPtr = string_desc_clone(newValue);
        store(addr, newPtr);
    } else {
        // Runtime check for in-place mutation
        oldPtr = load(addr);
        if (oldPtr->refcount == 1 && oldPtr->capacity >= newValue->length) {
            // Can mutate in place
            string_copy_data(oldPtr, newValue);
            oldPtr->dirty = 1;  // Invalidate UTF-8 cache
        } else {
            // Must clone
            string_desc_release(oldPtr);
            newPtr = string_desc_clone(newValue);
            store(addr, newPtr);
        }
    }
}
```

---

## Testing Requirements

After fixes, test:

1. **String assignment doesn't leak memory:**
   ```basic
   FOR i = 1 TO 10000
       a$ = "test" + STR$(i)
   NEXT i
   ' Check memory usage stable
   ```

2. **Copy-on-write works:**
   ```basic
   a$ = "hello"
   b$ = a$           ' Shared reference
   MID$(b$, 1, 1) = "H"  ' Should clone before mutating
   PRINT a$; b$      ' Should print "hello Hello"
   ```

3. **UTF-8 cache invalidation:**
   ```basic
   a$ = "test"
   PRINT a$          ' Generates UTF-8 cache
   MID$(a$, 1, 1) = "T"  ' Must clear cache
   PRINT a$          ' Should print "Test", not "test"
   ```

4. **Encoding preservation:**
   ```basic
   a$ = "hello"      ' ASCII
   b$ = "こんにちは"  ' UTF-32
   c$ = a$ + b$      ' Should be UTF-32
   PRINT LEN(c$)     ' Character count, not bytes
   ```

---

## Summary

| Component | Status | Compliance |
|-----------|--------|------------|
| Runtime StringDescriptor definition | ✅ Correct | 100% |
| Runtime string operations | ✅ Correct | 95% |
| Runtime copy-on-write | ✅ Correct | 100% |
| Runtime UTF-8 caching | ✅ Correct | 100% |
| V2 code generator assignment | ❌ BROKEN | 0% |
| V2 code generator reference counting | ❌ MISSING | 0% |
| String pool definition | ⚠️ CONFLICTING | 50% |

**Overall Assessment:** Runtime is solid, but the V2 code generator needs **immediate fixes** to prevent memory leaks and implement proper string semantics.

---

## Priority

**P0 - CRITICAL:**
1. Fix `storeVariable()` to handle string assignment with reference counting
2. Add `string_desc_release()` calls to prevent memory leaks

**P1 - HIGH:**
3. Unify StringDescriptor definitions
4. Add runtime wrapper methods to RuntimeLibrary

**P2 - MEDIUM:**
5. Implement in-place mutation optimization
6. Add comprehensive string assignment tests

**P3 - LOW:**
7. Document string assignment semantics
8. Add performance benchmarks