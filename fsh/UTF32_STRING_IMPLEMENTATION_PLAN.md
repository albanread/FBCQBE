# UTF-32 String Implementation Plan for FasterBASIC QBE

## Executive Summary

This document outlines a complete strategy for implementing UTF-32 (fixed 32-bit code points) string handling in the FasterBASIC QBE compiler with inline code generation for maximum performance. This approach trades memory (4x vs ASCII) for significant performance gains and implementation simplicity.

## Core Benefits of UTF-32

### Performance Advantages
- **O(1) Character Access**: `A$(5)` is just `Base + (5 * 4)` - no scanning required
- **Simple Slicing**: `MID$`, `LEFT$`, `RIGHT$` become `memcpy` operations
- **Fast Pattern Matching**: Every character is the same size, enabling SIMD operations
- **Predictable Performance**: No variable-length character encoding to decode

### Implementation Simplicity
- **Treat Strings Like Arrays**: Reuse array descriptor pattern for strings
- **No Unicode Complexity**: No need for UTF-8/UTF-16 boundary detection
- **Easy Indexing**: No complex iterator or state machines
- **Straightforward Concatenation**: Simple pointer arithmetic

### Trade-offs Accepted
- **Memory Usage**: 4x vs ASCII (acceptable on modern systems)
- **I/O Conversion**: UTF-8 ↔ UTF-32 conversion at system boundaries
- **Cache Efficiency**: Slightly reduced vs UTF-8 (but predictable access patterns help)

## String Descriptor Structure

### Version 1: Basic Descriptor (40 bytes)

```c
typedef struct {
    uint32_t* data;        // Offset 0:  Pointer to UTF-32 code points (8 bytes)
    int64_t   length;      // Offset 8:  Length in code points (8 bytes)
    int64_t   capacity;    // Offset 16: Capacity in code points (8 bytes)
    int32_t   refcount;    // Offset 24: Reference count for sharing (4 bytes)
    uint8_t   dirty;       // Offset 28: UTF-8 cache needs update (1 byte)
    uint8_t   _padding[3]; // Offset 29: Alignment (3 bytes)
    char*     utf8_cache;  // Offset 32: Cached UTF-8 for C interop (8 bytes)
} StringDescriptor;
```

### Memory Layout Rationale

1. **data pointer** (offset 0): Quick access to character data
2. **length** (offset 8): Number of code points (NOT bytes)
3. **capacity** (offset 16): Allocated space for growth optimization
4. **refcount** (offset 24): Reference counting for efficient assignment
5. **dirty flag** (offset 28): Tracks if UTF-8 cache is stale
6. **utf8_cache** (offset 32): Lazy UTF-8 conversion for PRINT/FILE I/O

### Version 2: Small String Optimization (SSO)

For strings ≤8 characters, store inline to avoid heap allocation:

```c
typedef struct {
    union {
        struct {
            uint32_t* data;
            int64_t   length;
            int64_t   capacity;
            int32_t   refcount;
            uint8_t   dirty;
            uint8_t   _padding[3];
            char*     utf8_cache;
        } heap;
        struct {
            uint32_t  inline_data[8];  // 32 bytes inline storage
            uint8_t   length;          // Length (0-8)
            uint8_t   is_sso;          // Always 1 for SSO
        } sso;
    };
} StringDescriptorSSO;
```

**SSO Benefits**:
- Eliminates malloc for short strings (most strings in BASIC are short)
- Improves cache locality
- Reduces heap fragmentation
- Still maintains same external interface

## Inline Code Generation Strategy

### 1. String Declaration (DIM/Variable)

