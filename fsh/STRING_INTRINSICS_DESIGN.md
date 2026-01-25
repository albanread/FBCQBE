# String Intrinsics Design Document
## FasterBASIC QBE Backend - ASCII/UTF-32 Dual-Encoding String System

**Date**: 2026-01-25  
**Status**: Implementation Phase  
**Author**: FasterBASIC Compiler Team

---

## Overview

This document describes the implementation of intrinsic string functions for FasterBASIC's dual-encoding string system. We support both **ASCII** (1 byte/char) and **UTF-32** (4 bytes/char) encodings with automatic promotion when Unicode characters are detected.

### Key Principles

1. **Intrinsic Operations**: Small, performance-critical string operations (LEN, ASC, CHR$, VAL, character access) are emitted inline as QBE IL rather than function calls
2. **Dual Encoding**: Strings start as ASCII (compact) and auto-promote to UTF-32 only when Unicode characters appear
3. **Zero-Cost ASCII**: Pure ASCII strings use 1 byte per character (no UTF-32 overhead)
4. **Transparent Promotion**: Combining ASCII and UTF-32 strings auto-promotes to UTF-32
5. **Type Safety**: Compiler tracks encoding type through expressions

---

## String Descriptor Structure

```c
typedef struct {
    void*     data;        // Character data (uint8_t* or uint32_t*)
    int64_t   length;      // Length in characters (offset 8)
    int64_t   capacity;    // Capacity in characters (offset 16)
    int32_t   refcount;    // Reference count (offset 24)
    uint8_t   encoding;    // STRING_ENCODING_ASCII (0) or UTF-32 (1) - offset 28
    uint8_t   dirty;       // UTF-8 cache dirty flag (offset 29)
    uint8_t   _padding[2]; // Alignment (offset 30)
    char*     utf8_cache;  // UTF-8 cache for I/O (offset 32)
} StringDescriptor;  // Total: 40 bytes
```

### Encoding Values
- `STRING_ENCODING_ASCII = 0` - 7-bit ASCII, 1 byte per character
- `STRING_ENCODING_UTF32 = 1` - UTF-32, 4 bytes per character

---

## Intrinsic Functions

### 1. LEN(s$) - String Length

**Returns**: `int64_t` (number of characters, not bytes)

**Implementation**: Direct load from descriptor offset 8

```qbe
; LEN(s$) - Already intrinsic!
%str =l ...                    ; String descriptor pointer
%len_ptr =l add %str, 8        ; Offset to length field
%len =l loadl %len_ptr         ; Load length (int64_t)
```

**Status**: ✅ Already implemented in codegen as intrinsic

---

### 2. ASC(s$) - Get First Character Code

**Returns**: `uint32_t` (code point, 0 if empty)

**Implementation**: Check encoding, load first character

```qbe
; ASC(s$) - Encoding-aware intrinsic
%str =l ...                    ; String descriptor pointer

; 1. Check if empty (length == 0)
%len_ptr =l add %str, 8
%len =l loadl %len_ptr
%has_chars =w csgtl %len, 0    ; len > 0?
jnz %has_chars, @asc_valid, @asc_empty

@asc_valid
; 2. Load encoding byte (offset 28)
%enc_ptr =l add %str, 28
%enc =w loadub %enc_ptr        ; Load encoding (0=ASCII, 1=UTF-32)

; 3. Load data pointer (offset 0)
%data_ptr =l loadl %str

; 4. Check encoding type
%is_ascii =w ceqw %enc, 0
jnz %is_ascii, @asc_ascii, @asc_utf32

@asc_ascii
; Load 1 byte (unsigned)
%ch_ascii =w loadub %data_ptr
%result =w copy %ch_ascii
jmp @asc_end

@asc_utf32
; Load 4 bytes (unsigned word)
%ch_utf32 =w loaduw %data_ptr
%result =w copy %ch_utf32
jmp @asc_end

@asc_empty
%result =w copy 0

@asc_end
; %result contains the code point
```

