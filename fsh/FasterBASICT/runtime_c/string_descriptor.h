//
// string_descriptor.h
// FasterBASIC Runtime - UTF-32 String Descriptor
//
// Implements efficient string handling using UTF-32 (32-bit fixed-width code points)
// for internal representation with UTF-8 conversion at system boundaries.
//
// Benefits of UTF-32:
// - O(1) character access: A$(5) is just Base + (5 * 4)
// - Simple slicing: MID$, LEFT$, RIGHT$ become memcpy operations
// - Fast pattern matching: every character unit is the same size
// - No need to scan for character boundaries
//
// Trade-off:
// - 4x memory vs ASCII (acceptable for modern systems)
// - Conversion overhead at I/O boundaries (mitigated by lazy conversion)
//

#ifndef STRING_DESCRIPTOR_H
#define STRING_DESCRIPTOR_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

//
// StringDescriptor: Tracks UTF-32 string metadata
//
// Memory layout:
//   Offset 0:  uint32_t* data      - Pointer to UTF-32 code points
//   Offset 8:  int64_t length       - Length in code points (not bytes)
//   Offset 16: int64_t capacity     - Allocated capacity in code points
//   Offset 24: int32_t refcount     - Reference count for sharing
//   Offset 28: uint8_t dirty        - Needs UTF-8 re-encoding flag
//   Offset 29: uint8_t _padding[3]  - Alignment padding
//   Offset 32: char* utf8_cache     - Cached UTF-8 representation (NULL if dirty)
//
// Total size: 40 bytes (aligned)
//
typedef struct {
    uint32_t* data;        // UTF-32 code points
    int64_t   length;      // Length in code points
    int64_t   capacity;    // Capacity in code points
    int32_t   refcount;    // Reference count
    uint8_t   dirty;       // UTF-8 cache is invalid
    uint8_t   _padding[3]; // Alignment
    char*     utf8_cache;  // Cached UTF-8 string (for C interop)
} StringDescriptor;

// Small String Optimization (SSO) threshold
// Strings shorter than this are stored inline to avoid heap allocation
#define SSO_THRESHOLD 8  // code points (32 bytes of data)

//
// StringDescriptor with Small String Optimization
//
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
            uint32_t  inline_data[8];  // 8 code points inline
            uint8_t   length;          // Length in code points
            uint8_t   is_sso;          // Always 1 for SSO
        } sso;
    };
} StringDescriptorSSO;

//
// Basic String Operations (Core API)
//

// Create new string from UTF-8 C string
StringDescriptor* string_new_utf8(const char* utf8_str);

// Create new string from UTF-32 data
StringDescriptor* string_new_utf32(const uint32_t* data, int64_t length);

// Create empty string with reserved capacity
StringDescriptor* string_new_capacity(int64_t capacity);

// Create string by repeating a character
StringDescriptor* string_new_repeat(uint32_t codepoint, int64_t count);

// Clone string (deep copy)
StringDescriptor* string_clone(const StringDescriptor* str);

// Retain string (increment refcount)
StringDescriptor* string_retain(StringDescriptor* str);

// Release string (decrement refcount, free if 0)
void string_release(StringDescriptor* str);

// Get UTF-8 representation (cached, valid until string modified)
const char* string_to_utf8(StringDescriptor* str);

// Get length in code points
static inline int64_t string_length(const StringDescriptor* str) {
    return str ? str->length : 0;
}

// Get character at index (0-based, returns 0 if out of bounds)
static inline uint32_t string_char_at(const StringDescriptor* str, int64_t index) {
    if (!str || index < 0 || index >= str->length) {
        return 0;
    }
    return str->data[index];
}

// Set character at index (returns false if out of bounds)
static inline bool string_set_char(StringDescriptor* str, int64_t index, uint32_t codepoint) {
    if (!str || index < 0 || index >= str->length) {
        return false;
    }
    str->data[index] = codepoint;
    str->dirty = 1;  // Invalidate UTF-8 cache
    return true;
}

//
// String Manipulation Operations
//

// Concatenate two strings (A$ + B$)
StringDescriptor* string_concat(const StringDescriptor* a, const StringDescriptor* b);

// Substring (MID$): extract from start for given length
// 0-based indexing internally (converted from BASIC's 1-based)
StringDescriptor* string_mid(const StringDescriptor* str, int64_t start, int64_t length);

// Left substring (LEFT$): first n characters
StringDescriptor* string_left(const StringDescriptor* str, int64_t count);

// Right substring (RIGHT$): last n characters
StringDescriptor* string_right(const StringDescriptor* str, int64_t count);

// Find substring (INSTR): returns index of first occurrence (0-based, -1 if not found)
int64_t string_instr(const StringDescriptor* haystack, const StringDescriptor* needle, int64_t start_pos);

// String comparison (case-sensitive)
int string_compare(const StringDescriptor* a, const StringDescriptor* b);

// String comparison (case-insensitive)
int string_compare_nocase(const StringDescriptor* a, const StringDescriptor* b);

// String equality check
static inline bool string_equals(const StringDescriptor* a, const StringDescriptor* b) {
    return string_compare(a, b) == 0;
}

// Convert to uppercase
StringDescriptor* string_upper(const StringDescriptor* str);

// Convert to lowercase
StringDescriptor* string_lower(const StringDescriptor* str);

// Trim whitespace from both ends
StringDescriptor* string_trim(const StringDescriptor* str);