**QBE IL Pattern**:
```qbe
# DIM S$ or S$ = "" generates:
%str_S =l alloc8 40              # Allocate descriptor (40 bytes)

# Initialize to empty string
storel 0, %str_S                 # data = NULL (offset 0)

%len_addr =l add %str_S, 8
storel 0, %len_addr              # length = 0 (offset 8)

%cap_addr =l add %str_S, 16
storel 0, %cap_addr              # capacity = 0 (offset 16)

%ref_addr =l add %str_S, 24
storew 1, %ref_addr              # refcount = 1 (offset 24)

%dirty_addr =l add %str_S, 28
storeb 1, %dirty_addr            # dirty = 1 (offset 28)

%cache_addr =l add %str_S, 32
storel 0, %cache_addr            # utf8_cache = NULL (offset 32)
```

### 2. String Literal Assignment

**BASIC Code**: `S$ = "Hello"`

**QBE IL Pattern**:
```qbe
# 1. Convert UTF-8 literal to UTF-32 at compile time or startup
#    Store in data section as array of 32-bit values
data $str_literal_0 = { w 72, w 101, w 108, w 108, w 111, w 0 }

# 2. Allocate space for string data
%char_count =l copy 5
%byte_count =l mul %char_count, 4
%data_ptr =l call $malloc(l %byte_count)

# 3. Copy literal data
call $memcpy(l %data_ptr, l $str_literal_0, l %byte_count)

# 4. Update descriptor
storel %data_ptr, %str_S         # data pointer

%len_addr =l add %str_S, 8
storel 5, %len_addr              # length = 5

%cap_addr =l add %str_S, 16
storel 5, %cap_addr              # capacity = 5

# 5. Mark as dirty (needs UTF-8 encoding for output)
%dirty_addr =l add %str_S, 28
storeb 1, %dirty_addr
```

### 3. Character Access (READ)

**BASIC Code**: `C = ASC(MID$(S$, 5, 1))` or `C$ = S$(5)`

**QBE IL Pattern**:
```qbe
# Access character at index (0-based internally)
%index =w copy 4                 # Index 5 in BASIC = 4 in 0-based

# Bounds check
%len_addr =l add %str_S, 8
%len =l loadl %len_addr
%index_long =l extsw %index

%check_lower =w csgel %index_long, 0
%check_upper =w csltl %index_long, %len
%check_both =w and %check_lower, %check_upper
jnz %check_both, @bounds_ok, @bounds_err

@bounds_err
    call $basic_string_bounds_error(l %index_long, l 0, l %len)

@bounds_ok
    # Calculate byte offset: index * 4
    %byte_offset =l mul %index_long, 4
    
    # Load data pointer
    %data_ptr =l loadl %str_S
    
    # Get character address
    %char_addr =l add %data_ptr, %byte_offset
    
    # Load character (32-bit code point)
    %char =w loadw %char_addr
```

### 4. Character Access (WRITE)

**BASIC Code**: `MID$(S$, 5, 1) = "X"`

**QBE IL Pattern**:
```qbe
# [Same bounds checking as READ]

# Store character at address
storew %new_char, %char_addr     # Store 32-bit code point

# Mark as dirty
%dirty_addr =l add %str_S, 28
storeb 1, %dirty_addr

# Invalidate UTF-8 cache
%cache_addr =l add %str_S, 32
%old_cache =l loadl %cache_addr
call $free(l %old_cache)
storel 0, %cache_addr
```

### 5. String Concatenation (A$ + B$)

**Inline Code Generation Strategy**: Smart capacity management

```qbe
# Load lengths
%a_len_addr =l add %str_A, 8
%a_len =l loadl %a_len_addr

%b_len_addr =l add %str_B, 8
%b_len =l loadl %b_len_addr

# Calculate total length
%total_len =l add %a_len, %b_len

# Allocate new string descriptor
%result =l call $malloc(l 40)

# Allocate data buffer
%total_bytes =l mul %total_len, 4
%result_data =l call $malloc(l %total_bytes)

# Copy A's data
%a_data =l loadl %str_A
%a_bytes =l mul %a_len, 4
call $memcpy(l %result_data, l %a_data, l %a_bytes)

# Copy B's data
%b_data =l loadl %str_B
%b_bytes =l mul %b_len, 4
%b_dest =l add %result_data, %a_bytes
call $memcpy(l %b_dest, l %b_data, l %b_bytes)

# Initialize result descriptor
storel %result_data, %result     # data
%len_addr =l add %result, 8
storel %total_len, %len_addr     # length
%cap_addr =l add %result, 16
storel %total_len, %cap_addr     # capacity
%ref_addr =l add %result, 24
storew 1, %ref_addr              # refcount = 1
%dirty_addr =l add %result, 28
storeb 1, %dirty_addr            # dirty = 1
%cache_addr =l add %result, 32
storel 0, %cache_addr            # utf8_cache = NULL
```

