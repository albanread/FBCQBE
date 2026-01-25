//
// array_descriptor.h
// FasterBASIC Runtime - Array Descriptor (Dope Vector)
//
// Defines the array descriptor structure used for efficient bounds checking
// and dynamic array operations (DIM, REDIM, REDIM PRESERVE, ERASE).
//

#ifndef ARRAY_DESCRIPTOR_H
#define ARRAY_DESCRIPTOR_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

//
// ArrayDescriptor: Tracks array metadata for bounds checking and reallocation
//
// Memory layout (kept in sync with QBE codegen):
//   Offset 0:  void*   data         - Pointer to array data
//   Offset 8:  int64_t lowerBound   - Lower index bound (typically 0 or 1)
//   Offset 16: int64_t upperBound   - Upper index bound
//   Offset 24: int64_t elementSize  - Size of each element in bytes
//   Offset 32: int32_t dimensions   - Number of dimensions (1 for 1D arrays)
//   Offset 36: int32_t base         - OPTION BASE (0 or 1)
//   Offset 40: char    typeSuffix   - BASIC suffix ('%', '!', '#', '$', '&' or 0 for UDT)
//   Offset 41: char[7] _padding     - Padding / future use
//
// Total size: 48 bytes (aligned)
//
typedef struct {
    void*    data;          // Pointer to the array data
    int64_t  lowerBound;    // Lower index bound
    int64_t  upperBound;    // Upper index bound
    int64_t  elementSize;   // Size per element in bytes
    int32_t  dimensions;    // Number of dimensions
    int32_t  base;          // OPTION BASE (0 or 1)
    char     typeSuffix;    // BASIC type suffix; 0 for UDT/opaque
    char     _padding[7];   // Padding for alignment / future use
} ArrayDescriptor;

//
// Runtime helper functions for array operations
//

// Initialize a new array descriptor
// Returns 0 on success, -1 on failure
static inline int array_descriptor_init(
    ArrayDescriptor* desc,
    int64_t lowerBound,
    int64_t upperBound,
    int64_t elementSize,
    int32_t dimensions,
    int32_t base,
    char typeSuffix)
{
    if (!desc || upperBound < lowerBound || elementSize <= 0) {
        return -1;
    }

    int64_t count = upperBound - lowerBound + 1;
    size_t totalSize = (size_t)(count * elementSize);

    desc->data = malloc(totalSize);
    if (!desc->data) {
        return -1;
    }

    // Zero-initialize the array data
    memset(desc->data, 0, totalSize);

    desc->lowerBound = lowerBound;
    desc->upperBound = upperBound;
    desc->elementSize = elementSize;
    desc->dimensions = dimensions;
    desc->base = base;
    desc->typeSuffix = typeSuffix;
    memset(desc->_padding, 0, sizeof(desc->_padding));

    return 0;
}

// Free array data (for ERASE or before REDIM)
static inline void array_descriptor_free(ArrayDescriptor* desc)
{
    if (desc && desc->data) {
        free(desc->data);
        desc->data = NULL;
        desc->lowerBound = 0;
        desc->upperBound = -1;
    }
}

// REDIM: Free old data and allocate new
static inline int array_descriptor_redim(
    ArrayDescriptor* desc,
    int64_t newLowerBound,
    int64_t newUpperBound)
{
    if (!desc || newUpperBound < newLowerBound) {
        return -1;
    }

    // Free old data
    if (desc->data) {
        free(desc->data);
        desc->data = NULL;
    }

    // Allocate new data
    int64_t newCount = newUpperBound - newLowerBound + 1;
    size_t totalSize = (size_t)(newCount * desc->elementSize);

    desc->data = malloc(totalSize);
    if (!desc->data) {
        desc->lowerBound = 0;
        desc->upperBound = -1;
        return -1;
    }

    // Zero-initialize
    memset(desc->data, 0, totalSize);

    desc->lowerBound = newLowerBound;
    desc->upperBound = newUpperBound;

    return 0;
}

// REDIM PRESERVE: Resize array keeping existing data
static inline int array_descriptor_redim_preserve(
    ArrayDescriptor* desc,
    int64_t newLowerBound,
    int64_t newUpperBound)
{

// ERASE helper: implemented in array_descriptor_runtime.c
void array_descriptor_erase(ArrayDescriptor* desc);
    if (!desc || newUpperBound < newLowerBound) {
        return -1;
    }

    int64_t oldCount = desc->upperBound - desc->lowerBound + 1;
    int64_t newCount = newUpperBound - newLowerBound + 1;
    size_t oldSize = (size_t)(oldCount * desc->elementSize);
    size_t newSize = (size_t)(newCount * desc->elementSize);

    // Use realloc to resize
    void* newData = realloc(desc->data, newSize);
    if (!newData) {
        return -1;
    }

    desc->data = newData;

    // If growing, zero-fill the new elements
    if (newSize > oldSize) {
        char* fillStart = (char*)desc->data + oldSize;
        size_t fillSize = newSize - oldSize;
        memset(fillStart, 0, fillSize);
    }

    // Handle index shift if lower bound changed
    // Note: This is a simplified version. Full implementation would need
    // to copy data to account for index offset changes.
    if (newLowerBound != desc->lowerBound && newCount > 0 && oldCount > 0) {
        // For now, we assume bounds don't change much
        // A full implementation would memmove data based on index shift
    }

    desc->lowerBound = newLowerBound;
    desc->upperBound = newUpperBound;

    return 0;
}

// Bounds check - returns 1 if index is valid, 0 if out of bounds
static inline int array_descriptor_check_bounds(
    const ArrayDescriptor* desc,
    int64_t index)
{
    return (desc && index >= desc->lowerBound && index <= desc->upperBound);
}

// Calculate element pointer for given index (no bounds check)
static inline void* array_descriptor_get_element_ptr(
    const ArrayDescriptor* desc,
    int64_t index)
{
    int64_t offset = (index - desc->lowerBound) * desc->elementSize;
    return (char*)desc->data + offset;
}

// Runtime error handler for bounds violations
// This should be called when bounds check fails
extern void basic_array_bounds_error(int64_t index, int64_t lower, int64_t upper);

#ifdef __cplusplus
}
#endif

#endif // ARRAY_DESCRIPTOR_H