//
// basic_runtime.c
// FasterBASIC QBE Runtime Library - Core Implementation
//
// NOTE: This is part of the C runtime library (runtime_c/) that gets linked with
//       COMPILED BASIC programs, not the C++ compiler runtime (runtime/).
//
// This file contains runtime initialization, cleanup, and core utilities.
//

#include "basic_runtime.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// =============================================================================
// Global State
// =============================================================================

// Current line number (for error reporting)
static int32_t g_current_line = 0;

// Random number generator state
static bool g_rnd_initialized = false;

// Arena allocator for temporary values
#define ARENA_SIZE (1024 * 1024)  // 1 MB
static char* g_arena = NULL;
static size_t g_arena_offset = 0;

// DATA statement support
static const char** g_data_values = NULL;
static int32_t g_data_count = 0;
static int32_t g_data_index = 0;

// File table (for file operations)
#define MAX_FILES 256
static BasicFile* g_files[MAX_FILES] = {NULL};

// Program start time (for TIMER function)
static int64_t g_program_start_ms = 0;

// =============================================================================
// Runtime Initialization and Cleanup
// =============================================================================

void basic_runtime_init(void) {
    // Allocate arena for temporary values
    g_arena = (char*)malloc(ARENA_SIZE);
    if (!g_arena) {
        fprintf(stderr, "FATAL: Failed to allocate arena memory\n");
        exit(1);
    }
    g_arena_offset = 0;
    
    // Initialize random number generator
    if (!g_rnd_initialized) {
        srand((unsigned int)time(NULL));
        g_rnd_initialized = true;
    }
    
    // Initialize program start time
    g_program_start_ms = basic_timer_ms();
    
    // Initialize file table
    for (int i = 0; i < MAX_FILES; i++) {
        g_files[i] = NULL;
    }
    
    // Reset line number
    g_current_line = 0;
}

void basic_runtime_cleanup(void) {
    // Close all open files
    file_close_all();
    
    // Free arena
    if (g_arena) {
        free(g_arena);
        g_arena = NULL;
    }
    g_arena_offset = 0;
}

// =============================================================================
// Memory Management - Arena Allocator
// =============================================================================

void* basic_alloc_temp(size_t size) {
    // Align to 8 bytes
    size = (size + 7) & ~7;
    
    if (g_arena_offset + size > ARENA_SIZE) {
        fprintf(stderr, "FATAL: Arena memory exhausted\n");
        exit(1);
    }
    
    void* ptr = g_arena + g_arena_offset;
    g_arena_offset += size;
    return ptr;
}

void basic_clear_temps(void) {
    g_arena_offset = 0;
}

// =============================================================================
// Error Handling
// =============================================================================

void basic_error(int32_t line_number, const char* message) {
    fprintf(stderr, "Runtime error at line %d: %s\n", line_number, message);
    exit(1);
}

void basic_error_msg(const char* message) {
    if (g_current_line > 0) {
        fprintf(stderr, "Runtime error at line %d: %s\n", g_current_line, message);
    } else {
        fprintf(stderr, "Runtime error: %s\n", message);
    }
    exit(1);
}

void basic_set_line(int32_t line_number) {
    g_current_line = line_number;
}

int32_t basic_get_line(void) {
    return g_current_line;
}

void basic_array_bounds_error(int64_t index, int64_t lower, int64_t upper) {
    char msg[256];
    snprintf(msg, sizeof(msg), 
             "Array subscript out of bounds: index %lld not in [%lld, %lld]",
             (long long)index, (long long)lower, (long long)upper);
    basic_error_msg(msg);
}

// =============================================================================
// DATA/READ/RESTORE Support
// =============================================================================

void basic_data_init(const char** data_values, int32_t count) {
    g_data_values = data_values;
    g_data_count = count;
    g_data_index = 0;
}

BasicString* basic_read_data_string(void) {
    if (g_data_index >= g_data_count) {
        basic_error_msg("OUT OF DATA");
        return NULL;
    }
    
    BasicString* result = str_new(g_data_values[g_data_index]);
    g_data_index++;
    return result;
}

int32_t basic_read_data_int(void) {
    if (g_data_index >= g_data_count) {
        basic_error_msg("OUT OF DATA");
        return 0;
    }
    
    int32_t result = atoi(g_data_values[g_data_index]);
    g_data_index++;
    return result;
}

double basic_read_data_double(void) {
    if (g_data_index >= g_data_count) {
        basic_error_msg("OUT OF DATA");
        return 0.0;
    }
    
    double result = atof(g_data_values[g_data_index]);
    g_data_index++;
    return result;
}

void basic_restore_data(void) {
    g_data_index = 0;
}

// =============================================================================
// Timer Support
// =============================================================================

#include <sys/time.h>

int64_t basic_timer_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (int64_t)tv.tv_sec * 1000 + (int64_t)tv.tv_usec / 1000;
}

// Get seconds since program start (BASIC TIMER function)
double basic_timer(void) {
    int64_t current_ms = basic_timer_ms();
    return (double)(current_ms - g_program_start_ms) / 1000.0;
}

void basic_sleep_ms(int32_t milliseconds) {
    if (milliseconds <= 0) return;
    
    struct timespec ts;
    ts.tv_sec = milliseconds / 1000;
    ts.tv_nsec = (milliseconds % 1000) * 1000000;
    nanosleep(&ts, NULL);
}

// =============================================================================
// File Management Utilities
// =============================================================================

void file_close_all(void) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (g_files[i]) {
            file_close(g_files[i]);
        }
    }
}

// Internal: Register file in global table
void _basic_register_file(BasicFile* file) {
    if (!file) return;
    
    for (int i = 0; i < MAX_FILES; i++) {
        if (!g_files[i]) {
            g_files[i] = file;
            return;
        }
    }
    
    basic_error_msg("Too many open files");
}

// Internal: Unregister file from global table
void _basic_unregister_file(BasicFile* file) {
    if (!file) return;
    
    for (int i = 0; i < MAX_FILES; i++) {
        if (g_files[i] == file) {
            g_files[i] = NULL;
            return;
        }
    }
}