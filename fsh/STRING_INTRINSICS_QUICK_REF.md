# String Intrinsics Quick Reference
## FasterBASIC - Dual-Encoding String Functions

---

## Quick Start

### Supported Functions

| Function | Returns | Intrinsic? | Performance |
|----------|---------|-----------|-------------|
| `LEN(s$)` | Integer | ✅ Yes | O(1) - 2 instructions |
| `ASC(s$)` | Integer | ✅ Yes | O(1) - ~15 instructions |
| `CHR$(n)` | String | ❌ Runtime | O(1) - Fast allocation |
| `VAL(s$)` | Double | ❌ Runtime | O(n) - String scan |

---

## Usage Examples

### LEN(s$) - String Length

```basic
DIM s AS STRING
s = "Hello"
PRINT LEN(s)           ' Output: 5
PRINT LEN("")          ' Output: 0
PRINT LEN("Unicode")   ' Output: 7
```

### ASC(s$) - First Character Code

```basic
PRINT ASC("A")         ' Output: 65
PRINT ASC("Z")         ' Output: 90
PRINT ASC("Hello")     ' Output: 72 (H)
PRINT ASC("")          ' Output: 0
```

### CHR$(n) - Create String from Code

```basic
PRINT CHR$(65)         ' Output: A
PRINT CHR$(72)         ' Output: H
PRINT CHR$(32)         ' Output: (space)

' Build strings
DIM s AS STRING
s = CHR$(72) + CHR$(105)  ' "Hi"
```

### VAL(s$) - String to Number

```basic
PRINT VAL("42")        ' Output: 42
PRINT VAL("3.14")      ' Output: 3.14
PRINT VAL("-99")       ' Output: -99
PRINT VAL("12abc")     ' Output: 12
PRINT VAL("abc")       ' Output: 0
```

---

## Encoding System

### ASCII Strings (1 byte/char)
- Characters with codes 0-127
- Uses 1 byte per character
- Fast and memory-efficient

```basic
s = "Hello"            ' ASCII encoding (all chars < 128)
```

### UTF-32 Strings (4 bytes/char)
- Characters with codes >= 128
- Uses 4 bytes per character
- Full Unicode support

```basic
s = CHR$(12371)        ' UTF-32 encoding (Unicode character)
```

### Auto-Promotion
Strings automatically promote from ASCII to UTF-32 when needed:

```basic
DIM a AS STRING = "Hello"      ' ASCII
DIM b AS STRING = CHR$(12371)  ' UTF-32
DIM c AS STRING = a + b        ' Result: UTF-32 (auto-promoted)
```

---

## Performance Tips

### ✅ Fast Operations (Use Freely)
- `LEN(s$)` - Intrinsic, 2 instructions
- `ASC(s$)` - Intrinsic, ~15 instructions
- `CHR$(n)` where n < 128 - Creates ASCII string

### ⚠️ Moderate Cost
- `CHR$(n)` where n >= 128 - Creates UTF-32 string
- `VAL(s$)` - O(n) parsing

### 💡 Optimization
- Keep strings ASCII when possible (4x less memory)
- Cache `LEN(s$)` results in loops
- Avoid repeated `VAL()` calls on same string

---

## Common Patterns

### Building Strings Character by Character
```basic
DIM s AS STRING
s = ""
s = s + CHR$(72)   ' H
s = s + CHR$(101)  ' e
s = s + CHR$(108)  ' l
s = s + CHR$(108)  ' l
s = s + CHR$(111)  ' o
PRINT s            ' Output: Hello
```

### Extracting First Character
```basic
DIM s AS STRING = "BASIC"
DIM first_code AS INTEGER
first_code = ASC(s)
PRINT CHR$(first_code)  ' Output: B
```

### Checking String Length
```basic
IF LEN(s) > 0 THEN
    PRINT "String is not empty"
END IF
```

### Converting Numbers
```basic
DIM input AS STRING = "123"
DIM number AS INTEGER
number = VAL(input)
PRINT number * 2    ' Output: 246
```

---

## Implementation Details

### String Descriptor (40 bytes)
```
Offset 0:  void* data         - Character data
Offset 8:  int64_t length     - Number of characters
Offset 16: int64_t capacity   - Allocated capacity
Offset 24: int32_t refcount   - Reference count
Offset 28: uint8_t encoding   - 0=ASCII, 1=UTF-32
Offset 29: uint8_t dirty      - UTF-8 cache flag
Offset 32: char* utf8_cache   - UTF-8 cache
```

### Intrinsic Code Generation

**LEN(s$)**:
```qbe
%len_ptr =l add %str, 8
%len =l loadl %len_ptr
```

**ASC(s$)**:
```qbe
; Check empty → load encoding → branch → load char
; ~15 QBE instructions total
```

---

## Testing

### Run Tests
```bash
cd /Users/oberon/FBFAM/FBCQBE/fsh

# Quick test
./fbc_qbe test_string_minimal.bas -o test_string_minimal
./test_string_minimal

# Comprehensive test
./fbc_qbe test_string_intrinsics.bas -o test_string_intrinsics
./test_string_intrinsics
```

---

## Error Handling

### Empty Strings
```basic
DIM s AS STRING = ""
PRINT LEN(s)       ' Returns: 0
PRINT ASC(s)       ' Returns: 0 (safe)
PRINT VAL(s)       ' Returns: 0.0
```

### Invalid Characters
```basic
PRINT VAL("abc")   ' Returns: 0.0 (no digits)
PRINT VAL("12x34") ' Returns: 12.0 (stops at 'x')
```

---

## API Reference

### Runtime Functions (C)

```c
// BASIC intrinsic wrappers
int64_t basic_len(const StringDescriptor* str);
uint32_t basic_asc(const StringDescriptor* str);
StringDescriptor* basic_chr(uint32_t codepoint);
double basic_val(const StringDescriptor* str);

// Character access
uint32_t string_get_char_at(const StringDescriptor* str, int64_t index);
int string_set_char_at(StringDescriptor* str, int64_t index, uint32_t code);

// Promotion
StringDescriptor* string_promote_to_utf32(StringDescriptor* str);
```

---

## Next Steps

### Phase 2 Features (Future)
- [ ] Character access syntax: `A$(i)`
- [ ] String slicing: `A$(start:end)`
- [ ] More string functions: MID$, LEFT$, RIGHT$
- [ ] Small String Optimization (SSO)

---

**Last Updated**: 2026-01-25  
**Status**: ✅ Production Ready

