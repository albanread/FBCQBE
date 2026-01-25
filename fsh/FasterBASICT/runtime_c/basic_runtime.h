//
// basic_runtime.h
// FasterBASIC QBE Runtime Library
//
// This is the C runtime library that supports QBE-generated BASIC programs.
// It provides string management, array operations, I/O, and memory management.
//

#ifndef BASIC_RUNTIME_H
#define BASIC_RUNTIME_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// Type Definitions
// =============================================================================

// String type with reference counting
typedef struct BasicString {
    char* data;           // UTF-8 string data
    size_t length;        // Length in bytes
    size_t capacity;      // Allocated capacity
    int32_t refcount;     // Reference count
} BasicString;

// Array type (multi-dimensional, dynamically allocated)
typedef struct BasicArray {
    void* data;           // Array data
    size_t element_size;  // Size of each element in bytes
    int32_t dimensions;   // Number of dimensions (1-7 typical)
    int32_t* bounds;      // Array bounds [lower1, upper1, lower2, upper2, ...]
    int32_t* strides;     // Strides for each dimension
    int32_t base;         // Array base (0 or 1)
    char type_suffix;     // Type suffix: '%', '#', '!', '$', '&'
} BasicArray;

// File handle
typedef struct BasicFile {
    FILE* fp;
    int32_t file_number;
    char* filename;
    char* mode;
    bool is_open;
} BasicFile;

// =============================================================================
// Memory Management
// =============================================================================

// Initialize runtime (call once at program start)
void basic_runtime_init(void);

// Cleanup runtime (call once at program end)
void basic_runtime_cleanup(void);

// Arena allocator for temporary values (cleared after each statement)
void* basic_alloc_temp(size_t size);
void basic_clear_temps(void);

// =============================================================================
// String Operations
// =============================================================================

// Create new string from C string
BasicString* str_new(const char* cstr);

// Create new string with specific length
BasicString* str_new_length(const char* data, size_t length);

// Create empty string with reserved capacity
BasicString* str_new_capacity(size_t capacity);

// Retain reference to string (increment refcount)
BasicString* str_retain(BasicString* str);

// Release reference to string (decrement refcount, free if 0)
void str_release(BasicString* str);

// Get C string pointer (temporary, valid until next GC)
const char* str_cstr(BasicString* str);

// String concatenation
BasicString* str_concat(BasicString* a, BasicString* b);

// String substring (1-based indexing)
BasicString* str_substr(BasicString* str, int32_t start, int32_t length);

// String left (leftmost n characters)
BasicString* str_left(BasicString* str, int32_t n);

// String right (rightmost n characters)
BasicString* str_right(BasicString* str, int32_t n);

// String comparison (-1: less, 0: equal, 1: greater)
int32_t str_compare(BasicString* a, BasicString* b);

// String length
int32_t str_length(BasicString* str);

// String upper case
BasicString* str_upper(BasicString* str);

// String lower case
BasicString* str_lower(BasicString* str);

// String trim (remove leading/trailing whitespace)
BasicString* str_trim(BasicString* str);

// Find substring (returns 1-based position, 0 if not found)
int32_t str_instr(BasicString* haystack, BasicString* needle);

// String replacement
BasicString* str_replace(BasicString* str, BasicString* find, BasicString* replace);

// =============================================================================
// Type Conversions
// =============================================================================

// Convert integer to string
BasicString* int_to_str(int32_t value);

// Convert long to string
BasicString* long_to_str(int64_t value);

// Convert float to string
BasicString* float_to_str(float value);

// Convert double to string
BasicString* double_to_str(double value);

// Convert string to integer
int32_t str_to_int(BasicString* str);

// Convert string to long
int64_t str_to_long(BasicString* str);

// Convert string to float
float str_to_float(BasicString* str);

// Convert string to double
double str_to_double(BasicString* str);

// =============================================================================
// Array Operations
// =============================================================================

// Create array with specified dimensions
// bounds is array of [lower1, upper1, lower2, upper2, ...]
BasicArray* array_new(char type_suffix, int32_t dimensions, int32_t* bounds, int32_t base);

// Simple array creation wrapper (for codegen convenience)
BasicArray* array_create(int32_t dimensions, ...);

// Free array
void array_free(BasicArray* array);

// Erase array (set to length 0)
void array_erase(BasicArray* array);

// Get element address (for load/store operations)
void* array_get_address(BasicArray* array, int32_t* indices);

// Get integer element
int32_t array_get_int(BasicArray* array, int32_t* indices);

// Set integer element
void array_set_int(BasicArray* array, int32_t* indices, int32_t value);

// Get long element
int64_t array_get_long(BasicArray* array, int32_t* indices);

// Set long element
void array_set_long(BasicArray* array, int32_t* indices, int64_t value);

// Get float element
float array_get_float(BasicArray* array, int32_t* indices);

