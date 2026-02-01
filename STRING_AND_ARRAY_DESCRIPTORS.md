# String and Array Descriptors in FasterBASIC

**Date:** February 1, 2026  
**Purpose:** Document the string and array descriptor system used in FasterBASIC runtime

---

## Overview

FasterBASIC uses **descriptor structures** to manage strings and arrays efficiently. These descriptors provide:

- **Memory management** (reference counting, dynamic allocation)
- **Bounds checking** (array indices)
- **Type information** (encoding, element size)
- **Metadata** (length, capacity, dimensions)

This design enables:
- Efficient substring operations (O(1) character access)
- Safe array access with bounds checking
- Dynamic resizing (REDIM, REDIM PRESERVE)
- Mixed ASCII/UTF-32 string support

---

## String Descriptors

### Structure Definition

Located in: `fsh/FasterBASICT/runtime_c/string_descriptor.h`

```c
typedef struct {
    void*     data;        // Character data (uint8_t* for ASCII, uint32_t* for UTF-32)
    int64_t   length;      // Length in characters (not bytes)
    int64_t   capacity;    // Allocated capacity in characters
    int32_t   refcount;    // Reference count for sharing
    uint8_t   encoding;    // STRING_ENCODING_ASCII or STRING_ENCODING_UTF32
    uint8_t   dirty;       // UTF-8 cache is invalid
    uint8_t   _padding[2]; // Alignment
    char*     utf8_cache;  // Cached UTF-8 string (for C interop)
} StringDescriptor;
```

**Size:** 40 bytes (aligned)

### Encoding Types

```c
typedef enum {
    STRING_ENCODING_ASCII = 0,   // 7-bit ASCII, 1 byte per character
    STRING_ENCODING_UTF32 = 1    // UTF-32, 4 bytes per character
} StringEncoding;
```

**Automatic Encoding Selection:**
- Pure ASCII strings (all bytes < 128) → ASCII encoding (memory efficient)
- Contains non-ASCII → UTF-32 encoding (Unicode support)

### Memory Layout

```
Offset  Size  Field          Description
------  ----  -------------  ----------------------------------
0       8     data           Pointer to character data
8       8     length         Length in characters
16      8     capacity       Allocated capacity in characters
24      4     refcount       Reference count (for sharing)
28      1     encoding       STRING_ENCODING_ASCII or UTF32
29      1     dirty          UTF-8 cache needs refresh
30      2     _padding       Alignment padding
32      8     utf8_cache     Cached UTF-8 C string (or NULL)
------  ----
Total: 40 bytes
```

### Key Features

#### 1. Dual Encoding System

**ASCII Mode (7-bit):**
- 1 byte per character
- Direct byte access: `data[index]`
- Memory efficient for English text
- Promotes to UTF-32 if non-ASCII character added

**UTF-32 Mode:**
- 4 bytes per character
- O(1) character access: `((uint32_t*)data)[index]`
- Supports full Unicode
- Fixed-width encoding (no byte scanning needed)

#### 2. Reference Counting

```c
StringDescriptor* string_retain(StringDescriptor* str);   // Increment refcount
void string_release(StringDescriptor* str);               // Decrement, free if 0
```

**Benefits:**
- Copy-on-write semantics
- Efficient string passing
- Automatic memory management

#### 3. Lazy UTF-8 Conversion

```c
const char* string_to_utf8(StringDescriptor* str);
```

- UTF-8 cache generated only when needed (for C interop)
- Cached result stored in `utf8_cache`
- Invalidated when string modified (`dirty` flag)
- Reduces conversion overhead

### String Operations

#### Creation

```c
// From C string (auto-detects ASCII vs UTF-32)
StringDescriptor* string_new_utf8(const char* utf8_str);

// Force ASCII (for pure 7-bit strings)
StringDescriptor* string_new_ascii(const char* ascii_str);

// From UTF-32 data
StringDescriptor* string_new_utf32(const uint32_t* data, int64_t length);

// Empty string with capacity
StringDescriptor* string_new_capacity(int64_t capacity);
```

#### Access

```c
// O(1) character access
uint32_t string_char_at(const StringDescriptor* str, int64_t index);
bool string_set_char(StringDescriptor* str, int64_t index, uint32_t codepoint);

// Length
int64_t string_length(const StringDescriptor* str);
```

#### Manipulation

