//
// io_ops.c
// FasterBASIC QBE Runtime Library - I/O Operations
//
// This file implements console and file I/O operations.
//

#include "basic_runtime.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// =============================================================================
// Console Output
// =============================================================================

void basic_print_int(int32_t value) {
    printf("%d", value);
    fflush(stdout);
}

void basic_print_long(int64_t value) {
    printf("%lld", (long long)value);
    fflush(stdout);
}

void basic_print_float(float value) {
    printf("%g", value);
    fflush(stdout);
}

void basic_print_double(double value) {
    printf("%g", value);
    fflush(stdout);
}

void basic_print_string(BasicString* str) {
    if (!str) return;
    printf("%s", str->data);
    fflush(stdout);
}

// Print a C string literal (for compile-time string constants)
void basic_print_cstr(const char* str) {
    if (!str) return;
    printf("%s", str);
    fflush(stdout);
}

void basic_print_newline(void) {
    printf("\n");
    fflush(stdout);
}

void basic_print_tab(void) {
    printf("\t");
    fflush(stdout);
}

void basic_print_at(int32_t row, int32_t col, BasicString* str) {
    // ANSI escape codes for cursor positioning (1-based)
    printf("\033[%d;%dH", row, col);
    if (str) {
        printf("%s", str->data);
    }
    fflush(stdout);
}

void basic_cls(void) {
    // ANSI escape code to clear screen and move cursor to home
    printf("\033[2J\033[H");
    fflush(stdout);
}

// =============================================================================
// Console Input
// =============================================================================

BasicString* basic_input_string(void) {
    char buffer[4096];
    
    if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
        return str_new("");
    }
    
    // Remove trailing newline
    size_t len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n') {
        buffer[len - 1] = '\0';
    }
    
    return str_new(buffer);
}

BasicString* basic_input_prompt(BasicString* prompt) {
    if (prompt && prompt->length > 0) {
        printf("%s", prompt->data);
        fflush(stdout);
    }
    
    return basic_input_string();
}

int32_t basic_input_int(void) {
    BasicString* str = basic_input_string();
    int32_t result = str_to_int(str);
    str_release(str);
    return result;
}

double basic_input_double(void) {
    BasicString* str = basic_input_string();
    double result = str_to_double(str);
    str_release(str);
    return result;
}

// =============================================================================
// File Operations
// =============================================================================

// Forward declarations for internal functions
extern void _basic_register_file(BasicFile* file);
extern void _basic_unregister_file(BasicFile* file);

BasicFile* file_open(BasicString* filename, BasicString* mode) {
    if (!filename || !mode) {
        basic_error_msg("Invalid file open parameters");
        return NULL;
    }
    
    BasicFile* file = (BasicFile*)malloc(sizeof(BasicFile));
    if (!file) {
        basic_error_msg("Out of memory (file allocation)");
        return NULL;
    }
    
    file->filename = strdup(filename->data);
    file->mode = strdup(mode->data);
    file->file_number = 0;  // Will be set by caller if needed
    file->is_open = false;
    
    // Open the file
    file->fp = fopen(filename->data, mode->data);
    if (!file->fp) {
        free(file->filename);
        free(file->mode);
        free(file);
        
        char err_msg[256];
        snprintf(err_msg, sizeof(err_msg), "Cannot open file: %s", filename->data);
        basic_error_msg(err_msg);
        return NULL;
    }
    
    file->is_open = true;
    _basic_register_file(file);
    
    return file;
}

void file_close(BasicFile* file) {
    if (!file) return;
    
    if (file->is_open && file->fp) {
        fclose(file->fp);
        file->fp = NULL;
        file->is_open = false;
    }
    
    _basic_unregister_file(file);
    
    if (file->filename) {
        free(file->filename);
        file->filename = NULL;
    }
    
    if (file->mode) {
        free(file->mode);
        file->mode = NULL;
    }
    
    free(file);
}

void file_print_string(BasicFile* file, BasicString* str) {
    if (!file || !file->is_open || !file->fp) {
        basic_error_msg("File not open for writing");
        return;
    }
    
    if (!str) return;
    
    fprintf(file->fp, "%s", str->data);
    fflush(file->fp);
}

void file_print_int(BasicFile* file, int32_t value) {
    if (!file || !file->is_open || !file->fp) {
        basic_error_msg("File not open for writing");
        return;
    }
    
    fprintf(file->fp, "%d", value);
    fflush(file->fp);
}

void file_print_newline(BasicFile* file) {
    if (!file || !file->is_open || !file->fp) {
        basic_error_msg("File not open for writing");
        return;
    }
    
    fprintf(file->fp, "\n");
    fflush(file->fp);
}

BasicString* file_read_line(BasicFile* file) {
    if (!file || !file->is_open || !file->fp) {
        basic_error_msg("File not open for reading");
        return str_new("");
    }
    
    char buffer[4096];
    
    if (fgets(buffer, sizeof(buffer), file->fp) == NULL) {
        return str_new("");
    }
    
    // Remove trailing newline
    size_t len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n') {
        buffer[len - 1] = '\0';
    }
    
    return str_new(buffer);
}

bool file_eof(BasicFile* file) {
    if (!file || !file->is_open || !file->fp) {
        return true;
    }
    
    return feof(file->fp) != 0;
}