# UTF-32 String Implementation - Quick Reference

## Core Concept

Store strings as **arrays of 32-bit Unicode code points** (UTF-32) internally, convert to/from UTF-8 only at I/O boundaries.

## String Descriptor (40 bytes)

```c
typedef struct {
    uint32_t* data;        // Offset 0:  UTF-32 code points
    int64_t   length;      // Offset 8:  Length in code points
    int64_t   capacity;    // Offset 16: Allocated capacity
    int32_t   refcount;    // Offset 24: Reference count
    uint8_t   dirty;       // Offset 28: UTF-8 cache invalid flag
    uint8_t   _padding[3]; // Offset 29: Alignment
    char*     utf8_cache;  // Offset 32: Cached UTF-8 for I/O
} StringDescriptor;
```

## Key Benefits

✅ **O(1) character access**: `A$(5)` = `data + (5 * 4)` - no scanning  
✅ **Simple slicing**: MID$, LEFT$, RIGHT$ = memcpy operations  
✅ **Fast search**: memcmp works on fixed-width data  
✅ **Easy indexing**: No UTF-8 boundary detection needed  

❌ **4x memory vs ASCII** (acceptable trade-off)  
❌ **UTF-8 conversion at I/O** (cached to minimize overhead)  

## QBE IL Patterns

### 1. String Declaration
```qbe
%str_S =l alloc8 40              # Allocate descriptor
storel 0, %str_S                 # data = NULL
%len_addr =l add %str_S, 8
storel 0, %len_addr              # length = 0
%cap_addr =l add %str_S, 16
storel 0, %cap_addr              # capacity = 0
%ref_addr =l add %str_S, 24
storew 1, %ref_addr              # refcount = 1
%dirty_addr =l add %str_S, 28
storeb 1, %dirty_addr            # dirty = 1
%cache_addr =l add %str_S, 32
storel 0, %cache_addr            # utf8_cache = NULL
```

### 2. Character Access (Read)
```qbe
# A$(i) or ASC(MID$(A$, i, 1))
%index_long =l extsw %index      # Convert to long
%byte_offset =l mul %index_long, 4
%data_ptr =l loadl %str_S
%char_addr =l add %data_ptr, %byte_offset
%char =w loadw %char_addr        # Load 32-bit code point
```

### 3. Character Access (Write)
```qbe
# MID$(A$, i, 1) = "X"
storew %new_char, %char_addr
%dirty_addr =l add %str_S, 28
storeb 1, %dirty_addr            # Mark UTF-8 cache invalid
```

### 4. Concatenation (A$ + B$)
```qbe
# Calculate total length
%a_len =l loadl [...offset 8...]
%b_len =l loadl [...offset 8...]
%total_len =l add %a_len, %b_len

# Allocate result data
%total_bytes =l mul %total_len, 4
%result_data =l call $malloc(l %total_bytes)

# Copy A's data
%a_data =l loadl %str_A
%a_bytes =l mul %a_len, 4
call $memcpy(l %result_data, l %a_data, l %a_bytes)

# Copy B's data after A
%b_data =l loadl %str_B
%b_bytes =l mul %b_len, 4
%b_dest =l add %result_data, %a_bytes
call $memcpy(l %b_dest, l %b_data, l %b_bytes)

# Initialize result descriptor
# [... set data, length, capacity, refcount, dirty, cache ...]
```

### 5. MID$ (Substring)
```qbe
# MID$(S$, start, length)
# Convert BASIC 1-based to 0-based
%start_0 =w sub %start, 1
%start_long =l extsw %start_0

# Calculate source offset
%src_offset =l mul %start_long, 4
%src_data =l loadl %str_S
%src_ptr =l add %src_data, %src_offset

# Allocate result
%extract_bytes =l mul %extract_len, 4
%result_data =l call $malloc(l %extract_bytes)

# Copy substring
call $memcpy(l %result_data, l %src_ptr, l %extract_bytes)
```