```c
// Concatenation (A$ + B$)
StringDescriptor* string_concat(const StringDescriptor* a, const StringDescriptor* b);

// Substring operations (all O(n) with memcpy)
StringDescriptor* string_mid(const StringDescriptor* str, int64_t start, int64_t length);
StringDescriptor* string_left(const StringDescriptor* str, int64_t count);
StringDescriptor* string_right(const StringDescriptor* str, int64_t count);

// Search (INSTR)
int64_t string_instr(const StringDescriptor* haystack, const StringDescriptor* needle, int64_t start);

// Case conversion
StringDescriptor* string_upper(const StringDescriptor* str);
StringDescriptor* string_lower(const StringDescriptor* str);

// Trimming
StringDescriptor* string_trim(const StringDescriptor* str);
StringDescriptor* string_ltrim(const StringDescriptor* str);
StringDescriptor* string_rtrim(const StringDescriptor* str);
```

#### Comparison

```c
// Case-sensitive
int string_compare(const StringDescriptor* a, const StringDescriptor* b);
bool string_equals(const StringDescriptor* a, const StringDescriptor* b);

// Case-insensitive
int string_compare_nocase(const StringDescriptor* a, const StringDescriptor* b);
```

### Runtime Functions

From `basic_runtime.h`:

```c
// Print string descriptor
void basic_print_string_desc(StringDescriptor* desc);

// Input line as string descriptor
StringDescriptor* basic_input_line(void);

// INKEY$ function
StringDescriptor* basic_inkey(void);

// LINE INPUT with prompt
StringDescriptor* basic_line_input(const char* prompt);

// String conversions
StringDescriptor* string_from_int(int64_t value);
StringDescriptor* string_from_double(double value);
```

---

## Array Descriptors

### Structure Definition

Located in: `fsh/FasterBASICT/runtime_c/array_descriptor.h`

```c
typedef struct {
    void*    data;          // Pointer to the array data
    int64_t  lowerBound1;   // Lower index bound for dimension 1
    int64_t  upperBound1;   // Upper index bound for dimension 1
    int64_t  lowerBound2;   // Lower index bound for dimension 2 (0 if 1D)
    int64_t  upperBound2;   // Upper index bound for dimension 2 (0 if 1D)
    int64_t  elementSize;   // Size per element in bytes
    int32_t  dimensions;    // Number of dimensions (1 or 2)
    int32_t  base;          // OPTION BASE (0 or 1)
    char     typeSuffix;    // BASIC type suffix; 0 for UDT/opaque
    char     _padding[7];   // Padding for alignment / future use
} ArrayDescriptor;
```

**Size:** 64 bytes (aligned)

### Memory Layout

```
Offset  Size  Field          Description
------  ----  -------------  ----------------------------------
0       8     data           Pointer to array data
8       8     lowerBound1    Lower bound dimension 1
16      8     upperBound1    Upper bound dimension 1
24      8     lowerBound2    Lower bound dimension 2 (0 if 1D)
32      8     upperBound2    Upper bound dimension 2 (0 if 1D)
40      8     elementSize    Bytes per element
48      4     dimensions     1 or 2
52      4     base           OPTION BASE (0 or 1)
56      1     typeSuffix     '%', '!', '#', '$', '&', or 0
57      7     _padding       Reserved
------  ----
Total: 64 bytes
```

### Key Features

#### 1. Flexible Bounds

**OPTION BASE Support:**
- Base 0: Arrays start at index 0
- Base 1: Arrays start at index 1 (traditional BASIC)

**Example:**
```basic
OPTION BASE 1
DIM A(10) AS INTEGER    ' Indices 1..10
```

Descriptor:
```c
lowerBound1 = 1
upperBound1 = 10
base = 1
```

#### 2. Multi-Dimensional Arrays

**1D Array:**
```basic
DIM A(100) AS INTEGER
```

Descriptor:
```c
dimensions = 1
lowerBound1 = 1
upperBound1 = 100
lowerBound2 = 0
upperBound2 = 0
```

**2D Array:**
```basic
DIM M(10, 20) AS DOUBLE
```

Descriptor:
```c
dimensions = 2
lowerBound1 = 1
upperBound1 = 10
lowerBound2 = 1
upperBound2 = 20
```

**Index Calculation (Row-Major Order):**
```c
// For 2D array: index = (row - lowerBound1) * (upperBound2 - lowerBound2 + 1) + (col - lowerBound2)
offset = ((i1 - desc->lowerBound1) * (desc->upperBound2 - desc->lowerBound2 + 1) +
          (i2 - desc->lowerBound2)) * desc->elementSize;
element_ptr = (char*)desc->data + offset;
```

#### 3. Type Suffix Support

```c
char typeSuffix;  // '%' = INTEGER, '!' = SINGLE, '#' = DOUBLE, '$' = STRING, etc.
```

### Array Operations

#### Initialization