**Optimization**: Capacity-aware concatenation for loops:
```qbe
# If result has capacity, append in-place:
%cap_addr =l add %result, 16
%capacity =l loadl %cap_addr
%new_total =l add %current_len, %append_len
%has_space =w cslel %new_total, %capacity
jnz %has_space, @append_inplace, @realloc_grow

@append_inplace
    # Just copy and update length
    # [... append code ...]
    
@realloc_grow
    # Call realloc to grow
    # [... realloc code ...]
```

### 6. MID$ Function (Substring)

**BASIC Code**: `R$ = MID$(S$, start, length)`

**QBE IL Pattern**:
```qbe
# Convert BASIC 1-based to 0-based
%start_0based =w sub %start, 1

# Bounds check start
%len_addr =l add %str_S, 8
%str_len =l loadl %len_addr
%start_long =l extsw %start_0based

%check =w csgel %start_long, 0
%check2 =w csltl %start_long, %str_len
%valid =w and %check, %check2
jnz %valid, @start_ok, @error

@start_ok
    # Calculate actual substring length
    %remaining =l sub %str_len, %start_long
    %extract_len =l copy %length
    
    # Clamp to available characters
    %use_remaining =w csgtl %extract_len, %remaining
    jnz %use_remaining, @use_rem, @use_req
    
@use_rem
    %extract_len =l copy %remaining
    
@use_req
    # Allocate result
    %result =l call $malloc(l 40)
    %extract_bytes =l mul %extract_len, 4
    %result_data =l call $malloc(l %extract_bytes)
    
    # Calculate source offset
    %src_offset =l mul %start_long, 4
    %src_data =l loadl %str_S
    %src_ptr =l add %src_data, %src_offset
    
    # Copy substring
    call $memcpy(l %result_data, l %src_ptr, l %extract_bytes)
    
    # Initialize result descriptor
    storel %result_data, %result
    # [... set length, capacity, refcount, etc ...]
```

### 7. LEFT$ and RIGHT$ Functions

**LEFT$(S$, n)** - first n characters:
```qbe
# Same as MID$(S$, 1, n)
%start =w copy 0
# [... use MID$ logic ...]
```

**RIGHT$(S$, n)** - last n characters:
```qbe
# Calculate start: len - n
%len_addr =l add %str_S, 8
%str_len =l loadl %len_addr
%start =l sub %str_len, %n
# [... use MID$ logic ...]
```

### 8. INSTR Function (Find Substring)

**BASIC Code**: `pos = INSTR(haystack$, needle$)`

**QBE IL Pattern** (naive search, can optimize with Boyer-Moore later):
```qbe
# Load haystack length and data
%hay_len =l loadl [...offset 8...]
%hay_data =l loadl %str_haystack

# Load needle length and data
%needle_len =l loadl [...offset 8...]
%needle_data =l loadl %str_needle

# Quick checks
%is_empty =w ceql %needle_len, 0
jnz %is_empty, @return_zero, @search

@search
    %max_pos =l sub %hay_len, %needle_len
    %pos =l copy 0
    
@search_loop
    %done =w csgtl %pos, %max_pos
    jnz %done, @not_found, @check_pos
    
@check_pos
    # Compare characters at current position
    %hay_offset =l mul %pos, 4
    %hay_ptr =l add %hay_data, %hay_offset
    
    # Use memcmp for efficiency
    %needle_bytes =l mul %needle_len, 4
    %cmp =w call $memcmp(l %hay_ptr, l %needle_data, l %needle_bytes)
    %match =w ceqw %cmp, 0
    jnz %match, @found, @next_pos
    
@next_pos
    %pos =l add %pos, 1
    jmp @search_loop
    
@found
    # Convert 0-based to 1-based for BASIC
    %result =w add %pos, 1
    # [... return result ...]
    
@not_found
    %result =w copy 0
    # [... return 0 ...]
```

