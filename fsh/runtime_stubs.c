// runtime_stubs.c
// Minimal runtime stubs for testing FasterBASIC QBE backend

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Runtime initialization
void basic_init(void) {
    // Initialize runtime (nothing needed for simple test)
}

// Runtime cleanup
void basic_cleanup(void) {
    // Cleanup runtime (nothing needed for simple test)
}

// Print string
void basic_print_string(const char* str) {
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