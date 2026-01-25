//
// array_descriptor_runtime.c
// Runtime helpers for ArrayDescriptor
//
// Provides erase helpers that release string elements before freeing data.
//

#include "array_descriptor.h"
#include "string_descriptor.h"
#include <stdlib.h>

void array_descriptor_erase(ArrayDescriptor* desc) {
    if (!desc) return;

    if (desc->data && desc->typeSuffix == '$') {
        int64_t count = desc->upperBound - desc->lowerBound + 1;
        if (count > 0) {
            StringDescriptor** elems = (StringDescriptor**)desc->data;
            for (int64_t i = 0; i < count; i++) {
                if (elems[i]) {
                    string_release(elems[i]);
                }
            }
        }
    }

    if (desc->data) {
        free(desc->data);
        desc->data = NULL;
    }

    // Mark empty
    desc->lowerBound = 0;
    desc->upperBound = -1;
}

// Fully destroy a descriptor: erase contents and free descriptor itself
void array_descriptor_destroy(ArrayDescriptor* desc) {
    if (!desc) return;
    array_descriptor_erase(desc);
    free(desc);
}