**Optimization**: For single-character search, use simple loop instead of memcmp.

### 9. String Comparison

**BASIC Code**: `IF A$ = B$ THEN` or `IF A$ < B$ THEN`

**QBE IL Pattern**:
```qbe
# Load lengths
%a_len =l loadl [...%str_A + 8...]
%b_len =l loadl [...%str_B + 8...]

# Quick length check for equality
%same_len =w ceql %a_len, %b_len
jnz %same_len, @compare_data, @not_equal

@compare_data
    # Use memcmp
    %a_data =l loadl %str_A
    %b_data =l loadl %str_B
    %byte_count =l mul %a_len, 4
    %cmp =w call $memcmp(l %a_data, l %b_data, l %byte_count)
    
    # For equality: check if cmp == 0
    # For ordering: use cmp directly (< 0, == 0, > 0)
```

**Note**: `memcmp` on UTF-32 data works correctly for lexicographic ordering.

### 10. PRINT Statement (String Output)

**BASIC Code**: `PRINT S$`

**QBE IL Pattern**: Lazy UTF-8 conversion
```qbe
# Check if UTF-8 cache is valid
%dirty_addr =l add %str_S, 28
%dirty =w loadb %dirty_addr
jnz %dirty, @need_conversion, @use_cache

@use_cache
    %cache_addr =l add %str_S, 32
    %utf8 =l loadl %cache_addr
    call $basic_print_cstr(l %utf8)
    jmp @done

@need_conversion
    # Convert UTF-32 -> UTF-8
    %len =l loadl [...%str_S + 8...]
    %data =l loadl %str_S
    
    # Allocate UTF-8 buffer (worst case: 4 bytes per code point + null)
    %max_utf8 =l mul %len, 4
    %max_utf8_1 =l add %max_utf8, 1
    %utf8_buf =l call $malloc(l %max_utf8_1)
    
    # Call conversion function
    %utf8_len =l call $utf32_to_utf8(l %data, l %len, l %utf8_buf, l %max_utf8_1)
    
    # Cache the result
    %cache_addr =l add %str_S, 32
    storel %utf8_buf, %cache_addr
    
    # Clear dirty flag
    %dirty_addr =l add %str_S, 28
    storeb 0, %dirty_addr
    
    # Print
    call $basic_print_cstr(l %utf8_buf)

@done
```

**Optimization**: For constant strings, convert at compile time and embed UTF-8 directly.

### 11. INPUT Statement (String Input)

**BASIC Code**: `INPUT S$`

**QBE IL Pattern**: UTF-8 -> UTF-32 conversion
```qbe
# Read UTF-8 from console
%utf8_input =l call $basic_input_line()

# Get length in code points
%cp_len =l call $utf8_length_in_codepoints(l %utf8_input)

# Allocate UTF-32 buffer
%utf32_bytes =l mul %cp_len, 4
%utf32_buf =l call $malloc(l %utf32_bytes)

# Convert UTF-8 -> UTF-32
call $utf8_to_utf32(l %utf8_input, l %utf32_buf, l %cp_len)

# Update string descriptor
%old_data =l loadl %str_S
call $free(l %old_data)

storel %utf32_buf, %str_S        # data
%len_addr =l add %str_S, 8
storel %cp_len, %len_addr        # length
%cap_addr =l add %str_S, 16
storel %cp_len, %cap_addr        # capacity

# Mark as dirty
%dirty_addr =l add %str_S, 28
storeb 1, %dirty_addr

# Free input buffer
call $free(l %utf8_input)
```

