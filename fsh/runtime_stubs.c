// runtime_stubs.c
// Minimal runtime stubs for testing FasterBASIC QBE backend

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// ============================================================================
// UTF-32 String Implementation
// ============================================================================

// String Descriptor Structure (40 bytes)
typedef struct {
    uint32_t* data;        // Offset 0:  Pointer to UTF-32 code points (8 bytes)
    int64_t   length;      // Offset 8:  Length in code points (8 bytes)
    int64_t   capacity;    // Offset 16: Capacity in code points (8 bytes)
    int32_t   refcount;    // Offset 24: Reference count for sharing (4 bytes)
    uint8_t   dirty;       // Offset 28: UTF-8 cache needs update (1 byte)
    uint8_t   _padding[3]; // Offset 29: Alignment (3 bytes)
    char*     utf8_cache;  // Offset 32: Cached UTF-8 for C interop (8 bytes)
} StringDescriptor;

// ============================================================================
// UTF-8 to UTF-32 Conversion
// ============================================================================

// Get the length of a UTF-8 string in code points
int64_t utf8_length_in_codepoints(const char* utf8_str) {
    if (!utf8_str) return 0;
    
    int64_t count = 0;
    const uint8_t* p = (const uint8_t*)utf8_str;
    
    while (*p) {
        if ((*p & 0x80) == 0) {
            // 1-byte character
            p += 1;
        } else if ((*p & 0xE0) == 0xC0) {
            // 2-byte character
            p += 2;
        } else if ((*p & 0xF0) == 0xE0) {
            // 3-byte character
            p += 3;
        } else if ((*p & 0xF8) == 0xF0) {
            // 4-byte character
            p += 4;
        } else {
            // Invalid UTF-8, skip byte
            p += 1;
            continue;
        }
        count++;
    }
    
    return count;
}

