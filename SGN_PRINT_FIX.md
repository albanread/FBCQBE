# SGN() Function Print Fix

## Issue

The `SGN()` function was returning correct values internally but was printing incorrect values. Specifically:
- `SGN(-5)` printed `4294967295` instead of `-1`
- `SGN(-10)` printed `4294967295` instead of `-1`

The value `4294967295` is `0xFFFFFFFF`, which is the unsigned 32-bit representation of `-1` in two's complement.

## Root Cause

The issue was in the CodeGen V2 runtime library (`runtime_library.cpp`):

```cpp
void RuntimeLibrary::emitPrintInt(const std::string& value) {
    emitRuntimeCallVoid("basic_print_int", "w " + value);
}
```

This was **always** passing a `w` (32-bit word) directly to the C runtime function `basic_print_int`, which expects an `int64_t` (64-bit signed integer):

```c
void basic_print_int(int64_t value) {
    printf("%lld", (long long)value);
    fflush(stdout);
}
```

### The Problem

When a 32-bit value is passed to a function expecting 64-bit:
- The QBE calling convention passes 32 bits
- The C function reads 64 bits from the stack/registers
- The upper 32 bits contain garbage/undefined values
- For negative 32-bit values (e.g., `-1` = `0xFFFFFFFF`), the C function interprets it as a large unsigned 64-bit value

### The Second Problem

The initial "fix" was too aggressive - it **always** sign-extended from `w` to `l`, but some integer values are already in `l` (64-bit) form, such as:
- Some intermediate expression results
- Values that were already extended earlier in the pipeline

This would cause incorrect behavior for values that were already 64-bit.

## Solution

The correct fix checks the **actual QBE type** of the value before deciding whether to sign-extend:

```cpp
void RuntimeLibrary::emitPrintInt(const std::string& value, BasicType valueType) {
    // basic_print_int expects int64_t (l type)
    // Check actual QBE type - only sign-extend if it's w (32-bit)
    std::string qbeType = typeManager_.getQBEType(valueType);
    
    if (qbeType == "w") {
        // Sign-extend 32-bit word to 64-bit long
        std::string longValue = builder_.newTemp();
        builder_.emitConvert(longValue, "l", "extsw", value);
        emitRuntimeCallVoid("basic_print_int", "l " + longValue);
    } else {
        // Already long (l), pass directly
        emitRuntimeCallVoid("basic_print_int", "l " + value);
    }
}
```

The `extsw` QBE instruction (extend signed word to long) properly sign-extends the 32-bit value:
- `-1` (32-bit: `0xFFFFFFFF`) → `-1` (64-bit: `0xFFFFFFFFFFFFFFFF`)
- `1` (32-bit: `0x00000001`) → `1` (64-bit: `0x0000000000000001`)

## QBE IL Generated

Before the fix:
```qbe
%t.5 =w sub %t.4, %t.3
call $basic_print_int(w %t.5)    # Wrong: passing 32-bit to 64-bit function
```

After the fix:
```qbe
%t.5 =w sub %t.4, %t.3           # Result is w (32-bit)
%t.6 =l extsw %t.5               # Sign-extend 32-bit to 64-bit
call $basic_print_int(l %t.6)    # Correct: passing 64-bit to 64-bit function
```

For values already in `l` form, no extension is performed:
```qbe
%t.10 =l some_long_operation
call $basic_print_int(l %t.10)   # Already long, pass directly
```

## Files Modified

1. `fsh/FasterBASICT/src/codegen_v2/runtime_library.h`
   - Changed signature: `void emitPrintInt(const std::string& value, BasicType valueType)`
   - Added `valueType` parameter to determine actual QBE type

2. `fsh/FasterBASICT/src/codegen_v2/runtime_library.cpp` (lines 13-27)
   - Added type checking logic
   - Conditional sign-extension based on actual QBE type

3. `fsh/FasterBASICT/src/codegen_v2/ast_emitter.cpp` (line 878)
   - Updated call site to pass `exprType` parameter

## Test Results

After the fix, all integer printing works correctly:

### SGN Tests
```
SGN(-10) = -1  ✓
SGN(-1) = -1   ✓
SGN(0) = 0     ✓
SGN(1) = 1     ✓
SGN(10) = 1    ✓
```

### Full ABS & SGN Test Suite
```
=== ABS & SGN Optimization Tests ===

--- SGN Integer ---
SGN(-5) = -1
SGN(5) = 1
SGN(0) = 0
PASS

--- SGN Double (Branchless) ---
SGN(-3.14) = -1
SGN(3.14) = 1
SGN(0.0) = 0
PASS

=== ALL TESTS PASSED ===
```

### General Integer Printing
```
Negative: -100
Positive: 100
Expression: 0
Zero: 0
Max negative: -32768
Max positive: 32767
```

All signed integers now print with correct signs.

## Note on Old CodeGen

The old codegen (`codegen_old/qbe_codegen_runtime.cpp`) already had this logic correct:

```cpp
if (qbeType == "w") {
    std::string longValue = allocTemp("l");
    emit("    " + longValue + " =l extsw " + value + "\n");
    emit("    call $basic_print_int(l " + longValue + ")\n");
} else {
    // Already long, pass directly
    emit("    call $basic_print_int(l " + value + ")\n");
}
```

The CodeGen V2 implementation was missing this type-checking and conditional sign-extension, which has now been added to match the correct behavior.

## Impact

This fix ensures that:
1. All signed 32-bit integers are correctly printed with their proper sign
2. Works for `SGN()` function results
3. Works for all integer printing operations throughout the compiler
4. Doesn't break 64-bit integer values that are already in `l` form
5. Maintains compatibility with the QBE IL calling conventions and C runtime expectations

## Related Issues

During testing, it was discovered that user-defined FUNCTION calls are not yet fully implemented in CodeGen V2 (they emit `TODO` comments and store 0 instead of calling the function). This is a separate issue unrelated to the integer printing fix.