### 12. CHR$ Function

**BASIC Code**: `C$ = CHR$(65)` → "A"

**QBE IL Pattern**:
```qbe
# Allocate descriptor
%result =l call $malloc(l 40)

# Allocate single character
%data =l call $malloc(l 4)
storew %codepoint, %data

# Initialize descriptor
storel %data, %result            # data
%len_addr =l add %result, 8
storel 1, %len_addr              # length = 1
%cap_addr =l add %result, 16
storel 1, %cap_addr              # capacity = 1
%ref_addr =l add %result, 24
storew 1, %ref_addr              # refcount = 1
%dirty_addr =l add %result, 28
storeb 1, %dirty_addr
%cache_addr =l add %result, 32
storel 0, %cache_addr
```

**Optimization**: Cache common CHR$ values (0-127) at program startup.

### 13. ASC Function

**BASIC Code**: `code = ASC(S$)`

**QBE IL Pattern**:
```qbe
# Check if string is empty
%len =l loadl [...%str_S + 8...]
%is_empty =w ceql %len, 0
jnz %is_empty, @return_zero, @get_first

@return_zero
    %result =w copy 0
    # [... return ...]

@get_first
    %data =l loadl %str_S
    %result =w loadw %data       # Load first character
    # [... return ...]
```

### 14. LEN Function

**BASIC Code**: `length = LEN(S$)`

**QBE IL Pattern**:
```qbe
# Simply load length field
%len_addr =l add %str_S, 8
%length =l loadl %len_addr
%result =w copy %length          # Convert to word if needed
```

### 15. String Assignment (Reference Counting)

**BASIC Code**: `A$ = B$`

**QBE IL Pattern**:
```qbe
# Increment B's refcount
%b_ref_addr =l add %str_B, 24
%b_ref =w loadw %b_ref_addr
%b_ref_new =w add %b_ref, 1
storew %b_ref_new, %b_ref_addr

# Decrement A's refcount
%a_ref_addr =l add %str_A, 24
%a_ref =w loadw %a_ref_addr
%a_ref_new =w sub %a_ref, 1
storew %a_ref_new, %a_ref_addr

# If A's refcount is 0, free it
%should_free =w ceqw %a_ref_new, 0
jnz %should_free, @free_a, @copy_ptr

@free_a
    %a_data =l loadl %str_A
    call $free(l %a_data)
    %a_cache =l loadl [...%str_A + 32...]
    call $free(l %a_cache)

@copy_ptr
    # Copy descriptor contents
    # [... memcpy 40 bytes from B to A ...]
```

**Alternative**: Deep copy instead of reference counting (simpler but more memory).

### 16. VAL Function (String to Number)

**BASIC Code**: `x = VAL("123.45")`

**QBE IL Pattern**:
```qbe
# Get UTF-8 representation
call $string_to_utf8(l %str_S)
%utf8 =l [...load from cache...]

# Call C's strtod
%result =d call $strtod(l %utf8, l 0)
```

### 17. STR$ Function (Number to String)

**BASIC Code**: `S$ = STR$(123.45)`

**QBE IL Pattern**:
```qbe
# Convert number to UTF-8 string
%buf =l call $malloc(l 64)       # Temp buffer
call $snprintf(l %buf, l 64, l $fmt_double, d %value)

# Convert UTF-8 -> UTF-32
%result =l call $string_new_utf8(l %buf)

# Free temp buffer
call $free(l %buf)
```

## UTF-8 ↔ UTF-32 Conversion Functions

These C functions handle the system boundary conversions:

### utf8_to_utf32()

