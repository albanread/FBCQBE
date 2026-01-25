//
// basic_data.c
// FasterBASIC QBE Runtime Library - DATA/READ/RESTORE Support
//
// NOTE: This is part of the C runtime library (runtime_c/) that gets linked with
//       COMPILED BASIC programs, not the C++ compiler runtime (runtime/).
//
// This file contains runtime support for BASIC DATA, READ, and RESTORE statements.
//

#include "basic_runtime.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// =============================================================================
// External DATA Section References
// =============================================================================

// These symbols are defined in the generated QBE IL code
extern int64_t __basic_data[];        // Array of data values
extern uint8_t __basic_data_types[];  // Array of type tags (0=INT, 1=DOUBLE, 2=STRING)
extern int64_t __basic_data_ptr;      // Current read position

// =============================================================================
// Type Enumeration
// =============================================================================

#define DATA_TYPE_INT    0
#define DATA_TYPE_DOUBLE 1
#define DATA_TYPE_STRING 2

// =============================================================================
// READ Functions
// =============================================================================

// Read an integer value from DATA
int32_t basic_read_int(void) {
    // Check bounds
    if (__basic_data_ptr < 0) {
        fprintf(stderr, "ERROR: DATA pointer is negative\n");
        exit(1);
    }
    
    int64_t idx = __basic_data_ptr;
    uint8_t type = __basic_data_types[idx];
    
    // Type check
    if (type != DATA_TYPE_INT) {
        fprintf(stderr, "ERROR: Type mismatch in READ - expected INT at position %lld\n", 
                (long long)idx);
        exit(1);
    }
    
    // Read value and advance pointer
    int32_t value = (int32_t)__basic_data[idx];
    __basic_data_ptr++;
    
    return value;
}

// Read a double value from DATA
double basic_read_double(void) {
    // Check bounds
    if (__basic_data_ptr < 0) {
        fprintf(stderr, "ERROR: DATA pointer is negative\n");
        exit(1);
    }
    
    int64_t idx = __basic_data_ptr;
    uint8_t type = __basic_data_types[idx];
    
    // Type check - allow INT to be read as DOUBLE
    if (type == DATA_TYPE_INT) {
        int32_t value = (int32_t)__basic_data[idx];
        __basic_data_ptr++;
        return (double)value;
    } else if (type == DATA_TYPE_DOUBLE) {
        // Reinterpret the bits as double
        union {
            int64_t i;
            double d;
        } converter;
        converter.i = __basic_data[idx];
        __basic_data_ptr++;
        return converter.d;
    } else {
        fprintf(stderr, "ERROR: Type mismatch in READ - expected DOUBLE at position %lld\n", 
                (long long)idx);
        exit(1);
    }
}

// Read a string value from DATA
const char* basic_read_string(void) {
    // Check bounds
    if (__basic_data_ptr < 0) {
        fprintf(stderr, "ERROR: DATA pointer is negative\n");
        exit(1);
    }
    
    int64_t idx = __basic_data_ptr;
    uint8_t type = __basic_data_types[idx];
    
    // Type check
    if (type != DATA_TYPE_STRING) {
        fprintf(stderr, "ERROR: Type mismatch in READ - expected STRING at position %lld\n", 
                (long long)idx);
        exit(1);
    }
    
    // Read pointer value and advance pointer
    const char* str = (const char*)__basic_data[idx];
    __basic_data_ptr++;
    
    return str;
}

// =============================================================================
// RESTORE Function
// =============================================================================

// Restore DATA pointer to a specific position
void basic_restore(int64_t index) {
    __basic_data_ptr = index;
}

// Restore DATA pointer to the beginning
void basic_restore_start(void) {
    __basic_data_ptr = 0;
}
