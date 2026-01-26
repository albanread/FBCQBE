# String System Quick-Start Guide

## TL;DR - What You Need to Know

The FasterBASIC string system uses:
- **UTF-32** internally (4 bytes per character)
- **Descriptor pool** for fast allocation (20x faster than malloc)
- **Reference counting** for efficient assignment
- **Inline QBE code** for operations (5-10x faster)
- **Lazy UTF-8 conversion** only at I/O boundaries

## 30-Second Overview

```
BASIC String → Descriptor (40 bytes) → UTF-32 Data (4n bytes)
                    ↓                         ↓
              From Pool (O(1))         Fixed-width array
                    ↓                         ↓
              Refcounted            O(1) character access
```

## Implementation Order

### Phase 1: Pool Setup (Day 1)
```c
// In basic_runtime_init():
string_pool_init(&g_string_pool);

// In basic_runtime_cleanup():
string_pool_cleanup(&g_string_pool);
```

### Phase 2: UTF Conversion (Day 1-2)
```c
// Implement these in string_pool.c:
int64_t utf8_to_utf32(const char* utf8, uint32_t* out, int64_t cap);
int64_t utf32_to_utf8(const uint32_t* utf32, int64_t len, char* out, int64_t cap);
```

### Phase 3: Basic Codegen (Day 2-3)
```qbe
# String literal: S$ = "Hello"
%desc =l call $string_desc_alloc()
%data =l call $malloc(l 20)
call $memcpy(l %data, l $str_lit_0, l 20)
storel %data, %desc
[... init length, capacity, refcount ...]
```

### Phase 4: Operations (Day 3-5)
```qbe
# Character access: c = ASC(S$)
%data =l loadl %desc
%char =w loadw %data

# Concatenation: C$ = A$ + B$
[... allocate result, memcpy A$ + B$ ...]

# Print: PRINT S$
call $string_to_utf8(l %desc)
%utf8 =l [... get cache ...]
call $basic_print_cstr(l %utf8)
```

## Key QBE Patterns

### Allocate String
```qbe
%desc =l call $string_desc_alloc()    # From pool
```

### Free String
```qbe
call $string_desc_release(l %desc)    # Decrements refcount, returns to pool
```

### Access Character
```qbe
%data =l loadl %desc                  # offset 0
%offset =l mul %index, 4              # index * 4
%char_addr =l add %data, %offset
%char =w loadw %char_addr
```

### Get Length
```qbe
%len_addr =l add %desc, 8             # offset 8
%len =l loadl %len_addr
```

### Concatenate
```qbe
%c_desc =l call $string_desc_alloc()
[... calculate total_len = a_len + b_len ...]
%c_data =l call $malloc(l total_bytes)
call $memcpy(l %c_data, l %a_data, l %a_bytes)
call $memcpy(l %c_dest, l %b_data, l %b_bytes)
[... init c_desc ...]
```

## Descriptor Layout (40 bytes)

```
Offset  Field         Type      Size
------  ------------  --------  ----
0       data          uint32_t* 8
8       length        int64_t   8
16      capacity      int64_t   8
24      refcount      int32_t   4
28      dirty         uint8_t   1
29      _padding[3]   uint8_t   3
32      utf8_cache    char*     8
```

## Essential Functions

### Pool Management
```c
string_pool_init(&g_string_pool);              // Startup
string_pool_cleanup(&g_string_pool);           // Shutdown
StringDescriptor* string_desc_alloc(void);     // Get descriptor
void string_desc_release(StringDescriptor* d); // Return descriptor
```

### UTF Conversion
```c
utf8_to_utf32(utf8, out_utf32, capacity);      // Input
utf32_to_utf8(utf32, length, out_utf8, cap);   // Output
```

### String Operations (to implement)
```c
string_concat(a, b);        // A$ + B$
string_mid(s, start, len);  // MID$(S$, start, len)
string_left(s, n);          // LEFT$(S$, n)
string_right(s, n);         // RIGHT$(S$, n)
string_instr(hay, needle);  // INSTR(hay$, needle$)
string_compare(a, b);       // A$ = B$, A$ < B$
```

## BASIC Function Mapping

