//
// array_ops.c
// FasterBASIC QBE Runtime Library - Array Operations
//
// This file implements dynamic array management with bounds checking.
// Arrays can be multi-dimensional and support OPTION BASE 0 or 1.
//

#include "basic_runtime.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

// =============================================================================
// Array Creation
// =============================================================================

BasicArray* array_new(char type_suffix, int32_t dimensions, int32_t* bounds, int32_t base) {
    if (dimensions <= 0 || dimensions > 8) {
        basic_error_msg("Invalid array dimensions");
        return NULL;
    }
    
    if (!bounds) {
        basic_error_msg("Array bounds not specified");
        return NULL;
    }
    
    BasicArray* array = (BasicArray*)malloc(sizeof(BasicArray));
    if (!array) {
        basic_error_msg("Out of memory (array allocation)");
        return NULL;
    }
    
    array->dimensions = dimensions;
    array->base = base;
    array->type_suffix = type_suffix;
    
    // Allocate bounds array: [lower1, upper1, lower2, upper2, ...]
    array->bounds = (int32_t*)malloc(dimensions * 2 * sizeof(int32_t));
    if (!array->bounds) {
        free(array);
        basic_error_msg("Out of memory (array bounds)");
        return NULL;
    }
    memcpy(array->bounds, bounds, dimensions * 2 * sizeof(int32_t));
    
    // Calculate strides for each dimension
    array->strides = (int32_t*)malloc(dimensions * sizeof(int32_t));
    if (!array->strides) {
        free(array->bounds);
        free(array);
        basic_error_msg("Out of memory (array strides)");
        return NULL;
    }
    
    // Determine element size based on type suffix
    switch (type_suffix) {
        case '%': // INTEGER
            array->element_size = sizeof(int32_t);
            break;
        case '&': // LONG
            array->element_size = sizeof(int64_t);
            break;
        case '!': // SINGLE
            array->element_size = sizeof(float);
            break;
        case '#': // DOUBLE
            array->element_size = sizeof(double);
            break;
        case '$': // STRING (StringDescriptor*)
            array->element_size = sizeof(StringDescriptor*);
            break;
        default:
            // Default to DOUBLE for untyped arrays
            array->element_size = sizeof(double);
            type_suffix = '#';
            break;
    }
    
    // Calculate total size and strides
    size_t total_elements = 1;
    for (int32_t i = dimensions - 1; i >= 0; i--) {
        int32_t lower = bounds[i * 2];
        int32_t upper = bounds[i * 2 + 1];
        int32_t size = upper - lower + 1;
        
        if (size <= 0) {
            free(array->strides);
            free(array->bounds);
            free(array);
            basic_error_msg("Invalid array bounds");
            return NULL;
        }
        
        array->strides[i] = (int32_t)total_elements;
        total_elements *= size;
    }
    
    // Allocate data
    size_t data_size = total_elements * array->element_size;
    array->data = malloc(data_size);
    if (!array->data) {
        free(array->strides);
        free(array->bounds);
        free(array);
        basic_error_msg("Out of memory (array data)");
        return NULL;
    }
    
    // Initialize to zero
    memset(array->data, 0, data_size);
    
    return array;
}

// =============================================================================
// Array Destruction
// =============================================================================

void array_free(BasicArray* array) {
    if (!array) return;
    
    // If string array, release all strings
    if (array->type_suffix == '$' && array->data) {
        // Calculate total elements
        size_t total_elements = 1;
        for (int32_t i = 0; i < array->dimensions; i++) {
            int32_t lower = array->bounds[i * 2];
            int32_t upper = array->bounds[i * 2 + 1];
            total_elements *= (upper - lower + 1);
        }
        
        StringDescriptor** strings = (StringDescriptor**)array->data;
        for (size_t i = 0; i < total_elements; i++) {
            if (strings[i]) {
                string_release(strings[i]);
            }
        }
    }
    
    if (array->data) free(array->data);
    if (array->bounds) free(array->bounds);
    if (array->strides) free(array->strides);
    free(array);
}

// =============================================================================
// Index Calculation
// =============================================================================

static size_t calculate_offset(BasicArray* array, int32_t* indices) {
    size_t offset = 0;
    
    for (int32_t i = 0; i < array->dimensions; i++) {
        int32_t lower = array->bounds[i * 2];
        int32_t upper = array->bounds[i * 2 + 1];
        int32_t index = indices[i];
        
        // Bounds check
        if (index < lower || index > upper) {
            char msg[256];
            snprintf(msg, sizeof(msg), 
                "Array subscript out of range (dimension %d: %d not in [%d, %d])",
                i + 1, index, lower, upper);
            basic_error_msg(msg);
            return 0;
        }
        
        offset += (index - lower) * array->strides[i];
    }
    
    return offset;
}

