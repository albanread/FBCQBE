//
// fbc_qbe.cpp
// FasterBASIC QBE Compiler
// Compiles BASIC source code to native executables via QBE backend
//

#include "fasterbasic_lexer.h"
#include "fasterbasic_parser.h"
#include "fasterbasic_semantic.h"
#include "fasterbasic_cfg.h"
#include "fasterbasic_qbe_codegen.h"
#include "fasterbasic_data_preprocessor.h"
#include "modular_commands.h"
#include "command_registry_core.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstring>
#include <chrono>

using namespace FasterBASIC;
using namespace FasterBASIC::ModularCommands;

void initializeFBCCommandRegistry() {
    // Initialize global registry with core commands for compiler use
    CommandRegistry& registry = getGlobalCommandRegistry();
    
    // Add core BASIC commands and functions
    CoreCommandRegistry::registerCoreCommands(registry);
    CoreCommandRegistry::registerCoreFunctions(registry);
    
    // Mark registry as initialized to prevent clearing
    markGlobalRegistryInitialized();
}

void printUsage(const char* programName) {
    std::cerr << "FasterBASIC QBE Compiler - Compiles BASIC to native code\n\n";
    std::cerr << "Usage: " << programName << " [options] <input.bas>\n\n";
    std::cerr << "Options:\n";
    std::cerr << "  -o <file>      Output executable file (default: a.out)\n";
    std::cerr << "  --emit-ssa     Emit QBE IL (.ssa) file only and exit\n";
    std::cerr << "  --emit-asm     Emit assembly (.s) file only and exit\n";
    std::cerr << "  -v, --verbose  Verbose output (compilation stats)\n";
    std::cerr << "  -h, --help     Show this help message\n";
    std::cerr << "  --profile      Show detailed timing for each compilation phase\n";
    std::cerr << "\nTarget Options:\n";
    std::cerr << "  --target=<t>   Target architecture (default: auto-detect)\n";
    std::cerr << "                 amd64_apple, amd64_sysv, arm64_apple, arm64, rv64\n";
    std::cerr << "\nExamples:\n";
    std::cerr << "  " << programName << " program.bas              # Compile to a.out\n";
    std::cerr << "  " << programName << " -o myprogram prog.bas    # Compile to myprogram\n";
    std::cerr << "  " << programName << " --emit-ssa prog.bas      # Generate prog.ssa only\n";
    std::cerr << "  " << programName << " --profile prog.bas       # Show compilation phase timings\n";
}