| BASIC | Operation | QBE Pattern |
|-------|-----------|-------------|
| `LEN(S$)` | Load length field | `loadl [desc+8]` |
| `ASC(S$)` | Load first char | `loadw [data]` |
| `CHR$(n)` | Create 1-char string | `alloc, store char` |
| `MID$(S$,p,n)` | Substring | `memcpy` |
| `A$ + B$` | Concatenate | `2x memcpy` |
| `A$ = B$` | Assignment | `increment refcount` |
| `PRINT S$` | Output | `utf32_to_utf8, print` |
| `INPUT S$` | Input | `read, utf8_to_utf32` |

## Example: Complete String Literal

### BASIC Code
```basic
S$ = "Hello"
```

### Generated QBE IL
```qbe
# Allocate descriptor from pool
%desc =l call $string_desc_alloc()

# Allocate UTF-32 data (5 chars * 4 = 20 bytes)
%data =l call $malloc(l 20)

# Copy literal (converted to UTF-32 at compile time)
data $str_0 = { w 72, w 101, w 108, w 108, w 111 }
call $memcpy(l %data, l $str_0, l 20)

# Initialize descriptor
storel %data, %desc                 # data pointer
%len_addr =l add %desc, 8
storel 5, %len_addr                 # length = 5
%cap_addr =l add %desc, 16
storel 5, %cap_addr                 # capacity = 5
%ref_addr =l add %desc, 24
storew 1, %ref_addr                 # refcount = 1
%dirty_addr =l add %desc, 28
storeb 1, %dirty_addr               # dirty = 1
%cache_addr =l add %desc, 32
storel 0, %cache_addr               # utf8_cache = NULL

# S$ now points to descriptor
%str_S =l copy %desc
```

## Example: String Concatenation

### BASIC Code
```basic
C$ = A$ + B$
```

### Generated QBE IL
```qbe
# Allocate result descriptor
%c_desc =l call $string_desc_alloc()

# Get A$ length
%a_len_addr =l add %str_A, 8
%a_len =l loadl %a_len_addr

# Get B$ length
%b_len_addr =l add %str_B, 8
%b_len =l loadl %b_len_addr

# Calculate total length
%total_len =l add %a_len, %b_len

# Allocate result data
%total_bytes =l mul %total_len, 4
%c_data =l call $malloc(l %total_bytes)

# Copy A$'s data
%a_data =l loadl %str_A
%a_bytes =l mul %a_len, 4
call $memcpy(l %c_data, l %a_data, l %a_bytes)

# Copy B$'s data after A$
%b_data =l loadl %str_B
%b_bytes =l mul %b_len, 4
%b_dest =l add %c_data, %a_bytes
call $memcpy(l %b_dest, l %b_data, l %b_bytes)

# Initialize C$ descriptor
storel %c_data, %c_desc
%len_addr =l add %c_desc, 8
storel %total_len, %len_addr
%cap_addr =l add %c_desc, 16
storel %total_len, %cap_addr
%ref_addr =l add %c_desc, 24
storew 1, %ref_addr
%dirty_addr =l add %c_desc, 28
storeb 1, %dirty_addr
%cache_addr =l add %c_desc, 32
storel 0, %cache_addr

# C$ now points to result
%str_C =l copy %c_desc
```

## Example: Print with UTF-8 Conversion

### BASIC Code
```basic
PRINT S$
```

### Generated QBE IL
```qbe
# Check if UTF-8 cache is valid
%dirty_addr =l add %str_S, 28
%dirty =w loadb %dirty_addr
jnz %dirty, @convert, @use_cache

@use_cache
    # Use cached UTF-8
    %cache_addr =l add %str_S, 32
    %utf8 =l loadl %cache_addr
    call $basic_print_cstr(l %utf8)
    jmp @done

@convert
    # Convert UTF-32 → UTF-8
    %len_addr =l add %str_S, 8
    %len =l loadl %len_addr
    %data =l loadl %str_S
    
    # Allocate UTF-8 buffer (max 4 bytes per code point + null)
    %max_size =l mul %len, 4
    %max_size_1 =l add %max_size, 1
    %utf8_buf =l call $malloc(l %max_size_1)
    
    # Convert
    call $utf32_to_utf8(l %data, l %len, l %utf8_buf, l %max_size_1)
    
    # Cache result
    %cache_addr =l add %str_S, 32
    storel %utf8_buf, %cache_addr
    
    # Clear dirty flag
    storeb 0, %dirty_addr
    
    # Print
    call $basic_print_cstr(l %utf8_buf)

@done
```

