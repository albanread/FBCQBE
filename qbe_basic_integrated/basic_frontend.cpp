/* FasterBASIC Frontend Integration for QBE
 * Compiles BASIC source to QBE IL in memory using embedded compiler
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Forward declare the C++ function from fasterbasic_wrapper.cpp */
extern "C" char* compile_basic_to_qbe_string(const char *basic_path);
extern "C" void set_trace_cfg_impl(int enable);
extern "C" void set_trace_ast_impl(int enable);

extern "C" {

/* Compile BASIC source file to QBE IL in memory
 * Returns: FILE* to memory buffer containing QBE IL, or NULL on error
 */
FILE* compile_basic_to_il(const char *basic_path) {
    fprintf(stderr, "[DEBUG] compile_basic_to_il: Starting...\n");
    /* Call embedded FasterBASIC compiler */
    char *qbe_il = compile_basic_to_qbe_string(basic_path);
    
    fprintf(stderr, "[DEBUG] compile_basic_to_il: Returned from wrapper\n");
    
    if (!qbe_il) {
        fprintf(stderr, "[DEBUG] compile_basic_to_il: qbe_il is NULL, returning NULL\n");
        return NULL;
    }
    
    fprintf(stderr, "[DEBUG] compile_basic_to_il: qbe_il is valid\n");
    size_t len = strlen(qbe_il);
    fprintf(stderr, "[DEBUG] compile_basic_to_il: len = %zu\n", len);
    
    /* Create FILE* from memory buffer using fmemopen */
    fprintf(stderr, "[DEBUG] compile_basic_to_il: About to call fmemopen...\n");
    FILE *mem_file = fmemopen(qbe_il, len, "r");
    fprintf(stderr, "[DEBUG] compile_basic_to_il: fmemopen returned\n");
    if (!mem_file) {
        fprintf(stderr, "[DEBUG] compile_basic_to_il: fmemopen failed, freeing and returning NULL\n");
        free(qbe_il);
        return NULL;
    }
    
    fprintf(stderr, "[DEBUG] compile_basic_to_il: Returning mem_file\n");
    /* Note: qbe_il memory is now owned by fmemopen */
    return mem_file;
}

/* Check if filename ends with .bas or .BAS */
int is_basic_file(const char *filename) {
    size_t len = strlen(filename);
    if (len < 4) return 0;
    
    const char *ext = filename + len - 4;
    return (strcmp(ext, ".bas") == 0 || strcmp(ext, ".BAS") == 0);
}

/* Enable CFG tracing in the compiler */
void set_trace_cfg(int enable) {
    set_trace_cfg_impl(enable);
}

/* Enable AST tracing in the compiler */
void set_trace_ast(int enable) {
    set_trace_ast_impl(enable);
}

}  // extern "C"