```c
int64_t utf8_to_utf32(const char* utf8_str, uint32_t* out_utf32, int64_t out_capacity) {
    int64_t count = 0;
    const uint8_t* p = (const uint8_t*)utf8_str;
    
    while (*p && count < out_capacity) {
        uint32_t codepoint;
        
        if ((*p & 0x80) == 0) {
            // 1-byte: 0xxxxxxx
            codepoint = *p;
            p += 1;
        } else if ((*p & 0xE0) == 0xC0) {
            // 2-byte: 110xxxxx 10xxxxxx
            codepoint = ((*p & 0x1F) << 6) | (p[1] & 0x3F);
            p += 2;
        } else if ((*p & 0xF0) == 0xE0) {
            // 3-byte: 1110xxxx 10xxxxxx 10xxxxxx
            codepoint = ((*p & 0x0F) << 12) | ((p[1] & 0x3F) << 6) | (p[2] & 0x3F);
            p += 3;
        } else if ((*p & 0xF8) == 0xF0) {
            // 4-byte: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
            codepoint = ((*p & 0x07) << 18) | ((p[1] & 0x3F) << 12) | 
                       ((p[2] & 0x3F) << 6) | (p[3] & 0x3F);
            p += 4;
        } else {
            // Invalid UTF-8, skip byte
            p += 1;
            continue;
        }
        
        out_utf32[count++] = codepoint;
    }
    
    return count;
}
```

### utf32_to_utf8()

```c
int64_t utf32_to_utf8(const uint32_t* utf32_data, int64_t length, 
                      char* out_utf8, int64_t out_capacity) {
    int64_t written = 0;
    
    for (int64_t i = 0; i < length; i++) {
        uint32_t cp = utf32_data[i];
        
        if (cp < 0x80) {
            // 1 byte
            if (written + 1 >= out_capacity) break;
            out_utf8[written++] = (char)cp;
        } else if (cp < 0x800) {
            // 2 bytes
            if (written + 2 >= out_capacity) break;
            out_utf8[written++] = 0xC0 | (cp >> 6);
            out_utf8[written++] = 0x80 | (cp & 0x3F);
        } else if (cp < 0x10000) {
            // 3 bytes
            if (written + 3 >= out_capacity) break;
            out_utf8[written++] = 0xE0 | (cp >> 12);
            out_utf8[written++] = 0x80 | ((cp >> 6) & 0x3F);
            out_utf8[written++] = 0x80 | (cp & 0x3F);
        } else if (cp < 0x110000) {
            // 4 bytes
            if (written + 4 >= out_capacity) break;
            out_utf8[written++] = 0xF0 | (cp >> 18);
            out_utf8[written++] = 0x80 | ((cp >> 12) & 0x3F);
            out_utf8[written++] = 0x80 | ((cp >> 6) & 0x3F);
            out_utf8[written++] = 0x80 | (cp & 0x3F);
        }
    }
    
    out_utf8[written] = '\0';
    return written + 1;  // Include null terminator
}
```

## Memory Management Strategies

### Strategy 1: Reference Counting (Recommended)

**Benefits**:
- Efficient for `A$ = B$` assignments (just increment refcount)
- Avoids copying large strings
- Automatic cleanup when refcount reaches 0

**Drawbacks**:
- Need to track refcounts carefully
- Can't modify shared strings (copy-on-write needed)

**Implementation**:
```qbe
# On assignment: A$ = B$
%ref =w loadw [...B$ + 24...]
%ref_new =w add %ref, 1
storew %ref_new, [...B$ + 24...]
# Copy descriptor A$ = B$
```

### Strategy 2: Deep Copy

**Benefits**:
- Simpler implementation (no refcount tracking)
- Each string is independent (can modify freely)

**Drawbacks**:
- More memory usage
- Slower assignments

**Implementation**:
```qbe
# On assignment: A$ = B$
%len =l loadl [...B$ + 8...]
%bytes =l mul %len, 4
%new_data =l call $malloc(l %bytes)
%b_data =l loadl B$
call $memcpy(l %new_data, l %b_data, l %bytes)
# ... update A$ descriptor with new_data ...
```

### Strategy 3: Copy-on-Write (COW)

Hybrid: share data until modification, then copy.

**Benefits**:
- Fast assignments (like refcount)
- Safe modifications (automatic copy)

**Drawbacks**:
- More complex implementation
- Need to check refcount before every write