// Set float element
void array_set_float(BasicArray* array, int32_t* indices, float value);

// Get double element
double array_get_double(BasicArray* array, int32_t* indices);

// Set double element
void array_set_double(BasicArray* array, int32_t* indices, double value);

// Get string element
BasicString* array_get_string(BasicArray* array, int32_t* indices);

// Set string element
void array_set_string(BasicArray* array, int32_t* indices, BasicString* value);

// Get lower bound for dimension (1-based)
int32_t array_lbound(BasicArray* array, int32_t dimension);

// Get upper bound for dimension (1-based)
int32_t array_ubound(BasicArray* array, int32_t dimension);

// Redimension array (preserves data if possible)
void array_redim(BasicArray* array, int32_t* new_bounds, bool preserve);

// =============================================================================
// I/O Operations - Console
// =============================================================================

// Print integer
void basic_print_int(int32_t value);

// Print long
void basic_print_long(int64_t value);

// Print float
void basic_print_float(float value);

// Print double
void basic_print_double(double value);

// Print string
void basic_print_string(BasicString* str);

// Print C string literal (for compile-time constants)
void basic_print_cstr(const char* str);

// Print newline
void basic_print_newline(void);

// Print tab
void basic_print_tab(void);

// Print at position (row, col) - 1-based
void basic_print_at(int32_t row, int32_t col, BasicString* str);

// Input string from console
BasicString* basic_input_string(void);

// Input with prompt
BasicString* basic_input_prompt(BasicString* prompt);

// Input integer
int32_t basic_input_int(void);

// Input double
double basic_input_double(void);

// Clear screen
void basic_cls(void);

// =============================================================================
// I/O Operations - File
// =============================================================================

// Open file
BasicFile* file_open(BasicString* filename, BasicString* mode);

// Close file
void file_close(BasicFile* file);

// Close all files
void file_close_all(void);

// Print to file
void file_print_string(BasicFile* file, BasicString* str);

// Print integer to file
void file_print_int(BasicFile* file, int32_t value);

// Print newline to file
void file_print_newline(BasicFile* file);

// Read line from file
BasicString* file_read_line(BasicFile* file);

// Check if end of file
bool file_eof(BasicFile* file);

// =============================================================================
// Math Functions
// =============================================================================

// Absolute value (integer)
int32_t basic_abs_int(int32_t x);

// Absolute value (double)
double basic_abs_double(double x);

// Square root
double basic_sqrt(double x);

// Power
double basic_pow(double base, double exponent);

// Sine
double basic_sin(double x);

// Cosine
double basic_cos(double x);

// Tangent
double basic_tan(double x);

// Arc tangent
double basic_atan(double x);

// Arc tangent 2
double basic_atan2(double y, double x);

// Natural logarithm
double basic_log(double x);

// Exponential
double basic_exp(double x);

// Random number (0.0 to 1.0)
double basic_rnd(void);

// Random integer (min to max inclusive)
int32_t basic_rnd_int(int32_t min, int32_t max);

// Randomize seed
void basic_randomize(int32_t seed);

// Integer part
int32_t basic_int(double x);

// Sign (-1, 0, or 1)
int32_t basic_sgn(double x);

// =============================================================================
// Error Handling
// =============================================================================

// Runtime error with line number
void basic_error(int32_t line_number, const char* message);

// Runtime error without line number
void basic_error_msg(const char* message);

// Check array bounds (internal use)
void basic_check_bounds(BasicArray* array, int32_t* indices);

// Array bounds error handler (for descriptor-based arrays)
void basic_array_bounds_error(int64_t index, int64_t lower, int64_t upper);

// =============================================================================
// DATA/READ/RESTORE Support
// =============================================================================

// Initialize data pointer
void basic_data_init(const char** data_values, int32_t count);

// Read next data value as string
BasicString* basic_read_data_string(void);

// Read next data value as integer
int32_t basic_read_data_int(void);

// Read next data value as double
double basic_read_data_double(void);

// Restore data pointer to beginning
void basic_restore_data(void);

// =============================================================================
// Timer Support (if needed)
// =============================================================================

// Get current time in milliseconds
int64_t basic_timer_ms(void);

// Sleep for milliseconds
void basic_sleep_ms(int32_t milliseconds);

// =============================================================================
// Debugging Support
// =============================================================================

// Set current line number (for error reporting)
void basic_set_line(int32_t line_number);

// Get current line number
int32_t basic_get_line(void);

// =============================================================================
// DATA/READ/RESTORE Support
// =============================================================================

// Read values from DATA statements
int32_t basic_read_int(void);
double basic_read_double(void);
const char* basic_read_string(void);

// Restore DATA pointer to a specific position
void basic_restore(int64_t index);

// Restore DATA pointer to the beginning
void basic_restore_start(void);

#ifdef __cplusplus
}
#endif

#endif // BASIC_RUNTIME_H