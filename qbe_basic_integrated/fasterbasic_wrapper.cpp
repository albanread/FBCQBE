/* FasterBASIC Compiler Integration
 * Runs the full FasterBASIC compilation pipeline and returns QBE IL
 */

#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <cstring>
#include <cstdlib>

#include "fasterbasic_lexer.h"
#include "fasterbasic_parser.h"
#include "fasterbasic_semantic.h"
#include "fasterbasic_cfg.h"
#include "fasterbasic_data_preprocessor.h"
// #include "fasterbasic_qbe_codegen.h"  // DISABLED - codegen needs to be adapted to new CFG
#include "fasterbasic_ast_dump.h"
#include "modular_commands.h"
#include "command_registry_core.h"

using namespace FasterBASIC;

// Global flags for trace options
static bool g_traceCFG = false;
static bool g_traceAST = false;

extern "C" {

/* Compile BASIC source to QBE IL string
 * Returns: malloc'd string with QBE IL, or NULL on error
 */
char* compile_basic_to_qbe_string(const char *basic_path) {
    try {
        // Initialize command registry with core BASIC commands/functions
        static bool registryInitialized = false;
        if (!registryInitialized) {
            auto& registry = FasterBASIC::ModularCommands::getGlobalCommandRegistry();
            FasterBASIC::ModularCommands::CoreCommandRegistry::registerCoreCommands(registry);
            FasterBASIC::ModularCommands::CoreCommandRegistry::registerCoreFunctions(registry);
            FasterBASIC::ModularCommands::markGlobalRegistryInitialized();
            registryInitialized = true;
        }
        
        // Read source file
        std::ifstream file(basic_path);
        if (!file) {
            std::cerr << "Cannot open: " << basic_path << "\n";
            return nullptr;
        }
        std::string source((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
        file.close();
        
        // Preprocess DATA statements
        DataPreprocessor dataPreprocessor;
        DataPreprocessorResult dataResult = dataPreprocessor.process(source);
        source = dataResult.cleanedSource;  // Use cleaned source
        
        // Lexer
        Lexer lexer;
        lexer.tokenize(source);
        auto tokens = lexer.getTokens();
        
        // Parser
        SemanticAnalyzer semantic;
        semantic.ensureConstantsLoaded();
        
        Parser parser;
        parser.setConstantsManager(&semantic.getConstantsManager());
        auto ast = parser.parse(tokens, basic_path);
        
        if (!ast || parser.hasErrors()) {
            std::cerr << "Parse errors in: " << basic_path << "\n";
            const auto& errors = parser.getErrors();
            for (const auto& error : errors) {
                std::cerr << "  Line " << error.location.line << ": " << error.what() << "\n";
            }
            return nullptr;
        }
        
        // Semantic analysis
        const auto& compilerOptions = parser.getOptions();
        semantic.analyze(*ast, compilerOptions);
        
        if (semantic.hasErrors()) {
            std::cerr << "Semantic errors in: " << basic_path << "\n";
            const auto& errors = semantic.getErrors();
            for (const auto& error : errors) {
                std::cerr << "  " << error.toString() << "\n";
            }
            return nullptr;
        }
        
        // Debug: Dump AST if requested
        if (g_traceAST || getenv("TRACE_AST")) {
            dumpAST(*ast, std::cerr);
            return nullptr;  // Exit after dumping AST
        }
        
        // Debug: Dump symbol table if requested
        if (getenv("TRACE_SYMBOLS")) {
            std::cerr << "\n=== Symbol Table Dump ===\n";
            const auto& symbols = semantic.getSymbolTable();
            
            std::cerr << "\nArrays:\n";
            for (const auto& [name, arr] : symbols.arrays) {
                std::cerr << "  " << name << ": elementTypeDesc=" << arr.elementTypeDesc.toString()
                          << " legacyType=" << (int)arr.type 
                          << " dimensions=" << arr.dimensions.size() << "\n";
            }
            
            std::cerr << "\nVariables:\n";
            for (const auto& [name, var] : symbols.variables) {
                std::cerr << "  " << name << ": typeDesc=" << var.typeDesc.toString()
                          << " legacyType=" << (int)var.type << "\n";
            }
            
            std::cerr << "=== End Symbol Table ===\n\n";
        }
        
        // Build CFG using new single-pass recursive builder
        std::cerr << "[INFO] Building complete ProgramCFG (main + all SUBs/FUNCTIONs)...\n";
        CFGBuilder cfgBuilder;
        ProgramCFG* programCFG = cfgBuilder.buildProgramCFG(*ast);
        
        if (!programCFG) {
            std::cerr << "[ERROR] ProgramCFG build failed\n";
            return nullptr;
        }
        
        std::cerr << "[INFO] ProgramCFG build successful!\n";
        std::cerr << "[INFO] Main program CFG + " << programCFG->functionCFGs.size() 
                  << " function/subroutine CFGs\n";
        
        // Always dump the CFGs for verification using comprehensive report
        std::cerr << "\n╔══════════════════════════════════════════════════════════════════════════╗\n";
        std::cerr << "║                    PROGRAM CFG ANALYSIS REPORT                           ║\n";
        std::cerr << "╚══════════════════════════════════════════════════════════════════════════╝\n\n";
        
        std::cerr << "Total CFGs: " << (1 + programCFG->functionCFGs.size()) << "\n";
        std::cerr << "  - Main Program: 1\n";
        std::cerr << "  - Functions/Subs: " << programCFG->functionCFGs.size() << "\n\n";
        
        // Dump main CFG with comprehensive analysis
        CFGBuilder mainBuilder;
        mainBuilder.setCFGForDump(programCFG->mainCFG.get());
        mainBuilder.dumpCFG("Main Program");
        mainBuilder.setCFGForDump(nullptr); // Clear to prevent deletion
        
        // Dump function/SUB CFGs with comprehensive analysis
        for (const auto& [name, cfg] : programCFG->functionCFGs) {
            CFGBuilder funcBuilder;
            funcBuilder.setCFGForDump(cfg.get());
            funcBuilder.dumpCFG(name);
            funcBuilder.setCFGForDump(nullptr); // Clear to prevent deletion
        }
        
        delete programCFG;
        
        // CODE GENERATION DISABLED
        // The codegen layer needs to be updated to work with the new CFG structure
        // For now, we only verify CFG generation is correct
        std::cerr << "\n========================================\n";
        std::cerr << "CODE GENERATION: DISABLED\n";
        std::cerr << "========================================\n";
        std::cerr << "The new CFG builder generates correct control flow graphs.\n";
        std::cerr << "Code generation will be re-enabled after adapting the\n";
        std::cerr << "QBE code generator to work with the new CFG structure.\n";
        std::cerr << "\nTo test CFG generation:\n";
        std::cerr << "  ./qbe_basic -G program.bas\n";
        std::cerr << "\n========================================\n\n";
        
        // Return empty string to indicate CFG-only mode
        char *result = (char*)malloc(1);
        if (result) {
            result[0] = '\0';
        }
        return result;
        
    } catch (const std::exception& e) {
        std::cerr << "FasterBASIC error: " << e.what() << "\n";
        return nullptr;
    } catch (...) {
        std::cerr << "FasterBASIC unknown error\n";
        return nullptr;
    }
}

/* Enable/disable CFG tracing */
void set_trace_cfg_impl(int enable) {
    g_traceCFG = (enable != 0);
}

/* Enable/disable AST tracing */
void set_trace_ast_impl(int enable) {
    g_traceAST = (enable != 0);
}

} // extern "C"
