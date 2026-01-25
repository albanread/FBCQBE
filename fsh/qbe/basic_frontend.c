/* FasterBASIC Frontend Integration for QBE
 * Compiles BASIC source to QBE IL in memory, then passes to QBE backend
 */

#include "all.h"
#include <string.h>

/* Compile BASIC source file to QBE IL in memory
 * Returns: FILE* to memory buffer containing QBE IL, or NULL on error
 */
FILE* compile_basic_to_il(const char *basic_path) {
    char cmd[2048];
    FILE *pipe;
    char *buffer = NULL;
    size_t buffer_size = 0;
    size_t total_read = 0;
    char chunk[4096];
    size_t n;
    
    /* Run the BASIC compiler to emit QBE IL to stdout */
    snprintf(cmd, sizeof(cmd), "./basic --emit-qbe '%s' -o /dev/stdout 2>/dev/null", basic_path);
    
    pipe = popen(cmd, "r");
    if (!pipe) {
        return NULL;
    }
    
    /* Read all output into memory buffer */
    while ((n = fread(chunk, 1, sizeof(chunk), pipe)) > 0) {
        char *new_buffer = realloc(buffer, total_read + n + 1);
        if (!new_buffer) {
            free(buffer);
            pclose(pipe);
            return NULL;
        }
        buffer = new_buffer;
        memcpy(buffer + total_read, chunk, n);
        total_read += n;
    }
    
    pclose(pipe);
    
    if (total_read == 0) {
        free(buffer);
        return NULL;
    }
    
    buffer[total_read] = '\0';
    
    /* Create FILE* from memory buffer using fmemopen */
    FILE *mem_file = fmemopen(buffer, total_read, "r");
    if (!mem_file) {
        free(buffer);
        return NULL;
    }
    
    /* Note: buffer will leak here, but that's okay for a compiler that exits soon
     * In production, we'd need better memory management */
    
    return mem_file;
}

/* Check if filename ends with .bas or .BAS */
int is_basic_file(const char *filename) {
    size_t len = strlen(filename);
    if (len < 4) return 0;
    
    const char *ext = filename + len - 4;
    return (strcmp(ext, ".bas") == 0 || strcmp(ext, ".BAS") == 0);
}