**Status**: ✅ Partially implemented (needs refinement)

---

### 3. CHR$(n) - Create Single-Character String

**Returns**: `StringDescriptor*`

**Implementation**: Inline creation for ASCII (<128), call runtime for Unicode

```qbe
; CHR$(n) - Encoding-aware creation
%code =w ...                   ; Code point

; 1. Check if ASCII (< 128)
%is_ascii =w csltw %code, 128
jnz %is_ascii, @chr_ascii, @chr_unicode

@chr_ascii
; Inline ASCII string creation (fast path)
%desc =l alloc8 40             ; Allocate descriptor on stack/heap
storel 0, %desc                ; data = NULL (will allocate)

; Allocate 1-byte buffer
%data =l call $malloc(l 1)
storel %data, %desc            ; Store data pointer

; Store character
%code_byte =w copy %code
storeb %code_byte, %data       ; data[0] = code

; Set length = 1
%len_ptr =l add %desc, 8
storel 1, %len_ptr

; Set capacity = 1
%cap_ptr =l add %desc, 16
storel 1, %cap_ptr

; Set refcount = 1
%ref_ptr =l add %desc, 24
storew 1, %ref_ptr

; Set encoding = 0 (ASCII)
%enc_ptr =l add %desc, 28
storeb 0, %enc_ptr

; Set dirty = 1
%dirty_ptr =l add %desc, 29
storeb 1, %dirty_ptr

%result =l copy %desc
jmp @chr_end

@chr_unicode
; Call runtime for UTF-32 (handles allocation properly)
%result =l call $basic_chr(w %code)

@chr_end
; %result contains StringDescriptor*
```

**Alternative**: Always call runtime (simpler, still fast)

```qbe
; CHR$(n) - Simple version (always call runtime)
%code =w ...
%result =l call $basic_chr(w %code)
```

**Status**: ⚠️ Needs implementation (currently calls runtime always)

---

### 4. VAL(s$) - Convert String to Number

**Returns**: `double`

**Implementation**: Call runtime (complex parsing logic)

```qbe
; VAL(s$) - Call runtime (not intrinsic due to complexity)
%str =l ...
%result =d call $basic_val(l %str)
```

**Status**: ✅ Already calls runtime correctly  
**Note**: Too complex to inline (handles scientific notation, signs, etc.)

---

### 5. String Character Access - `A$(i)` (Get)

**Returns**: `uint32_t` (code point)

**Implementation**: Bounds-checked character load with encoding awareness

```qbe
; A$(i) - Character extraction (intrinsic)
%str =l ...                    ; String descriptor
%index =w ...                  ; Character index (0-based internally)

; 1. Load length and check bounds
%len_ptr =l add %str, 8
%len =l loadl %len_ptr
%idx_long =l extsw %index      ; Convert index to int64_t

; Check: 0 <= index < length
%valid_low =w csgel %idx_long, 0
%valid_high =w csltl %idx_long, %len
%valid =w and %valid_low, %valid_high

jnz %valid, @char_valid, @char_error

@char_error
; Return 0 for out-of-bounds
%result =w copy 0
jmp @char_end

@char_valid
; 2. Load encoding (offset 28)
%enc_ptr =l add %str, 28
%enc =w loadub %enc_ptr

; 3. Load data pointer
%data_ptr =l loadl %str

; 4. Check encoding
%is_ascii =w ceqw %enc, 0
jnz %is_ascii, @char_ascii, @char_utf32

@char_ascii
; data[index] - 1 byte
%char_addr =l add %data_ptr, %idx_long
%ch =w loadub %char_addr
%result =w copy %ch
jmp @char_end

@char_utf32
; data[index * 4] - 4 bytes
%offset =l mul %idx_long, 4
%char_addr =l add %data_ptr, %offset
%ch =w loaduw %char_addr
%result =w copy %ch

@char_end
; %result contains code point
```

**Status**: ⏳ To be implemented

---

### 6. String Character Assignment - `A$(i) = code` (Set)