## Optimization Techniques

### 1. Capacity Over-Allocation

When growing strings, allocate extra capacity:
```c
new_capacity = max(required_capacity, current_capacity * 1.5);
```

This amortizes allocation cost for repeated concatenations.

### 2. String Literal Pool

Store string literals in data section as UTF-32:
```qbe
data $str_0 = { w 72, w 101, w 108, w 108, w 111, w 0 }  # "Hello"
```

Create descriptors on first use, cache for reuse.

### 3. Empty String Singleton

Use single shared empty string descriptor:
```qbe
data $empty_string = { l 0, l 0, l 0, w 0, b 0, b 0, b 0, b 0, l 0 }
```

### 4. Common Character Cache

Pre-allocate CHR$(0) through CHR$(127) at startup.

### 5. SIMD String Operations

For operations like search, comparison, and case conversion, use SIMD:
```qbe
# Use QBE vector types when available
# or call optimized C functions that use SSE/AVX
```

### 6. Lazy UTF-8 Conversion

Only convert to UTF-8 when actually printing/saving:
- Keep `dirty` flag in descriptor
- Cache UTF-8 result in descriptor
- Invalidate cache on string modification

### 7. Small String Optimization (SSO)

Store strings ≤8 characters inline in descriptor:
```c
if (length <= 8) {
    // Store in descriptor.sso.inline_data
} else {
    // Allocate heap
}
```

## Testing Strategy

### Unit Tests

1. **Basic Operations**:
   - Empty string creation
   - Single character string
   - Long string (>8 chars to avoid SSO)
   - String with Unicode characters

2. **Concatenation**:
   - `A$ + B$` with various lengths
   - Repeated concatenation (test capacity growth)
   - Concatenation with empty strings

3. **Substring Operations**:
   - `MID$` with various start/length
   - `LEFT$` and `RIGHT$`
   - Edge cases (index 0, beyond length)

4. **Search**:
   - `INSTR` finding substring
   - Not found case
   - Empty needle/haystack

5. **Comparison**:
   - Equal strings
   - Ordering (< > = <>)
   - Case sensitivity

6. **Conversion**:
   - UTF-8 → UTF-32 → UTF-8 round-trip
   - ASCII strings
   - Multi-byte UTF-8 characters
   - Invalid UTF-8 handling

7. **Memory Management**:
   - Assignment (refcount or copy)
   - Cleanup on scope exit
   - No leaks in loops

### Integration Tests

1. **String Array**:
   ```basic
   DIM Names$(100)
   FOR i = 0 TO 100
       Names$(i) = "Name_" + STR$(i)
   NEXT
   ```

2. **Text Processing**:
   ```basic
   S$ = "The quick brown fox"
   PRINT LEN(S$)
   PRINT LEFT$(S$, 3)
   PRINT MID$(S$, 5, 5)
   PRINT RIGHT$(S$, 3)
   ```

3. **File I/O**:
   ```basic
   OPEN "test.txt" FOR OUTPUT AS #1
   PRINT #1, "Hello, World!"
   CLOSE #1
   ```

4. **INPUT Statement**:
   ```basic
   PRINT "Enter your name: ";
   INPUT Name$
   PRINT "Hello, "; Name$
   ```

### Performance Benchmarks

1. **Concatenation Loop**:
   ```basic
   S$ = ""
   FOR i = 1 TO 10000
       S$ = S$ + "x"
   NEXT
   ```
   Measure time vs runtime-based approach.

2. **Character Access**:
   ```basic
   DIM S$ = STRING$(10000, "A")
   FOR i = 1 TO 10000
       c = ASC(MID$(S$, i, 1))
   NEXT
   ```
   Should be O(1) per access.

3. **Search**:
   ```basic
   Haystack$ = STRING$(100000, "abcd")
   pos = INSTR(Haystack$, "xyz")
   ```

## Implementation Phases