### 6. INSTR (Find Substring)
```qbe
# INSTR(haystack$, needle$)
@search_loop
    # Calculate position in bytes
    %hay_offset =l mul %pos, 4
    %hay_ptr =l add %hay_data, %hay_offset
    
    # Compare using memcmp
    %needle_bytes =l mul %needle_len, 4
    %cmp =w call $memcmp(l %hay_ptr, l %needle_data, l %needle_bytes)
    %match =w ceqw %cmp, 0
    jnz %match, @found, @next_pos
    
@next_pos
    %pos =l add %pos, 1
    jmp @search_loop
```

### 7. String Comparison
```qbe
# IF A$ = B$ THEN
%a_len =l loadl [...offset 8...]
%b_len =l loadl [...offset 8...]

# Quick length check
%same_len =w ceql %a_len, %b_len
jnz %same_len, @compare_data, @not_equal

@compare_data
    %a_data =l loadl %str_A
    %b_data =l loadl %str_B
    %byte_count =l mul %a_len, 4
    %cmp =w call $memcmp(l %a_data, l %b_data, l %byte_count)
    # cmp == 0 means equal
```

### 8. PRINT (Output with UTF-8 Conversion)
```qbe
# PRINT S$
%dirty_addr =l add %str_S, 28
%dirty =w loadb %dirty_addr
jnz %dirty, @need_conversion, @use_cache

@use_cache
    %cache_addr =l add %str_S, 32
    %utf8 =l loadl %cache_addr
    call $basic_print_cstr(l %utf8)
    jmp @done

@need_conversion
    %len =l loadl [...offset 8...]
    %data =l loadl %str_S
    
    # Allocate UTF-8 buffer (4 bytes per code point max + null)
    %max_utf8 =l mul %len, 4
    %max_utf8_1 =l add %max_utf8, 1
    %utf8_buf =l call $malloc(l %max_utf8_1)
    
    # Convert UTF-32 -> UTF-8
    call $utf32_to_utf8(l %data, l %len, l %utf8_buf, l %max_utf8_1)
    
    # Cache result
    %cache_addr =l add %str_S, 32
    storel %utf8_buf, %cache_addr
    storeb 0, [...dirty_addr...]
    
    # Print
    call $basic_print_cstr(l %utf8_buf)
```

### 9. INPUT (Input with UTF-8 Conversion)
```qbe
# INPUT S$
%utf8_input =l call $basic_input_line()

# Convert UTF-8 -> UTF-32
%cp_len =l call $utf8_length_in_codepoints(l %utf8_input)
%utf32_bytes =l mul %cp_len, 4
%utf32_buf =l call $malloc(l %utf32_bytes)
call $utf8_to_utf32(l %utf8_input, l %utf32_buf, l %cp_len)

# Update string descriptor
%old_data =l loadl %str_S
call $free(l %old_data)
storel %utf32_buf, %str_S
%len_addr =l add %str_S, 8
storel %cp_len, %len_addr
```

## UTF-8 ↔ UTF-32 Conversion (C Runtime)

### UTF-8 → UTF-32
```c
int64_t utf8_to_utf32(const char* utf8_str, uint32_t* out_utf32, int64_t capacity) {
    int64_t count = 0;
    const uint8_t* p = (const uint8_t*)utf8_str;
    
    while (*p && count < capacity) {
        uint32_t cp;
        if ((*p & 0x80) == 0) {           // 1-byte: 0xxxxxxx
            cp = *p; p += 1;
        } else if ((*p & 0xE0) == 0xC0) { // 2-byte: 110xxxxx 10xxxxxx
            cp = ((*p & 0x1F) << 6) | (p[1] & 0x3F); p += 2;
        } else if ((*p & 0xF0) == 0xE0) { // 3-byte
            cp = ((*p & 0x0F) << 12) | ((p[1] & 0x3F) << 6) | (p[2] & 0x3F); p += 3;
        } else if ((*p & 0xF8) == 0xF0) { // 4-byte
            cp = ((*p & 0x07) << 18) | ((p[1] & 0x3F) << 12) | 
                 ((p[2] & 0x3F) << 6) | (p[3] & 0x3F); p += 4;
        } else {
            p++; continue; // Invalid UTF-8
        }
        out_utf32[count++] = cp;
    }
    return count;
}
```