int main(int argc, char** argv) {
    // Initialize modular commands registry
    initializeFBCCommandRegistry();
    
    std::string inputFile;
    std::string outputFile = "a.out";
    std::string targetArch;
    bool verbose = false;
    bool emitSSA = false;
    bool emitASM = false;
    bool showProfile = false;
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            printUsage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            verbose = true;
        } else if (strcmp(argv[i], "--emit-ssa") == 0) {
            emitSSA = true;
        } else if (strcmp(argv[i], "--emit-asm") == 0) {
            emitASM = true;
        } else if (strcmp(argv[i], "--profile") == 0) {
            showProfile = true;
            verbose = true;  // Auto-enable verbose for profiling
        } else if (strcmp(argv[i], "-o") == 0) {
            if (i + 1 < argc) {
                outputFile = argv[++i];
            } else {
                std::cerr << "Error: -o requires an output filename\n";
                return 1;
            }
        } else if (strncmp(argv[i], "--target=", 9) == 0) {
            targetArch = argv[i] + 9;
        } else if (argv[i][0] == '-') {
            std::cerr << "Error: Unknown option: " << argv[i] << "\n";
            printUsage(argv[0]);
            return 1;
        } else {
            if (inputFile.empty()) {
                inputFile = argv[i];
            } else {
                std::cerr << "Error: Multiple input files specified\n";
                return 1;
            }
        }
    }
    
    if (inputFile.empty()) {
        std::cerr << "Error: No input file specified\n\n";
        printUsage(argv[0]);
        return 1;
    }
    
    try {
        auto compileStartTime = std::chrono::high_resolution_clock::now();
        auto phaseStartTime = compileStartTime;
        
        // Read source file
        if (verbose) {
            std::cerr << "Reading: " << inputFile << "\n";
        }
        
        std::ifstream file(inputFile);
        if (!file.is_open()) {
            std::cerr << "Error: Cannot open file: " << inputFile << "\n";
            return 1;
        }
        
        std::string source((std::istreambuf_iterator<char>(file)), 
                          std::istreambuf_iterator<char>());
        file.close();
        
        if (verbose) {
            std::cerr << "Source size: " << source.length() << " bytes\n";
        }
        
        auto readEndTime = std::chrono::high_resolution_clock::now();
        double readMs = std::chrono::duration<double, std::milli>(readEndTime - phaseStartTime).count();
        
        // Data preprocessing
        phaseStartTime = std::chrono::high_resolution_clock::now();
        if (verbose) {
            std::cerr << "Preprocessing DATA statements...\n";
        }
        
        DataPreprocessor dataPreprocessor;
        DataPreprocessorResult dataResult = dataPreprocessor.process(source);
        source = dataResult.cleanedSource;
        
        if (verbose && !dataResult.values.empty()) {
            std::cerr << "DATA values extracted: " << dataResult.values.size() << "\n";
        }
        
        auto dataEndTime = std::chrono::high_resolution_clock::now();
        double dataMs = std::chrono::duration<double, std::milli>(dataEndTime - phaseStartTime).count();
        
        // Lexical analysis
        phaseStartTime = std::chrono::high_resolution_clock::now();
        if (verbose) {
            std::cerr << "Lexing...\n";
        }
        
        Lexer lexer;
        lexer.tokenize(source);
        auto tokens = lexer.getTokens();
        
        auto lexEndTime = std::chrono::high_resolution_clock::now();
        double lexMs = std::chrono::duration<double, std::milli>(lexEndTime - phaseStartTime).count();
        
        if (verbose) {
            std::cerr << "Tokens: " << tokens.size() << "\n";
        }
        
        // Parsing
        phaseStartTime = std::chrono::high_resolution_clock::now();
        if (verbose) {
            std::cerr << "Parsing...\n";
        }
        
        // Create semantic analyzer early to get ConstantsManager
        SemanticAnalyzer semantic;
        
        // Ensure constants are loaded before parsing (for fast constant lookup)
        semantic.ensureConstantsLoaded();
        
        Parser parser;
        parser.setConstantsManager(&semantic.getConstantsManager());
        auto ast = parser.parse(tokens, inputFile);
        
        auto parseEndTime = std::chrono::high_resolution_clock::now();
        double parseMs = std::chrono::duration<double, std::milli>(parseEndTime - phaseStartTime).count();
        
        // Check for parser errors - if parsing failed, don't continue
        if (!ast || parser.hasErrors()) {
            std::cerr << "\nParsing failed with errors:\n";
            for (const auto& error : parser.getErrors()) {
                std::cerr << "  " << error.toString() << "\n";
            }
            std::cerr << "Compilation aborted.\n";
            return 1;
        }
        
        // Get compiler options from OPTION statements (collected during parsing)
        const auto& compilerOptions = parser.getOptions();
        
        if (verbose) {
            std::cerr << "Program lines: " << ast->lines.size() << "\n";
            std::cerr << "Compiler options: arrayBase=" << compilerOptions.arrayBase 
                      << " unicodeMode=" << compilerOptions.unicodeMode << "\n";
        }
        
        // Semantic analysis
        phaseStartTime = std::chrono::high_resolution_clock::now();
        if (verbose) {
            std::cerr << "Semantic analysis...\n";
        }
        
        semantic.analyze(*ast, compilerOptions);
        
        auto semanticEndTime = std::chrono::high_resolution_clock::now();
        double semanticMs = std::chrono::duration<double, std::milli>(semanticEndTime - phaseStartTime).count();
        
        if (verbose) {
            const auto& symTable = semantic.getSymbolTable();
            size_t varCount = symTable.variables.size();
            size_t funcCount = symTable.functions.size();
            size_t labelCount = symTable.lineNumbers.size();
            std::cerr << "Symbols: " << varCount << " variables, " 
                     << funcCount << " functions, " << labelCount << " labels\n";
        }
        
        // Control flow graph
        phaseStartTime = std::chrono::high_resolution_clock::now();
        if (verbose) {
            std::cerr << "Building CFG...\n";
        }
        
        CFGBuilder cfgBuilder;
        auto cfg = cfgBuilder.build(*ast, semantic.getSymbolTable());
        
        auto cfgEndTime = std::chrono::high_resolution_clock::now();
        double cfgMs = std::chrono::duration<double, std::milli>(cfgEndTime - phaseStartTime).count();
        
        if (verbose) {
            std::cerr << "CFG blocks: " << cfg->getBlockCount() << "\n";
        }
        
        // QBE code generation
        phaseStartTime = std::chrono::high_resolution_clock::now();
        if (verbose) {
            std::cerr << "Generating QBE IL...\n";
        }
        
        QBECodeGenerator qbeGen;
        qbeGen.setDataValues(dataResult);  // Pass DATA values to code generator
        std::string qbeIL = qbeGen.generate(*cfg, semantic.getSymbolTable(), compilerOptions);
        
        auto qbeGenEndTime = std::chrono::high_resolution_clock::now();
        double qbeGenMs = std::chrono::duration<double, std::milli>(qbeGenEndTime - phaseStartTime).count();
        
        if (verbose) {
            std::cerr << "Generated QBE IL size: " << qbeIL.length() << " bytes\n";
        }
        
        auto compileEndTime = std::chrono::high_resolution_clock::now();
        double totalCompileMs = std::chrono::duration<double, std::milli>(compileEndTime - compileStartTime).count();
        
        // Show detailed profiling if requested
        if (showProfile) {
            std::cerr << "\n=== Compilation Phase Timing ===\n";
            std::cerr << "  File I/O:          " << std::fixed << std::setprecision(3) << readMs << " ms\n";
            std::cerr << "  Data Preprocess:   " << std::fixed << std::setprecision(3) << dataMs << " ms\n";
            std::cerr << "  Lexer:             " << std::fixed << std::setprecision(3) << lexMs << " ms\n";
            std::cerr << "  Parser:            " << std::fixed << std::setprecision(3) << parseMs << " ms\n";
            std::cerr << "  Semantic:          " << std::fixed << std::setprecision(3) << semanticMs << " ms\n";
            std::cerr << "  CFG Builder:       " << std::fixed << std::setprecision(3) << cfgMs << " ms\n";
            std::cerr << "  QBE CodeGen:       " << std::fixed << std::setprecision(3) << qbeGenMs << " ms\n";
            std::cerr << "  --------------------------------\n";
            std::cerr << "  Total Compile:     " << std::fixed << std::setprecision(3) << totalCompileMs << " ms\n";
            std::cerr << "\n";
        }
        
        // Output QBE IL if requested
        if (emitSSA || !outputFile.empty()) {
            std::string ssaFile = emitSSA ? 
                (inputFile.substr(0, inputFile.find_last_of('.')) + ".ssa") : 
                outputFile;
            
            if (verbose) {
                std::cerr << "Writing QBE IL to: " << ssaFile << "\n";
            }
            
            std::ofstream outFile(ssaFile);
            if (!outFile) {
                std::cerr << "Error: Cannot write to file: " << ssaFile << "\n";
                return 1;
            }
            
            // Emit string literals first
            for (const auto& strData : qbeGen.getStats().instructionsGenerated > 0 ? 
                 std::vector<std::string>{} : std::vector<std::string>{}) {
                outFile << strData << "\n";
            }
            
            outFile << qbeIL;
            outFile.close();
            
            if (verbose) {
                std::cerr << "\n=== Compilation Status ===\n";
                std::cerr << "✓ Lexical analysis complete\n";
                std::cerr << "✓ Parsing complete\n";
                std::cerr << "✓ Semantic analysis complete\n";
                std::cerr << "✓ Control flow graph built\n";
                std::cerr << "✓ QBE IL generated\n";
                std::cerr << "\nQBE IL written to: " << ssaFile << "\n";
            }
            
            if (emitSSA) {
                return 0;  // Stop here if only emitting SSA
            }
        }
        
        if (verbose && !emitSSA) {
            std::cerr << "\n=== Compilation Status ===\n";
            std::cerr << "✓ Lexical analysis complete\n";
            std::cerr << "✓ Parsing complete\n";
            std::cerr << "✓ Semantic analysis complete\n";
            std::cerr << "✓ Control flow graph built\n";
            std::cerr << "✓ QBE IL generated\n";
            std::cerr << "\nNext: TODO - Invoke QBE compiler and assembler\n";
        }
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Compilation error: " << e.what() << "\n";
        return 1;
    }
}