// Convert UTF-8 to UTF-32
// Returns the number of code points converted
int64_t utf8_to_utf32(const char* utf8_str, uint32_t* out_utf32, int64_t out_capacity) {
    if (!utf8_str || !out_utf32) return 0;
    
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

// ============================================================================
// UTF-32 to UTF-8 Conversion
// ============================================================================

// Convert UTF-32 to UTF-8
// Returns the number of bytes written (including null terminator)
int64_t utf32_to_utf8(const uint32_t* utf32_data, int64_t length, 
                      char* out_utf8, int64_t out_capacity) {
    if (!utf32_data || !out_utf8 || out_capacity == 0) return 0;
    
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

// ============================================================================
// String Memory Management
// ============================================================================

// Create a new empty string descriptor
StringDescriptor* string_new_empty(void) {
    StringDescriptor* desc = (StringDescriptor*)calloc(1, sizeof(StringDescriptor));
    if (desc) {
        desc->data = NULL;
        desc->length = 0;
        desc->capacity = 0;
        desc->refcount = 1;
        desc->dirty = 1;
        desc->utf8_cache = NULL;
    }
    return desc;
}

// Create empty string with reserved capacity
StringDescriptor* string_new_capacity(int64_t capacity) {
    if (capacity <= 0) return string_new_empty();
    
    StringDescriptor* desc = (StringDescriptor*)calloc(1, sizeof(StringDescriptor));
    if (!desc) return NULL;
    
    desc->data = (uint32_t*)malloc(capacity * sizeof(uint32_t));
    if (!desc->data) {
        free(desc);
        return NULL;
    }
    
    desc->length = 0;
    desc->capacity = capacity;
    desc->refcount = 1;
    desc->dirty = 1;
    desc->utf8_cache = NULL;
    
    return desc;
}

// Create a string from UTF-8 literal
StringDescriptor* string_new_utf8(const char* utf8_str) {
    if (!utf8_str) return string_new_empty();
    
    StringDescriptor* desc = (StringDescriptor*)calloc(1, sizeof(StringDescriptor));
    if (!desc) return NULL;
    
    // Get length in code points
    int64_t cp_len = utf8_length_in_codepoints(utf8_str);
    
    if (cp_len == 0) {
        desc->data = NULL;
        desc->length = 0;
        desc->capacity = 0;
    } else {
        // Allocate UTF-32 buffer
        desc->data = (uint32_t*)malloc(cp_len * sizeof(uint32_t));
        if (!desc->data) {
            free(desc);
            return NULL;
        }
        
        // Convert UTF-8 to UTF-32
        utf8_to_utf32(utf8_str, desc->data, cp_len);
        desc->length = cp_len;
        desc->capacity = cp_len;
    }
    
    desc->refcount = 1;
    desc->dirty = 1;
    desc->utf8_cache = NULL;
    
    return desc;
}

// Create a string from UTF-32 data
StringDescriptor* string_new_from_utf32(const uint32_t* data, int64_t length) {
    if (!data || length == 0) return string_new_empty();
    
    StringDescriptor* desc = (StringDescriptor*)calloc(1, sizeof(StringDescriptor));
    if (!desc) return NULL;
    
    desc->data = (uint32_t*)malloc(length * sizeof(uint32_t));
    if (!desc->data) {
        free(desc);
        return NULL;
    }
    
    memcpy(desc->data, data, length * sizeof(uint32_t));
    desc->length = length;
    desc->capacity = length;
    desc->refcount = 1;
    desc->dirty = 1;
    desc->utf8_cache = NULL;
    
    return desc;
}

// Release a string descriptor
void string_release(StringDescriptor* desc) {
    if (!desc) return;
    
    desc->refcount--;
    
    if (desc->refcount <= 0) {
        if (desc->data) {
            free(desc->data);
        }
        if (desc->utf8_cache) {
            free(desc->utf8_cache);
        }
        free(desc);
    }
}

// Get or create UTF-8 cache
const char* string_get_utf8(StringDescriptor* desc) {
    if (!desc) return "";
    
    if (desc->length == 0) return "";
    
    // If cache is valid, use it
    if (!desc->dirty && desc->utf8_cache) {
        return desc->utf8_cache;
    }
    
    // Free old cache if exists
    if (desc->utf8_cache) {
        free(desc->utf8_cache);
    }
    
    // Allocate UTF-8 buffer (worst case: 4 bytes per code point + null)
    int64_t max_utf8 = desc->length * 4 + 1;
    desc->utf8_cache = (char*)malloc(max_utf8);
    if (!desc->utf8_cache) return "";
    
    // Convert UTF-32 to UTF-8
    utf32_to_utf8(desc->data, desc->length, desc->utf8_cache, max_utf8);
    desc->dirty = 0;
    
    return desc->utf8_cache;
}

// ============================================================================
// String Operations
// ============================================================================

// Concatenate two strings
StringDescriptor* string_concat(StringDescriptor* a, StringDescriptor* b) {
    if (!a || !b) return string_new_empty();
    
    int64_t total_len = a->length + b->length;
    
    if (total_len == 0) return string_new_empty();
    
    StringDescriptor* result = (StringDescriptor*)calloc(1, sizeof(StringDescriptor));
    if (!result) return NULL;
    
    result->data = (uint32_t*)malloc(total_len * sizeof(uint32_t));
    if (!result->data) {
        free(result);
        return NULL;
    }
    
    // Copy both strings
    if (a->length > 0) {
        memcpy(result->data, a->data, a->length * sizeof(uint32_t));
    }
    if (b->length > 0) {
        memcpy(result->data + a->length, b->data, b->length * sizeof(uint32_t));
    }
    
    result->length = total_len;
    result->capacity = total_len;
    result->refcount = 1;
    result->dirty = 1;
    result->utf8_cache = NULL;
    
    return result;
}

// Get substring (MID$ function)
StringDescriptor* string_mid(StringDescriptor* str, int64_t start, int64_t length) {
    if (!str || start < 0 || start >= str->length || length <= 0) {
        return string_new_empty();
    }
    
    // Clamp length to available characters
    if (start + length > str->length) {
        length = str->length - start;
    }
    
    return string_new_from_utf32(str->data + start, length);
}

// Get left substring
StringDescriptor* string_left(StringDescriptor* str, int64_t length) {
    if (!str || length <= 0) return string_new_empty();
    if (length > str->length) length = str->length;
    return string_new_from_utf32(str->data, length);
}

// Get right substring
StringDescriptor* string_right(StringDescriptor* str, int64_t length) {
    if (!str || length <= 0) return string_new_empty();
    if (length > str->length) length = str->length;
    return string_new_from_utf32(str->data + (str->length - length), length);
}

// Compare two strings
int32_t string_compare(StringDescriptor* a, StringDescriptor* b) {
    if (!a || !b) return 0;
    
    int64_t min_len = (a->length < b->length) ? a->length : b->length;
    
    for (int64_t i = 0; i < min_len; i++) {
        if (a->data[i] < b->data[i]) return -1;
        if (a->data[i] > b->data[i]) return 1;
    }
    
    if (a->length < b->length) return -1;
    if (a->length > b->length) return 1;
    return 0;
}

// Find substring (INSTR function) - returns 1-based index, 0 if not found
int64_t string_instr(StringDescriptor* haystack, StringDescriptor* needle) {
    if (!haystack || !needle || needle->length == 0 || needle->length > haystack->length) {
        return 0;
    }
    
    int64_t max_pos = haystack->length - needle->length;
    
    for (int64_t pos = 0; pos <= max_pos; pos++) {
        int match = 1;
        for (int64_t i = 0; i < needle->length; i++) {
            if (haystack->data[pos + i] != needle->data[i]) {
                match = 0;
                break;
            }
        }
        if (match) {
            return pos + 1;  // Convert to 1-based index for BASIC
        }
    }
    
    return 0;
}

// Get character at index (0-based)
uint32_t string_char_at(StringDescriptor* str, int64_t index) {
    if (!str || index < 0 || index >= str->length) return 0;
    return str->data[index];
}

// Create single character string
StringDescriptor* string_chr(uint32_t codepoint) {
    return string_new_from_utf32(&codepoint, 1);
}

// Bounds check error handler
void basic_string_bounds_error(int64_t index, int64_t min, int64_t max) {
    fprintf(stderr, "String index out of bounds: %lld (valid range: %lld to %lld)\n", 
            (long long)index, (long long)min, (long long)max);
    exit(1);
}

// ============================================================================
// BASIC Runtime Functions (Original)
// ============================================================================

// Runtime initialization
void basic_runtime_init(void) {
    // Initialize runtime (nothing needed for simple test)
}

// Runtime cleanup
void basic_runtime_cleanup(void) {
    // Cleanup runtime (nothing needed for simple test)
}

// Print string
void basic_print_string(const char* str) {
    if (str) {
        printf("%s", str);
    }
}

// Print string descriptor (UTF-32)
void basic_print_string_desc(StringDescriptor* desc) {
    if (desc) {
        const char* utf8 = string_get_utf8(desc);
        printf("%s", utf8);
    }
}

// Print C string (for backward compatibility)
void basic_print_cstr(const char* str) {
    if (str) {
        printf("%s", str);
    }
}

// Print integer
void basic_print_int(int value) {
    printf("%d", value);
}

// Print float (takes double for QBE compatibility)
void basic_print_float(double value) {
    printf("%f", value);
}

// Print double
void basic_print_double(double value) {
    printf("%lf", value);
}

// Print newline
void basic_print_newline(void) {
    printf("\n");
}

// Input integer
int basic_input_int(const char* prompt) {
    int value = 0;
    if (prompt) {
        printf("%s", prompt);
    }
    scanf("%d", &value);
    return value;
}

// Input float
float basic_input_float(const char* prompt) {
    float value = 0.0f;
    if (prompt) {
        printf("%s", prompt);
    }
    scanf("%f", &value);
    return value;
}

// Input double
double basic_input_double(const char* prompt) {
    double value = 0.0;
    if (prompt) {
        printf("%s", prompt);
    }
    scanf("%lf", &value);
    return value;
}

// Input string (allocates memory, caller must free)
char* basic_input_string(const char* prompt) {
    char buffer[1024];
    if (prompt) {
        printf("%s", prompt);
    }
    if (fgets(buffer, sizeof(buffer), stdin)) {
        // Remove trailing newline
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len-1] == '\n') {
            buffer[len-1] = '\0';
        }
        return strdup(buffer);
    }
    return strdup("");
}

