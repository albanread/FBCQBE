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
#include "fasterbasic_qbe_codegen.h"

using namespace FasterBASIC;

extern "C" {

/* Compile BASIC source to QBE IL string
 * Returns: malloc'd string with QBE IL, or NULL on error
 */
char* compile_basic_to_qbe_string(const char *basic_path) {
    try {
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
        
        // Build CFG - use CFGBuilder
        CFGBuilder cfgBuilder;
        auto cfg = cfgBuilder.build(*ast, semantic.getSymbolTable());
        if (!cfg) {
            std::cerr << "CFG build failed\n";
            return nullptr;
        }
        
        // Generate QBE IL
        QBECodeGenerator qbeGen;
        qbeGen.setDataValues(dataResult);
        std::string qbeIL = qbeGen.generate(*cfg, semantic.getSymbolTable(), compilerOptions);
        
        // Debug: Dump QBE IL if DEBUG_IL environment variable is set
        if (getenv("DEBUG_IL")) {
            std::cerr << "\n=== QBE IL Output ===\n";
            std::cerr << qbeIL;
            std::cerr << "\n=== End QBE IL ===\n\n";
        }
        
        // Return as C string
        char *result = (char*)malloc(qbeIL.length() + 1);
        if (result) {
            strcpy(result, qbeIL.c_str());
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

} // extern "C"