**Implementation**: Bounds-checked character store with promotion check

```qbe
; A$(i) = code - Character assignment (intrinsic)
%str =l ...                    ; String descriptor
%index =w ...                  ; Character index
%code =w ...                   ; Code point to store

; 1. Bounds check (same as get)
%len_ptr =l add %str, 8
%len =l loadl %len_ptr
%idx_long =l extsw %index

%valid_low =w csgel %idx_long, 0
%valid_high =w csltl %idx_long, %len
%valid =w and %valid_low, %valid_high

jnz %valid, @set_valid, @set_error

@set_error
; Ignore out-of-bounds writes (or call error handler)
jmp @set_end

@set_valid
; 2. Check if code requires promotion (code >= 128)
%enc_ptr =l add %str, 28
%enc =w loadub %enc_ptr
%is_ascii_enc =w ceqw %enc, 0
%needs_unicode =w csgew %code, 128

%need_promote =w and %is_ascii_enc, %needs_unicode
jnz %need_promote, @set_promote, @set_direct

@set_promote
; Promote ASCII → UTF-32 before storing
%new_str =l call $string_promote_to_utf32(l %str)
; Continue with UTF-32 storage using %new_str
; (Simplified: assume in-place promotion for now)

@set_direct
; 3. Load data pointer
%data_ptr =l loadl %str

; 4. Store based on encoding
%is_ascii =w ceqw %enc, 0
jnz %is_ascii, @set_ascii, @set_utf32

@set_ascii
; data[index] = (uint8_t)code
%char_addr =l add %data_ptr, %idx_long
storeb %code, %char_addr
jmp @set_mark_dirty

@set_utf32
; data[index * 4] = (uint32_t)code
%offset =l mul %idx_long, 4
%char_addr =l add %data_ptr, %offset
storew %code, %char_addr

@set_mark_dirty
; Mark UTF-8 cache as dirty (offset 29)
%dirty_ptr =l add %str, 29
storeb 1, %dirty_ptr

@set_end
```

**Status**: ⏳ To be implemented

---

## Auto-Promotion Rules

### When to Promote ASCII → UTF-32

1. **Character Assignment**: `A$(i) = code` where `code >= 128`
2. **String Concatenation**: `ASCII$ + UTF32$` → result is UTF-32
3. **String Functions**: Functions receiving mixed encoding operands

### Promotion Function (Runtime)

```c
StringDescriptor* string_promote_to_utf32(StringDescriptor* str) {
    if (!str || str->encoding == STRING_ENCODING_UTF32) {
        return str;  // Already UTF-32 or NULL
    }
    
    // Convert ASCII → UTF-32 in-place (may reallocate)
    uint8_t* ascii_data = (uint8_t*)str->data;
    int64_t len = str->length;
    
    uint32_t* utf32_data = (uint32_t*)malloc(len * sizeof(uint32_t));
    if (!utf32_data) return str;  // Allocation failed, keep ASCII
    
    // Copy ASCII bytes to UTF-32 code points
    for (int64_t i = 0; i < len; i++) {
        utf32_data[i] = (uint32_t)ascii_data[i];
    }
    
    free(ascii_data);
    str->data = utf32_data;
    str->capacity = len;
    str->encoding = STRING_ENCODING_UTF32;
    str->dirty = 1;  // Invalidate UTF-8 cache
    
    return str;
}
```

**Status**: ⏳ To be implemented in runtime

---

## Compiler Tracking

### Expression Type Inference

The compiler tracks string encoding through expressions:

```cpp
enum class StringEncodingHint {
    UNKNOWN,   // Not yet determined
    ASCII,     // Pure ASCII (all chars < 128)
    UTF32      // Contains Unicode (chars >= 128)
};

struct StringExpressionInfo {
    VariableType baseType;           // VariableType::STRING
    StringEncodingHint encodingHint; // Encoding hint
};
```

### Tracking Rules