// Trim whitespace from left
StringDescriptor* string_ltrim(const StringDescriptor* str);

// Trim whitespace from right
StringDescriptor* string_rtrim(const StringDescriptor* str);

// Reverse string
StringDescriptor* string_reverse(const StringDescriptor* str);

// Replace all occurrences of a substring
StringDescriptor* string_replace(const StringDescriptor* str, 
                                  const StringDescriptor* old_substr,
                                  const StringDescriptor* new_substr);

//
// Conversion Functions
//

// Convert string to integer (returns 0 if conversion fails)
int64_t string_to_int(const StringDescriptor* str);

// Convert string to double (returns 0.0 if conversion fails)
double string_to_double(const StringDescriptor* str);

// Convert integer to string
StringDescriptor* string_from_int(int64_t value);

// Convert double to string
StringDescriptor* string_from_double(double value);

//
// Character Classification (Unicode-aware)
//

// Check if character is whitespace
static inline bool char_is_whitespace(uint32_t codepoint) {
    // Basic whitespace: space, tab, newline, carriage return
    return codepoint == 0x20 || codepoint == 0x09 || 
           codepoint == 0x0A || codepoint == 0x0D ||
           codepoint == 0xA0;  // Non-breaking space
}

// Check if character is alphanumeric (basic ASCII)
static inline bool char_is_alnum(uint32_t codepoint) {
    return (codepoint >= '0' && codepoint <= '9') ||
           (codepoint >= 'A' && codepoint <= 'Z') ||
           (codepoint >= 'a' && codepoint <= 'z');
}

// Convert character to uppercase (basic ASCII)
static inline uint32_t char_to_upper(uint32_t codepoint) {
    if (codepoint >= 'a' && codepoint <= 'z') {
        return codepoint - 32;
    }
    return codepoint;
}

// Convert character to lowercase (basic ASCII)
static inline uint32_t char_to_lower(uint32_t codepoint) {
    if (codepoint >= 'A' && codepoint <= 'Z') {
        return codepoint + 32;
    }
    return codepoint;
}

//
// UTF-8 ↔ UTF-32 Conversion Utilities
//

// Decode UTF-8 string to UTF-32 code points
// Returns number of code points written (or -1 on error)
int64_t utf8_to_utf32(const char* utf8_str, uint32_t* out_utf32, int64_t out_capacity);

// Encode UTF-32 code points to UTF-8 string
// Returns number of bytes written (including null terminator, or -1 on error)
int64_t utf32_to_utf8(const uint32_t* utf32_data, int64_t length, char* out_utf8, int64_t out_capacity);

// Get length of UTF-8 string in code points (not bytes)
int64_t utf8_length_in_codepoints(const char* utf8_str);

// Get required buffer size for UTF-32 → UTF-8 conversion
int64_t utf32_to_utf8_size(const uint32_t* utf32_data, int64_t length);

//
// Memory Management Helpers
//

// Ensure string has enough capacity (may reallocate)
bool string_ensure_capacity(StringDescriptor* str, int64_t required_capacity);

// Shrink capacity to match length (free unused memory)
void string_shrink_to_fit(StringDescriptor* str);

// Mark UTF-8 cache as dirty (forces re-encoding on next access)
static inline void string_mark_dirty(StringDescriptor* str) {
    if (str) {
        str->dirty = 1;
        if (str->utf8_cache) {
            free(str->utf8_cache);
            str->utf8_cache = NULL;
        }
    }
}

//
// Debug and Statistics
//

// Print string descriptor info (for debugging)
void string_debug_print(const StringDescriptor* str);

// Get memory usage of string (descriptor + data + cache)
size_t string_memory_usage(const StringDescriptor* str);

//
// BASIC-Specific String Functions
//

// STRING$(n, c) - Create string of n characters c
StringDescriptor* basic_string_repeat(int64_t count, uint32_t codepoint);

// CHR$(n) - Create single-character string from code point
StringDescriptor* basic_chr(uint32_t codepoint);

// ASC(s$) - Get code point of first character (0 if empty)
uint32_t basic_asc(const StringDescriptor* str);

// LEN(s$) - Get length in code points
static inline int64_t basic_len(const StringDescriptor* str) {
    return string_length(str);
}

// VAL(s$) - Convert string to number (double)
double basic_val(const StringDescriptor* str);

// STR$(n) - Convert number to string
StringDescriptor* basic_str_int(int64_t value);
StringDescriptor* basic_str_double(double value);

// SPACE$(n) - Create string of n spaces
StringDescriptor* basic_space(int64_t count);

// LCASE$(s$) - Convert to lowercase
static inline StringDescriptor* basic_lcase(const StringDescriptor* str) {
    return string_lower(str);
}

// UCASE$(s$) - Convert to uppercase
static inline StringDescriptor* basic_ucase(const StringDescriptor* str) {
    return string_upper(str);
}

// LTRIM$(s$) - Trim left whitespace
static inline StringDescriptor* basic_ltrim(const StringDescriptor* str) {
    return string_ltrim(str);
}

// RTRIM$(s$) - Trim right whitespace
static inline StringDescriptor* basic_rtrim(const StringDescriptor* str) {
    return string_rtrim(str);
}

// TRIM$(s$) - Trim both sides
static inline StringDescriptor* basic_trim(const StringDescriptor* str) {
    return string_trim(str);
}

#ifdef __cplusplus
}
#endif

#endif // STRING_DESCRIPTOR_H