### UTF-32 → UTF-8
```c
int64_t utf32_to_utf8(const uint32_t* utf32_data, int64_t length,
                      char* out_utf8, int64_t capacity) {
    int64_t written = 0;
    for (int64_t i = 0; i < length; i++) {
        uint32_t cp = utf32_data[i];
        if (cp < 0x80) {         // 1 byte
            if (written + 1 >= capacity) break;
            out_utf8[written++] = (char)cp;
        } else if (cp < 0x800) { // 2 bytes
            if (written + 2 >= capacity) break;
            out_utf8[written++] = 0xC0 | (cp >> 6);
            out_utf8[written++] = 0x80 | (cp & 0x3F);
        } else if (cp < 0x10000) { // 3 bytes
            if (written + 3 >= capacity) break;
            out_utf8[written++] = 0xE0 | (cp >> 12);
            out_utf8[written++] = 0x80 | ((cp >> 6) & 0x3F);
            out_utf8[written++] = 0x80 | (cp & 0x3F);
        } else if (cp < 0x110000) { // 4 bytes
            if (written + 4 >= capacity) break;
            out_utf8[written++] = 0xF0 | (cp >> 18);
            out_utf8[written++] = 0x80 | ((cp >> 12) & 0x3F);
            out_utf8[written++] = 0x80 | ((cp >> 6) & 0x3F);
            out_utf8[written++] = 0x80 | (cp & 0x3F);
        }
    }
    out_utf8[written] = '\0';
    return written + 1;
}
```

## Optimizations

### 1. Small String Optimization (SSO)
Store strings ≤8 characters inline in descriptor (saves malloc):
```c
union {
    struct { uint32_t* data; ... } heap;  // Normal case
    struct { uint32_t inline_data[8]; uint8_t len; } sso; // ≤8 chars
}
```

### 2. Capacity Over-Allocation
When growing, allocate extra space:
```c
new_capacity = max(required, current * 1.5);
```
Amortizes cost for repeated concatenations.

### 3. String Literal Pool
Store literals in data section as UTF-32:
```qbe
data $str_0 = { w 72, w 101, w 108, w 108, w 111, w 0 } # "Hello"
```

### 4. Lazy UTF-8 Conversion
Only convert when printing/saving:
- Set `dirty` flag on modification
- Cache UTF-8 result in descriptor
- Reuse cache until dirty

### 5. Common Character Cache
Pre-allocate CHR$(0-127) at startup.

### 6. Reference Counting
Share data between strings:
```qbe
# A$ = B$
%ref =w loadw [...B$ + 24...]
%ref_new =w add %ref, 1
storew %ref_new, [...B$ + 24...]
```

## BASIC Function Mapping

| BASIC Function | Operation | Complexity |
|----------------|-----------|------------|
| `LEN(S$)` | Load length field | O(1) |
| `ASC(S$)` | Load first character | O(1) |
| `CHR$(n)` | Create 1-char string | O(1) |
| `MID$(S$,p,n)` | memcpy substring | O(n) |
| `LEFT$(S$,n)` | memcpy first n | O(n) |
| `RIGHT$(S$,n)` | memcpy last n | O(n) |
| `INSTR(H$,N$)` | memcmp search | O(h*n) |
| `A$ + B$` | Allocate + 2x memcpy | O(a+b) |
| `A$ = B$` | Increment refcount | O(1) * |
| `LCASE$(S$)` | Loop + lowercase | O(n) |
| `UCASE$(S$)` | Loop + uppercase | O(n) |
| `VAL(S$)` | Convert to UTF-8 + strtod | O(n) |
| `STR$(n)` | snprintf + UTF-8→UTF-32 | O(digits) |

\* With reference counting; O(n) with deep copy

## Performance vs Runtime Calls