```c
// 1D array
int array_descriptor_init(
    ArrayDescriptor* desc,
    int64_t lowerBound,
    int64_t upperBound,
    int64_t elementSize,
    int32_t base,
    char typeSuffix);

// 2D array
int array_descriptor_init_2d(
    ArrayDescriptor* desc,
    int64_t lowerBound1,
    int64_t upperBound1,
    int64_t lowerBound2,
    int64_t upperBound2,
    int64_t elementSize,
    int32_t base,
    char typeSuffix);
```

#### Memory Management

```c
// Free array data
void array_descriptor_free(ArrayDescriptor* desc);

// REDIM (allocate new, lose old data)
int array_descriptor_redim(ArrayDescriptor* desc, int64_t newLowerBound, int64_t newUpperBound);

// REDIM PRESERVE (allocate new, copy old data)
int array_descriptor_redim_preserve(ArrayDescriptor* desc, int64_t newLowerBound, int64_t newUpperBound);
```

#### Element Access

```c
// Get element address (with bounds checking)
void* array_get_element_address(ArrayDescriptor* desc, int32_t* indices);

// Type-specific accessors
int32_t array_get_int(ArrayDescriptor* array, int32_t* indices);
void array_set_int(ArrayDescriptor* array, int32_t* indices, int32_t value);

double array_get_double(ArrayDescriptor* array, int32_t* indices);
void array_set_double(ArrayDescriptor* array, int32_t* indices, double value);

StringDescriptor* array_get_string(ArrayDescriptor* array, int32_t* indices);
void array_set_string(ArrayDescriptor* array, int32_t* indices, StringDescriptor* value);
```

---

## Code Generation Integration

### String Constants

**Problem:** String literals must be emitted as global data sections in QBE.

**Correct QBE IL:**
```qbe
# Global data section (outside functions)
data $str_0 = { b "Hello World", b 0 }

# Function references the global
export function w $main() {
@start
    %s =l call $string_new_utf8(l $str_0)
    call $basic_print_string_desc(l %s)
    ret 0
}
```

**Incorrect (current issue):**
```qbe
export function w $main() {
@start
    data $str_0 = { b "Hello", b 0 }  # ❌ INVALID - data inside function
    %s =l call $string_new_utf8(l $str_0)
    ret 0
}
```

### String Constant Pool

**Solution:** Three-pass approach

**Pass 1: Collection**
```cpp
// Scan AST for all string literals
void collectStringLiterals(const Program* program) {
    for (auto& line : program->lines) {
        for (auto& stmt : line->statements) {
            visitStatement(stmt.get());  // Recursively find strings
        }
    }
}
```

**Pass 2: Emission**
```cpp
// Emit all strings as global data
void emitStringPool() {
    for (const auto& [text, label] : stringPool_) {
        builder_->emitGlobalData(label, "b", escapeString(text));
    }
}
```

**Pass 3: Reference**
```cpp
// Use pre-defined labels in function code
std::string emitStringLiteral(const std::string& text) {
    std::string label = stringPool_[text];  // Look up pre-defined label
    std::string result = builder_->newTemp();
    builder_->emitCall(result, "l", "$string_new_utf8", {"l $" + label});
    return result;
}
```

### Array Allocation

**QBE IL Pattern:**
```qbe
# Allocate array descriptor (global or local)
data $array_A = { l 0, l 1, l 10, l 0, l 0, l 4, w 1, w 1, b 37, b 0, b 0, b 0, b 0, b 0, b 0, b 0 }

# In function: Initialize array
@start
    %arr =l add $array_A, 0
    %size =l mul 10, 4          # 10 elements * 4 bytes
    %data =l call $malloc(l %size)
    storel %data, %arr           # Store data pointer
    # ... initialize bounds, etc.
```

### Array Access Pattern

**BASIC Code:**
```basic
DIM A(100) AS INTEGER
A(5) = 42
PRINT A(5)
```

**QBE IL:**
```qbe
# Load array descriptor address
%arr =l add $array_A, 0

# Calculate index offset
%idx =l copy 5
%lb =l loadl %arr[8]             # Load lowerBound1
%adjusted =l sub %idx, %lb       # index - lowerBound
%elem_size =l loadl %arr[40]     # Load elementSize
%offset =l mul %adjusted, %elem_size

# Get element address
%data =l loadl %arr[0]           # Load data pointer
%elem_addr =l add %data, %offset

# Store value
storew 42, %elem_addr
```

---

## Type Sizes

### Primitive Types

| BASIC Type  | Suffix | QBE Type | Size | C Type    | Element Size |
|-------------|--------|----------|------|-----------|--------------|
| BYTE        | (none) | w        | 1    | int8_t    | 1            |
| SHORT       | (none) | w        | 2    | int16_t   | 2            |
| INTEGER     | %      | w        | 4    | int32_t   | 4            |
| LONG        | &      | l        | 8    | int64_t   | 8            |
| SINGLE      | !      | s        | 4    | float     | 4            |
| DOUBLE      | #      | d        | 8    | double    | 8            |
| STRING      | $      | l        | 8    | StringDesc* | 8          |