## Common Pitfalls

### ❌ Don't malloc descriptors directly
```c
StringDescriptor* d = malloc(sizeof(StringDescriptor)); // NO!
```

### ✅ Use the pool
```c
StringDescriptor* d = string_desc_alloc(); // YES!
```

### ❌ Don't forget refcounting
```c
A = B;  // NO! B becomes dangling when A is freed
```

### ✅ Increment refcount
```c
string_desc_retain(B);
A = B;  // YES! Both reference same descriptor
```

### ❌ Don't forget to mark dirty
```c
desc->data[5] = 'X';  // NO! UTF-8 cache now incorrect
```

### ✅ Mark dirty after modification
```c
desc->data[5] = 'X';
desc->dirty = 1;        // YES! Cache will be regenerated
```

## Performance Tips

1. **Pre-allocate capacity** for strings that grow:
   ```c
   capacity = length * 1.5;  // 50% extra
   ```

2. **Cache common strings** (empty string, single chars):
   ```c
   static StringDescriptor* g_empty_string;
   static StringDescriptor* g_chr_cache[128];
   ```

3. **Batch UTF-8 conversions** if printing multiple strings:
   ```c
   // Convert all, then print all
   ```

4. **Use memcmp for comparison**:
   ```c
   result = memcmp(a->data, b->data, len * 4);
   ```

5. **Over-allocate on growth** to amortize reallocs:
   ```c
   new_cap = max(required, current * 1.5);
   ```

## Testing Checklist

- [ ] Empty string creation
- [ ] Single character string
- [ ] String concatenation
- [ ] Character access (ASC, CHR$)
- [ ] Substring (MID$, LEFT$, RIGHT$)
- [ ] String search (INSTR)
- [ ] String comparison (=, <>, <, >)
- [ ] UTF-8 round-trip conversion
- [ ] Unicode characters (beyond ASCII)
- [ ] Reference counting (assignment)
- [ ] Pool statistics (no leaks)
- [ ] Print output (UTF-8 conversion)
- [ ] Input reading (UTF-8 conversion)

## Debug Commands

```c
// Print pool stats
string_pool_print_stats(&g_string_pool);

// Check for leaks
string_pool_check_leaks(&g_string_pool);

// Validate pool integrity
string_pool_validate(&g_string_pool);

// Print descriptor info
string_debug_print(desc);
```

## Next Steps

1. ✅ Implement pool (string_pool.c)
2. ⏳ Implement UTF conversion (utf8_utf32.c)
3. ⏳ Add codegen for string literals
4. ⏳ Add codegen for basic operations
5. ⏳ Test with simple BASIC programs
6. ⏳ Optimize hot paths
7. ⏳ Add advanced features (SSO, etc.)

## Resources

- `string_pool.h` - Pool interface
- `string_pool.c` - Pool implementation
- `string_descriptor.h` - Descriptor structure and operations
- `STRING_POOL_DESIGN.md` - Detailed pool design
- `UTF32_STRING_IMPLEMENTATION_PLAN.md` - Complete implementation plan
- `STRING_SYSTEM_SUMMARY.md` - System architecture overview

## Questions?

**Q: Why UTF-32 instead of UTF-8?**  
A: O(1) character access vs O(n). Worth 4x memory for speed.

**Q: Why pool descriptors?**  
A: 20x faster than malloc/free. Essential for performance.

**Q: Why reference counting?**  
A: Efficient string assignment without copying. Simple to implement.

**Q: What about memory overhead?**  
A: 1.5-4x vs UTF-8, acceptable for 2-4x program speedup.

**Q: Thread-safe?**  
A: No by default (simpler, faster). Can add locking or thread-local pools.

**Q: How to debug leaks?**  
A: `string_pool_check_leaks()` finds descriptors not returned to pool.

---

**Ready to implement!** Start with Phase 1 (pool setup) and Phase 2 (UTF conversion), then move to Phase 3 (codegen). Each phase builds on the previous one.