| Operation | Inline Code | Runtime Call | Speedup |
|-----------|-------------|--------------|---------|
| Character access | ~5 instructions | ~50 instructions | 10x |
| MID$ | ~15 instructions | ~100 instructions | 6x |
| Concatenation | ~20 instructions | ~80 instructions | 4x |
| INSTR | ~30 instructions + memcmp | ~200 instructions | 6x |
| Comparison | ~10 instructions + memcmp | ~60 instructions | 6x |

## Memory Usage

| String | UTF-32 | UTF-8 | Ratio |
|--------|--------|-------|-------|
| Descriptor | 40 bytes | 40 bytes | 1:1 |
| "Hello" | 20 bytes | 5 bytes | 4:1 |
| "こんにちは" | 20 bytes | 15 bytes | 1.3:1 |
| Total "Hello" | 60 bytes | 45 bytes | 1.3:1 |
| Total "こんにちは" | 60 bytes | 55 bytes | 1.1:1 |

**Conclusion**: 1.5-2x total memory for typical strings (acceptable).

## Implementation Checklist

### Phase 1: Core (Week 1)
- [ ] StringDescriptor structure
- [ ] utf8_to_utf32() and utf32_to_utf8()
- [ ] string_new(), string_release()
- [ ] Memory management helpers

### Phase 2: Basics (Week 1-2)
- [ ] String literal codegen
- [ ] LEN() function
- [ ] ASC() and CHR$() functions
- [ ] String concatenation (+)
- [ ] PRINT with UTF-8 conversion

### Phase 3: Substrings (Week 2)
- [ ] MID$() function
- [ ] LEFT$() and RIGHT$()
- [ ] Bounds checking

### Phase 4: Search/Compare (Week 3)
- [ ] INSTR() function
- [ ] String comparison (=, <>, <, >)
- [ ] Case-insensitive compare

### Phase 5: Conversions (Week 3)
- [ ] VAL() function
- [ ] STR$() function
- [ ] LCASE$(), UCASE$()
- [ ] TRIM$(), LTRIM$(), RTRIM$()

### Phase 6: I/O (Week 4)
- [ ] INPUT statement
- [ ] FILE I/O integration
- [ ] PRINT #n statement

### Phase 7: Optimizations (Week 4+)
- [ ] Small String Optimization
- [ ] Capacity over-allocation
- [ ] String literal pooling
- [ ] Character cache
- [ ] Reference counting

## Test Cases

```basic
' Test 1: Basic operations
S$ = "Hello"
PRINT LEN(S$)                    ' 5
PRINT ASC(S$)                    ' 72 (H)
PRINT CHR$(65)                   ' A

' Test 2: Concatenation
A$ = "Hello"
B$ = "World"
C$ = A$ + " " + B$
PRINT C$                         ' Hello World

' Test 3: Substrings
S$ = "ABCDEFGH"
PRINT LEFT$(S$, 3)               ' ABC
PRINT RIGHT$(S$, 3)              ' FGH
PRINT MID$(S$, 3, 4)             ' CDEF

' Test 4: Search
S$ = "The quick brown fox"
pos = INSTR(S$, "quick")
PRINT pos                        ' 5

' Test 5: Comparison
IF "ABC" < "XYZ" THEN
    PRINT "Correct"
END IF

' Test 6: Case conversion
S$ = "Hello World"
PRINT LCASE$(S$)                 ' hello world
PRINT UCASE$(S$)                 ' HELLO WORLD

' Test 7: Conversion
x = VAL("123.45")
S$ = STR$(x)
PRINT S$                         ' 123.45

' Test 8: Unicode (if supported)
S$ = "こんにちは"
PRINT LEN(S$)                    ' 5 (5 code points)
PRINT MID$(S$, 2, 2)             ' んに
```

## Conclusion

UTF-32 internal representation provides:
- **Simple implementation** (treat strings like arrays)
- **Fast operations** (O(1) access, memcpy operations)
- **Safe execution** (bounds checking)
- **C compatibility** (UTF-8 at boundaries)

Trade-off: 4x memory vs ASCII (acceptable for modern systems).

**Status**: Ready for implementation. Start with Phase 1-2 for core functionality.