// Input string as descriptor (UTF-32)
StringDescriptor* basic_input_line(void) {
    char buffer[1024];
    if (fgets(buffer, sizeof(buffer), stdin)) {
        // Remove trailing newline
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len-1] == '\n') {
            buffer[len-1] = '\0';
        }
        return string_new_utf8(buffer);
    }
    return string_new_empty();
}

// Array operations (stubs)
void* basic_array_create_int(int size) {
    return calloc(size, sizeof(int));
}

void* basic_array_create_float(int size) {
    return calloc(size, sizeof(float));
}

void* basic_array_create_double(int size) {
    return calloc(size, sizeof(double));
}

void* basic_array_create_string(int size) {
    return calloc(size, sizeof(char*));
}

void basic_array_destroy(void* array) {
    if (array) {
        free(array);
    }
}

int basic_array_get_int(void* array, int index) {
    if (array) {
        return ((int*)array)[index];
    }
    return 0;
}

void basic_array_set_int(void* array, int index, int value) {
    if (array) {
        ((int*)array)[index] = value;
    }
}

float basic_array_get_float(void* array, int index) {
    if (array) {
        return ((float*)array)[index];
    }
    return 0.0f;
}

void basic_array_set_float(void* array, int index, float value) {
    if (array) {
        ((float*)array)[index] = value;
    }
}

double basic_array_get_double(void* array, int index) {
    if (array) {
        return ((double*)array)[index];
    }
    return 0.0;
}

void basic_array_set_double(void* array, int index, double value) {
    if (array) {
        ((double*)array)[index] = value;
    }
}

char* basic_array_get_string(void* array, int index) {
    if (array) {
        char* str = ((char**)array)[index];
        return str ? str : "";
    }
    return "";
}

void basic_array_set_string(void* array, int index, const char* value) {
    if (array) {
        char** arr = (char**)array;
        if (arr[index]) {
            free(arr[index]);
        }
        arr[index] = value ? strdup(value) : strdup("");
    }
}

// String operations
char* basic_string_concat(const char* s1, const char* s2) {
    size_t len1 = s1 ? strlen(s1) : 0;
    size_t len2 = s2 ? strlen(s2) : 0;
    char* result = malloc(len1 + len2 + 1);
    if (result) {
        if (s1) strcpy(result, s1);
        if (s2) strcat(result, s2);
    }
    return result;
}

int basic_string_compare(const char* s1, const char* s2) {
    return strcmp(s1 ? s1 : "", s2 ? s2 : "");
}

int basic_string_length(const char* str) {
    return str ? (int)strlen(str) : 0;
}