### Phase 1: Core Infrastructure (Week 1)
- [ ] Define StringDescriptor structure
- [ ] Implement UTF-8 ↔ UTF-32 conversion functions
- [ ] Create string_new(), string_release() helpers
- [ ] Basic memory management (malloc/free)

### Phase 2: Basic Operations (Week 1-2)
- [ ] String literal assignment codegen
- [ ] LEN() function
- [ ] Character access (ASC, CHR$)
- [ ] String concatenation (+)
- [ ] PRINT statement (with UTF-8 conversion)

### Phase 3: Substring Operations (Week 2)
- [ ] MID$ function
- [ ] LEFT$ function  
- [ ] RIGHT$ function
- [ ] Bounds checking

### Phase 4: Search and Compare (Week 3)
- [ ] INSTR function
- [ ] String comparison (=, <>, <, >, <=, >=)
- [ ] Case-insensitive comparison

### Phase 5: Conversions (Week 3)
- [ ] VAL function (string → number)
- [ ] STR$ function (number → string)
- [ ] LCASE$, UCASE$ functions
- [ ] LTRIM$, RTRIM$, TRIM$ functions

### Phase 6: I/O Integration (Week 4)
- [ ] INPUT statement (UTF-8 input)
- [ ] FILE I/O with UTF-8 conversion
- [ ] PRINT #n statement

### Phase 7: Optimizations (Week 4)
- [ ] Small String Optimization (SSO)
- [ ] Capacity over-allocation
- [ ] String literal pooling
- [ ] Common character cache
- [ ] Lazy UTF-8 conversion

### Phase 8: Advanced Features (Week 5+)
- [ ] Reference counting or COW
- [ ] SIMD operations for search/compare
- [ ] Regex support (optional)
- [ ] Unicode normalization (optional)

## Performance Expectations

### Memory Usage
- **Descriptor**: 40 bytes per string
- **Data**: 4 bytes per character
- **Example**: "Hello" (5 chars) = 40 + 20 = 60 bytes
- **vs UTF-8**: "Hello" = 5 bytes + overhead
- **Trade-off**: 10-12x memory for simple strings, but constant time access

### Speed Improvements vs Runtime Calls
- **Character access**: ~5x faster (no function call, direct load)
- **MID$ operation**: ~3x faster (simple memcpy vs scan + extract)
- **Concatenation**: ~2x faster (especially with capacity management)
- **INSTR**: ~4x faster (memcmp vs byte-by-byte scan)

### Expected Overhead
- **UTF-8 conversion**: ~100-200 cycles per character (only at I/O boundaries)
- **Bounds checking**: ~10-20 cycles (branch prediction helps)
- **Cache misses**: Slightly more than UTF-8 due to 4x size

## Alternative Approaches Considered

### Approach 1: UTF-8 Internal (Variable Width)
**Rejected because**:
- O(n) character access (must scan from start)
- Complex substring operations
- Difficult to implement efficiently in QBE IL

### Approach 2: UTF-16 Internal
**Rejected because**:
- Still variable-width (1-2 units per character)
- Worst of both worlds (not ASCII-compatible, not fixed-width)
- Rare in modern systems

### Approach 3: Mixed UTF-8/UTF-32
**Rejected because**:
- Complex to implement (need two code paths)
- Difficult to decide which representation to use
- Conversion overhead between representations

## Conclusion

The UTF-32 internal representation with inline QBE code generation provides:

✅ **Simplicity**: Treat strings like arrays of 32-bit integers
✅ **Performance**: O(1) access, simple operations, predictable behavior
✅ **Safety**: Bounds checking on every access
✅ **Compatibility**: UTF-8 conversion at system boundaries
✅ **Maintainability**: Clear descriptor structure, reusable patterns

The 4x memory trade-off is acceptable for modern systems where RAM is abundant, and the performance gains from O(1) access and simplified operations far outweigh the memory cost for typical BASIC programs.

**Recommendation**: Proceed with UTF-32 implementation, starting with Phase 1 (core infrastructure) and Phase 2 (basic operations) to validate the approach with real code generation and testing.