### Descriptor Sizes

| Type              | Size  | Alignment |
|-------------------|-------|-----------|
| StringDescriptor  | 40    | 8         |
| ArrayDescriptor   | 64    | 8         |

---

## Best Practices

### String Handling

1. **Always use runtime functions** for string operations:
   ```qbe
   %s1 =l call $string_new_utf8(l $str_0)
   %s2 =l call $string_new_utf8(l $str_1)
   %result =l call $string_concat(l %s1, l %s2)
   call $string_release(l %s1)
   call $string_release(l %s2)
   ```

2. **Reference counting:**
   - `string_retain()` when copying pointer
   - `string_release()` when done with string
   - Prevents memory leaks

3. **Prefer ASCII when possible:**
   - Automatically selected by `string_new_utf8()`
   - 4x memory savings for English text

### Array Handling

1. **Always bounds check** (runtime handles this):
   ```c
   void* array_get_element_address(ArrayDescriptor* desc, int32_t* indices) {
       // Checks: dimensions, bounds, etc.
       if (indices[0] < desc->lowerBound1 || indices[0] > desc->upperBound1) {
           fprintf(stderr, "Array subscript out of range\n");
           exit(1);
       }
       // ...
   }
   ```

2. **Zero-initialize arrays** (done by `array_descriptor_init`)

3. **Use correct element size:**
   ```c
   elementSize = typeManager_.getTypeSize(arraySymbol.elementTypeDesc.baseType);
   ```

### Memory Safety

1. **Null checks:**
   ```c
   if (!desc || !desc->data) {
       return NULL;  // or error
   }
   ```

2. **Reference counting:**
   - Prevents use-after-free
   - Enables efficient copying

3. **Bounds checking:**
   - Prevents buffer overruns
   - Catches BASIC index errors

---

## Performance Characteristics

### Strings

| Operation        | ASCII     | UTF-32    | Notes                        |
|------------------|-----------|-----------|------------------------------|
| Character access | O(1)      | O(1)      | Direct array indexing        |
| Concatenation    | O(n+m)    | O(n+m)    | Single allocation + memcpy   |
| Substring        | O(n)      | O(n)      | memcpy to new string         |
| Length           | O(1)      | O(1)      | Stored in descriptor         |
| INSTR (search)   | O(n*m)    | O(n*m)    | Naive search                 |
| Case conversion  | O(n)      | O(n)      | Single pass                  |

### Arrays

| Operation        | Time      | Notes                        |
|------------------|-----------|------------------------------|
| 1D access        | O(1)      | Simple offset calculation    |
| 2D access        | O(1)      | Row-major offset             |
| Bounds check     | O(1)      | Compare against bounds       |
| Allocation       | O(n)      | malloc + zero-init           |
| REDIM            | O(n)      | Allocate new                 |
| REDIM PRESERVE   | O(n)      | Allocate + copy old data     |

---

## Future Enhancements

### Strings

1. **Small String Optimization (SSO)**
   - Store short strings (≤8 chars) inline in descriptor
   - Eliminate heap allocation for small strings
   - Structure already defined (`StringDescriptorSSO`)

2. **String interning**
   - Share common string literals
   - Reduce memory for duplicate strings

3. **Faster search algorithms**
   - Boyer-Moore for INSTR
   - Optimize common patterns

### Arrays

1. **Multi-dimensional support** (3D, 4D, etc.)
   - Current limit: 2D
   - Requires variable-length bounds arrays

2. **Sparse arrays**
   - Hash-table based for large, sparse arrays
   - Useful for associative arrays

3. **Array slicing**
   - View into array without copying
   - Efficient for large arrays

---

## References

### Source Files

- `fsh/FasterBASICT/runtime_c/string_descriptor.h` - String descriptor API
- `fsh/FasterBASICT/runtime_c/string_utf32.c` - UTF-32 string implementation
- `fsh/FasterBASICT/runtime_c/string_ops.c` - String operations
- `fsh/FasterBASICT/runtime_c/array_descriptor.h` - Array descriptor API
- `fsh/FasterBASICT/runtime_c/array_ops.c` - Array operations
- `fsh/FasterBASICT/runtime_c/basic_runtime.h` - Runtime function declarations

### Documentation

- `CODEGEN_V2_INTEGRATION_STATUS.md` - Integration status
- `CODEGEN_V2_NEXT_ACTIONS.md` - Action plan
- `CODEGEN_V2_READY.md` - Original design document

---

**Last Updated:** February 1, 2026  
**Status:** Descriptors implemented and tested in runtime  
**Next:** Integrate string pool into code generator V2