// =============================================================================
// Get Element Address
// =============================================================================

void* array_get_address(BasicArray* array, int32_t* indices) {
    if (!array || !indices) return NULL;
    
    size_t offset = calculate_offset(array, indices);
    return (char*)array->data + (offset * array->element_size);
}

// =============================================================================
// Integer Array Operations
// =============================================================================

int32_t array_get_int(BasicArray* array, int32_t* indices) {
    if (!array || array->type_suffix != '%') {
        basic_error_msg("Type mismatch in array access");
        return 0;
    }
    
    size_t offset = calculate_offset(array, indices);
    int32_t* data = (int32_t*)array->data;
    return data[offset];
}

void array_set_int(BasicArray* array, int32_t* indices, int32_t value) {
    if (!array || array->type_suffix != '%') {
        basic_error_msg("Type mismatch in array assignment");
        return;
    }
    
    size_t offset = calculate_offset(array, indices);
    int32_t* data = (int32_t*)array->data;
    data[offset] = value;
}

// =============================================================================
// Long Array Operations
// =============================================================================

int64_t array_get_long(BasicArray* array, int32_t* indices) {
    if (!array || array->type_suffix != '&') {
        basic_error_msg("Type mismatch in array access");
        return 0;
    }
    
    size_t offset = calculate_offset(array, indices);
    int64_t* data = (int64_t*)array->data;
    return data[offset];
}

void array_set_long(BasicArray* array, int32_t* indices, int64_t value) {
    if (!array || array->type_suffix != '&') {
        basic_error_msg("Type mismatch in array assignment");
        return;
    }
    
    size_t offset = calculate_offset(array, indices);
    int64_t* data = (int64_t*)array->data;
    data[offset] = value;
}

// =============================================================================
// Float Array Operations
// =============================================================================

float array_get_float(BasicArray* array, int32_t* indices) {
    if (!array || array->type_suffix != '!') {
        basic_error_msg("Type mismatch in array access");
        return 0.0f;
    }
    
    size_t offset = calculate_offset(array, indices);
    float* data = (float*)array->data;
    return data[offset];
}

void array_set_float(BasicArray* array, int32_t* indices, float value) {
    if (!array || array->type_suffix != '!') {
        basic_error_msg("Type mismatch in array assignment");
        return;
    }
    
    size_t offset = calculate_offset(array, indices);
    float* data = (float*)array->data;
    data[offset] = value;
}

// =============================================================================
// Double Array Operations
// =============================================================================

double array_get_double(BasicArray* array, int32_t* indices) {
    if (!array || array->type_suffix != '#') {
        basic_error_msg("Type mismatch in array access");
        return 0.0;
    }
    
    size_t offset = calculate_offset(array, indices);
    double* data = (double*)array->data;
    return data[offset];
}

void array_set_double(BasicArray* array, int32_t* indices, double value) {
    if (!array || array->type_suffix != '#') {
        basic_error_msg("Type mismatch in array assignment");
        return;
    }
    
    size_t offset = calculate_offset(array, indices);
    double* data = (double*)array->data;
    data[offset] = value;
}

// =============================================================================
// String Array Operations
// =============================================================================

StringDescriptor* array_get_string(BasicArray* array, int32_t* indices) {
    if (!array || array->type_suffix != '$') {
        basic_error_msg("Type mismatch in array access");
        return string_new_capacity(0);
    }
    
    size_t offset = calculate_offset(array, indices);
    StringDescriptor** data = (StringDescriptor**)array->data;
    
    return string_retain(data[offset]);
}

void array_set_string(BasicArray* array, int32_t* indices, StringDescriptor* value) {
    if (!array || array->type_suffix != '$') {
        basic_error_msg("Type mismatch in array assignment");
        return;
    }
    
    size_t offset = calculate_offset(array, indices);
    StringDescriptor** data = (StringDescriptor**)array->data;
    
    if (data[offset]) {
        string_release(data[offset]);
    }
    
    data[offset] = string_retain(value);
}

// =============================================================================
// Array Bounds Inquiry
// =============================================================================

int32_t array_lbound(BasicArray* array, int32_t dimension) {
    if (!array || dimension < 1 || dimension > array->dimensions) {
        basic_error_msg("Invalid dimension in LBOUND");
        return 0;
    }
    
    return array->bounds[(dimension - 1) * 2];
}

int32_t array_ubound(BasicArray* array, int32_t dimension) {
    if (!array || dimension < 1 || dimension > array->dimensions) {
        basic_error_msg("Invalid dimension in UBOUND");
        return 0;
    }
    
    return array->bounds[(dimension - 1) * 2 + 1];
}

