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
#include "fasterbasic_ast_dump.h"
#include "modular_commands.h"
#include "command_registry_core.h"

using namespace FasterBASIC;

// Global flags for trace options
static bool g_traceCFG = false;
static bool g_traceAST = false;

// Function to visualize CFG structure
static void dumpCFG(const ProgramCFG& cfg) {
    std::cerr << "\n";
    std::cerr << "========================================\n";
    std::cerr << "           CFG TRACE OUTPUT             \n";
    std::cerr << "========================================\n\n";
    
    // Helper to get statement type name
    auto getStmtTypeName = [](const Statement* stmt) -> std::string {
        if (!stmt) return "NULL";
        switch (stmt->getType()) {
            case ASTNodeType::STMT_PRINT: return "PRINT";
            case ASTNodeType::STMT_LET: return "LET/ASSIGNMENT";
            case ASTNodeType::STMT_IF: return "IF";
            case ASTNodeType::STMT_WHILE: return "WHILE";
            case ASTNodeType::STMT_WEND: return "WEND";
            case ASTNodeType::STMT_FOR: return "FOR";
            case ASTNodeType::STMT_NEXT: return "NEXT";
            case ASTNodeType::STMT_DO: return "DO";
            case ASTNodeType::STMT_LOOP: return "LOOP";
            case ASTNodeType::STMT_GOTO: return "GOTO";
            case ASTNodeType::STMT_GOSUB: return "GOSUB";
            case ASTNodeType::STMT_RETURN: return "RETURN";
            case ASTNodeType::STMT_DIM: return "DIM";
            case ASTNodeType::STMT_REDIM: return "REDIM";
            case ASTNodeType::STMT_INPUT: return "INPUT";
            case ASTNodeType::STMT_EXIT: return "EXIT";
            default: return "OTHER";
        }
    };
    
    // Dump main CFG
    if (cfg.mainCFG) {
        std::cerr << "MAIN PROGRAM CFG:\n";
        std::cerr << "  Total blocks: " << cfg.mainCFG->blocks.size() << "\n\n";
        
        int stmtGlobalIndex = 0;
        
        for (size_t i = 0; i < cfg.mainCFG->blocks.size(); i++) {
            const BasicBlock* block = cfg.mainCFG->blocks[i].get();
            
            std::cerr << "--------------------\n";
            std::cerr << "Block " << block->id;
            if (!block->label.empty()) {
                std::cerr << " (" << block->label << ")";
            }
            if (block->isLoopHeader) {
                std::cerr << " [LOOP HEADER]";
            }
            if (block->isLoopExit) {
                std::cerr << " [LOOP EXIT]";
            }
            std::cerr << "\n";
            
            // Show line numbers
            if (!block->lineNumbers.empty()) {
                std::cerr << "  Lines: ";
                bool first = true;
                for (int lineNum : block->lineNumbers) {
                    if (!first) std::cerr << ", ";
                    std::cerr << lineNum;
                    first = false;
                }
                std::cerr << "\n";
            }
            
            // Show statements with global index
            if (!block->statements.empty()) {
                std::cerr << "  Statements:\n";
                for (const Statement* stmt : block->statements) {
                    std::cerr << "    [" << stmtGlobalIndex++ << "] ";
                    std::cerr << getStmtTypeName(stmt);
                    
                    // Show line number for this statement if available
                    int lineNum = block->getLineNumber(stmt);
                    if (lineNum > 0) {
                        std::cerr << " (line " << lineNum << ")";
                    }
                    
                    // Special info for specific statement types
                    if (stmt->getType() == ASTNodeType::STMT_WHILE) {
                        std::cerr << " - creates loop";
                    } else if (stmt->getType() == ASTNodeType::STMT_WEND) {
                        std::cerr << " - ends loop";
                    } else if (stmt->getType() == ASTNodeType::STMT_IF) {
                        const auto* ifStmt = static_cast<const IfStatement*>(stmt);
                        std::cerr << " - then:" << ifStmt->thenStatements.size() 
                                  << " else:" << ifStmt->elseStatements.size();
                    }
                    
                    std::cerr << "\n";
                }
            } else {
                std::cerr << "  Statements: (none)\n";
            }
            
            // Show successors (edges)
            if (!block->successors.empty()) {
                std::cerr << "  Successors: ";
                for (size_t j = 0; j < block->successors.size(); j++) {
                    if (j > 0) std::cerr << ", ";
                    std::cerr << block->successors[j];
                }
                std::cerr << "\n";
            } else {
                std::cerr << "  Successors: (none - exit)\n";
            }
            
            std::cerr << "\n";
        }
        
        // Dump loop structure info
        if (!cfg.mainCFG->whileLoopHeaders.empty()) {
            std::cerr << "--------------------\n";
            std::cerr << "WHILE Loop Headers:\n";
            for (const auto& [blockId, headerId] : cfg.mainCFG->whileLoopHeaders) {
                std::cerr << "  Block " << blockId << " -> header " << headerId << "\n";
            }
            std::cerr << "\n";
        }
        
        if (!cfg.mainCFG->forLoopStructure.empty()) {
            std::cerr << "--------------------\n";
            std::cerr << "FOR Loop Structures:\n";
            for (const auto& [blockId, forLoop] : cfg.mainCFG->forLoopStructure) {
                std::cerr << "  Block " << blockId << ": init=" << forLoop.initBlock
                          << " check=" << forLoop.checkBlock 
                          << " body=" << forLoop.bodyBlock
                          << " exit=" << forLoop.exitBlock
                          << " var=" << forLoop.variable << "\n";
            }
            std::cerr << "\n";
        }
    }
    
    // Dump function CFGs
    if (!cfg.functionCFGs.empty()) {
        std::cerr << "--------------------\n";
        std::cerr << "FUNCTION CFGs: " << cfg.functionCFGs.size() << " functions\n";
        for (const auto& [name, funcCFG] : cfg.functionCFGs) {
            std::cerr << "\nFunction: " << name << "\n";
            std::cerr << "  Blocks: " << funcCFG->blocks.size() << "\n";
        }
    }
    
    std::cerr << "\n========================================\n";
    std::cerr << "         END CFG TRACE OUTPUT           \n";
    std::cerr << "========================================\n\n";
}

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
        
        // Build CFG - use CFGBuilder
        CFGBuilder cfgBuilder;
        auto cfg = cfgBuilder.build(*ast, semantic.getSymbolTable());
        if (!cfg) {
            std::cerr << "CFG build failed\n";
            return nullptr;
        }
        
        // Dump CFG if --trace-cfg was specified
        if (g_traceCFG) {
            dumpCFG(*cfg);
            // Return empty string to signal successful CFG trace
            char *result = (char*)malloc(1);
            if (result) {
                result[0] = '\0';
            }
            return result;
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

/* Enable/disable CFG tracing */
void set_trace_cfg_impl(int enable) {
    g_traceCFG = (enable != 0);
}

/* Enable/disable AST tracing */
void set_trace_ast_impl(int enable) {
    g_traceAST = (enable != 0);
}

} // extern "C"
