# Remaining Fix for test_data_mixed

## Status
- **81/83 tests passing (97.6%)**
- Slice tests now PASS ✅
- test_data_mixed compiles but hangs at runtime

## Issue
The `basic_read_string()` function in `qbe_basic_integrated/runtime/basic_data.c` has been partially updated but the file editing system failed to save the complete changes.

## Required Manual Fix

Edit `qbe_basic_integrated/runtime/basic_data.c` around line 95:

### Current (WRONG):
```c
const char* basic_read_string(void) {
    // ... code ...
    const char* str = (const char*)__basic_data[idx];
    __basic_data_ptr++;
    return str;
}
```

### Should be (CORRECT):
```c
StringDescriptor* basic_read_string(void) {
    // ... code ...
    const char* str = (const char*)__basic_data[idx];
    __basic_data_ptr++;
    return string_new_utf8(str);  // <-- Create StringDescriptor from C string
}
```

## Why This Fix is Needed

1. The DATA array stores **pointers** to C strings (e.g., `l $data_str.3` points to "PI")
2. The BASIC code expects `StringDescriptor*` not `const char*`
3. Need to convert: C string → StringDescriptor using `string_new_utf8()`

## After Fix
1. Clean rebuild: `cd qbe_basic_integrated && rm -rf runtime/.obj && bash build_qbe_basic.sh`
2. Run tests: `bash run_tests_simple.sh`
3. Expected: 82/83 tests passing (only test_throw_no_handler should fail - test harness issue)

## Architecture Notes

**DATA Storage:**
- `$__basic_data` array: int64_t values (ints, double bits as int64, or pointers)
- `$__basic_data_types` array: uint8_t tags (0=INT, 1=DOUBLE, 2=STRING)
- String literals: stored separately as null-terminated C strings in data segment

**String Read Process:**
1. Check type tag → should be 2 (STRING)
2. Read pointer from data array → points to C string in data segment
3. Create StringDescriptor from C string → `string_new_utf8()`
4. Return StringDescriptor* → BASIC code can use it