1. **String Literals**: Scan at compile time → set ASCII or UTF32 hint
2. **Variables**: Track last known encoding (may change at runtime)
3. **Operations**: Propagate highest encoding (ASCII + UTF32 = UTF32)

---

## Performance Characteristics

| Operation | ASCII | UTF-32 | Notes |
|-----------|-------|--------|-------|
| LEN(s$) | O(1) | O(1) | Direct field load |
| ASC(s$) | O(1) | O(1) | Load + encoding check (6 QBE instructions) |
| CHR$(n) | O(1) | O(1) | Inline for ASCII, runtime for Unicode |
| VAL(s$) | O(n) | O(n) | Runtime call (complex parsing) |
| s$(i) get | O(1) | O(1) | 10-15 QBE instructions (bounds + encoding) |
| s$(i) set | O(1) | O(1) | 12-20 QBE instructions (may promote) |
| Concatenation | O(n) | O(n) | Runtime call |

---

## Implementation Plan

### Phase 1: Core Intrinsics (Current)
- ✅ LEN(s$) - already intrinsic
- ⚠️ ASC(s$) - refine existing implementation
- ⏳ CHR$(n) - decide: inline or runtime
- ✅ VAL(s$) - keep as runtime call

### Phase 2: Character Access
- ⏳ s$(i) - character extraction
- ⏳ s$(i) = code - character assignment
- ⏳ Bounds checking
- ⏳ Promotion logic

### Phase 3: Auto-Promotion
- ⏳ Runtime promotion function
- ⏳ Codegen promotion detection
- ⏳ Concatenation promotion

### Phase 4: Testing
- ⏳ Comprehensive test suite
- ⏳ Performance benchmarks
- ⏳ Edge cases (empty strings, boundary values)

---

## Test Cases

```basic
' Test 1: LEN() intrinsic
DIM s AS STRING
s = "Hello"
PRINT LEN(s)           ' 5

' Test 2: ASC() intrinsic
PRINT ASC("A")         ' 65
PRINT ASC("")          ' 0
PRINT ASC("こんにちは") ' Unicode code point

' Test 3: CHR$() intrinsic
PRINT CHR$(65)         ' "A" (ASCII)
PRINT CHR$(12371)      ' "こ" (UTF-32 auto-promotion)

' Test 4: Character access
s = "BASIC"
PRINT ASC(MID$(s, 1, 1))  ' Emulate s$(0) - need syntax support

' Test 5: Auto-promotion
DIM a AS STRING = "Hello"    ' ASCII
DIM b AS STRING = "世界"      ' UTF-32
DIM c AS STRING = a + b       ' Result: UTF-32
```

---

## Open Questions

1. **Syntax for Character Access**: Does FasterBASIC support `A$(i)` syntax?
   - If no: Need to add to parser
   - If yes: Implement in codegen

2. **CHR$() Implementation**: Inline or runtime?
   - **Inline**: Faster for ASCII (common case)
   - **Runtime**: Simpler, consistent

3. **Promotion Strategy**: In-place or copy-on-write?
   - **In-place**: Faster, modifies original
   - **Copy-on-write**: Safer, refcount-aware

4. **Error Handling**: How to handle invalid operations?
   - Out-of-bounds access: Return 0 or error?
   - Promotion failure: Fallback to ASCII?

---

## Recommendations

1. **Start with ASC(), LEN() refinement** - Already partially done
2. **Keep CHR$() as runtime call** - Simpler, still fast (1 allocation)
3. **Implement character access next** - Enables powerful string manipulation
4. **Add promotion as final step** - Requires runtime support

---

## Status Summary

| Function | Implementation | Status |
|----------|----------------|--------|
| LEN(s$) | Intrinsic | ✅ Done |
| ASC(s$) | Intrinsic | ⚠️ Needs refinement |
| CHR$(n) | Runtime call | ✅ Working |
| VAL(s$) | Runtime call | ✅ Working |
| s$(i) get | Not implemented | ⏳ TODO |
| s$(i) set | Not implemented | ⏳ TODO |
| Promotion | Not implemented | ⏳ TODO |