// =============================================================================
// Array Redimension
// =============================================================================

void array_redim(BasicArray* array, int32_t* new_bounds, bool preserve) {
    if (!array || !new_bounds) {
        basic_error_msg("Invalid REDIM parameters");
        return;
    }
    
    // For now, we'll implement a simple version that doesn't preserve data
    // A full implementation would copy over overlapping elements
    if (preserve) {
        // TODO: Implement REDIM PRESERVE
        basic_error_msg("REDIM PRESERVE not yet implemented");
        return;
    }
    
    // Free old data
    if (array->data) {
        if (array->type_suffix == '$') {
            // Release all strings
            size_t total_elements = 1;
            for (int32_t i = 0; i < array->dimensions; i++) {
                int32_t lower = array->bounds[i * 2];
                int32_t upper = array->bounds[i * 2 + 1];
                total_elements *= (upper - lower + 1);
            }
            
            StringDescriptor** strings = (StringDescriptor**)array->data;
            for (size_t i = 0; i < total_elements; i++) {
                if (strings[i]) {
                    string_release(strings[i]);
                }
            }
        }
        free(array->data);
    }
    
    // Update bounds
    memcpy(array->bounds, new_bounds, array->dimensions * 2 * sizeof(int32_t));
    
    // Recalculate strides and total size
    size_t total_elements = 1;
    for (int32_t i = array->dimensions - 1; i >= 0; i--) {
        int32_t lower = new_bounds[i * 2];
        int32_t upper = new_bounds[i * 2 + 1];
        int32_t size = upper - lower + 1;
        
        if (size <= 0) {
            basic_error_msg("Invalid array bounds in REDIM");
            return;
        }
        
        array->strides[i] = (int32_t)total_elements;
        total_elements *= size;
    }
    
    // Allocate new data
    size_t data_size = total_elements * array->element_size;
    array->data = malloc(data_size);
    if (!array->data) {
        basic_error_msg("Out of memory (REDIM)");
        return;
    }
    
    // Initialize to zero
    memset(array->data, 0, data_size);
}

// =============================================================================
// Bounds Checking
// =============================================================================

void basic_check_bounds(BasicArray* array, int32_t* indices) {
    if (!array || !indices) return;
    
    for (int32_t i = 0; i < array->dimensions; i++) {
        int32_t lower = array->bounds[i * 2];
        int32_t upper = array->bounds[i * 2 + 1];
        int32_t index = indices[i];
        
        if (index < lower || index > upper) {
            char msg[256];
            snprintf(msg, sizeof(msg), 
                "Array subscript out of range (dimension %d: %d not in [%d, %d])",
                i + 1, index, lower, upper);
            basic_error_msg(msg);
            return;
        }
    }
}

// =============================================================================
// Convenience Wrappers for Codegen
// =============================================================================

// Simple array creation wrapper for codegen
// Creates a 1D array with default type (double '#')
BasicArray* array_create(int32_t dimensions, ...) {
if (dimensions <= 0 || dimensions > 8) {
    basic_error_msg("Invalid array dimensions in array_create");
    return NULL;
}
    
// Allocate bounds array
int32_t* bounds = (int32_t*)malloc(dimensions * 2 * sizeof(int32_t));
if (!bounds) {
    basic_error_msg("Out of memory (array_create bounds)");
    return NULL;
}
    
// Extract dimension sizes from varargs
va_list args;
va_start(args, dimensions);
    
for (int32_t i = 0; i < dimensions; i++) {
    int32_t size = va_arg(args, int32_t);
    bounds[i * 2] = 0;      // Lower bound (OPTION BASE 0 by default)
    bounds[i * 2 + 1] = size;  // Upper bound
}
    
va_end(args);
    
// Create array with default type (double '#')
BasicArray* array = array_new('#', dimensions, bounds, 0);
    
free(bounds);
return array;
}

// Erase an array (set to length 0)
void array_erase(BasicArray* array) {
if (!array) return;
    
// Create new bounds with size 0 for all dimensions
int32_t* new_bounds = (int32_t*)malloc(array->dimensions * 2 * sizeof(int32_t));
if (!new_bounds) {
    basic_error_msg("Out of memory (array_erase)");
    return;
}
    
for (int32_t i = 0; i < array->dimensions; i++) {
    new_bounds[i * 2] = 0;
    new_bounds[i * 2 + 1] = -1;  // Make upper < lower to indicate empty
}
    
// Redim to size 0 without preserve
array_redim(array, new_bounds, false);
    
free(new_bounds);
}