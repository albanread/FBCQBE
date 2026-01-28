//
// qbe_codegen_main.cpp
// FasterBASIC QBE Code Generator - Main Orchestration
//
// This file contains the main code generation orchestration:
// - generate() entry point
// - emitHeader(), emitDataSection(), emitMainFunction()
// - emitBlock() - basic block emission
//

#include "../fasterbasic_qbe_codegen.h"
#include <iostream>
#include <iomanip>
#include <ctime>

namespace FasterBASIC {

// =============================================================================
// Constructor / Destructor
// =============================================================================

QBECodeGenerator::QBECodeGenerator()
    : m_programCFG(nullptr)
    , m_cfg(nullptr)
    , m_symbols(nullptr)
{
}

QBECodeGenerator::QBECodeGenerator(const QBECodeGenConfig& config)
    : m_config(config)
    , m_programCFG(nullptr)
    , m_cfg(nullptr)
    , m_symbols(nullptr)
{
}

QBECodeGenerator::~QBECodeGenerator() {
}

// =============================================================================
// Statistics
// =============================================================================

void QBECodeGenStats::print() const {
    std::cout << "=== QBE Code Generation Statistics ===\n";
    std::cout << "Instructions generated: " << instructionsGenerated << "\n";
    std::cout << "Labels generated: " << labelsGenerated << "\n";
    std::cout << "Variables used: " << variablesUsed << "\n";
    std::cout << "Arrays used: " << arraysUsed << "\n";
    std::cout << "Functions generated: " << functionsGenerated << "\n";
    std::cout << "Generation time: " << std::fixed << std::setprecision(2) 
              << generationTimeMs << " ms\n";
}

// =============================================================================
// Main Entry Point
// =============================================================================

std::string QBECodeGenerator::generate(const ProgramCFG& programCFG,
                                       const SymbolTable& symbols,
                                       const CompilerOptions& options) {
    clock_t startTime = clock();
    
    m_programCFG = &programCFG;
    m_symbols = &symbols;
    m_options = options;
    
    // Populate variable types
    for (const auto& var : symbols.variables) {
        std::string lower = var.first;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        if (var.second.type == VariableType::DOUBLE) {
            m_varTypes[lower] = "d";
        } else {
            m_varTypes[lower] = "w";
        }
    }
    
    // Reset state
    m_output.str("");
    m_output.clear();
    m_variables.clear();
    m_varTypes.clear();
    m_arrays.clear();
    m_labels.clear();
    m_stringLiterals.clear();
    m_dataStrings.clear();
    m_loopStack.clear();
    m_gosubReturnLabels.clear();
    m_tempCounter = 0;
    m_labelCounter = 0;
    m_stringCounter = 0;
    m_inFunction = false;
    
    m_stats = QBECodeGenStats();
    
    // Generate QBE IL sections
    emitHeader();
    
    // Pre-pass: collect FOR loop variables to determine correct types
    collectForLoopVariables();
    
    // Emit main function
    m_cfg = m_programCFG->mainCFG.get();
    emitMainFunction();
    
    // Emit user-defined functions
    for (const auto& funcName : m_programCFG->getFunctionNames()) {
        m_cfg = m_programCFG->getFunctionCFG(funcName);
        emitFunction(funcName);
    }
    
    emitDataSection();  // Emit after all functions so strings are collected
    
    clock_t endTime = clock();
    m_stats.generationTimeMs = (double)(endTime - startTime) / CLOCKS_PER_SEC * 1000.0;
    
    return m_output.str();
}
// =============================================================================
// Pre-pass: Collect FOR Loop Variables
// =============================================================================

void QBECodeGenerator::collectForLoopVariables() {
    m_forLoopVariables.clear();
    
    // Traverse all CFGs (main + functions) to find FOR statements
    std::vector<const ControlFlowGraph*> cfgs = {m_programCFG->mainCFG.get()};
    for (const auto& funcName : m_programCFG->getFunctionNames()) {
        cfgs.push_back(m_programCFG->getFunctionCFG(funcName));
    }
    
    for (const ControlFlowGraph* cfg : cfgs) {
        if (!cfg) continue;
        
        for (int i = 0; i < cfg->getBlockCount(); ++i) {
            const BasicBlock* block = cfg->getBlock(i);
            if (!block) continue;
            
            // Check each statement in the block
            for (const Statement* stmt : block->statements) {
                if (stmt->getType() == ASTNodeType::STMT_FOR) {
                    const ForStatement* forStmt = static_cast<const ForStatement*>(stmt);
                    // Strip suffix from variable name (i% -> i, j$ -> j, etc.)
                    std::string varName = forStmt->variable;
                    size_t pos = varName.find_last_of("%$#!&@^");
                    if (pos != std::string::npos && pos == varName.length() - 1) {
                        varName = varName.substr(0, pos);
                    }
                    m_forLoopVariables.insert(varName);
                }
            }
        }
    }
}
// =============================================================================
// Header Emission
// =============================================================================

void QBECodeGenerator::emitHeader() {
    emitComment("Generated by FasterBASIC QBE Code Generator");
    emitComment("Target: QBE IL (SSA format)");
    emitComment("");
    emitComment("External runtime functions from libbasic_runtime.a");
    emit("\n");
}

// =============================================================================
// Data Section Emission
// =============================================================================

void QBECodeGenerator::setDataValues(const DataPreprocessorResult& dataResult) {
    m_dataValues = dataResult.values;
    m_lineRestorePoints = dataResult.lineRestorePoints;
    m_labelRestorePoints = dataResult.labelRestorePoints;
}

void QBECodeGenerator::emitDataSection() {
    emitComment("=== DATA SECTION ===");
    emit("\n");
    
    // Emit DATA values if present
    if (!m_dataValues.empty()) {
        emitComment("DATA values");
        emit("export data $__basic_data = { ");
        
        for (size_t i = 0; i < m_dataValues.size(); ++i) {
            if (i > 0) emit(", ");
            
            const DataValue& val = m_dataValues[i];
            if (std::holds_alternative<int>(val)) {
                // Use 'l' (long/64-bit) for integers to match runtime int64_t
                emit("l " + std::to_string(std::get<int>(val)));
            } else if (std::holds_alternative<double>(val)) {
                // Use 'd' (double/64-bit) for doubles
                emit("d " + std::to_string(std::get<double>(val)));
            } else {
                // String pointer - add to string pool and reference it
                const std::string& str = std::get<std::string>(val);
                size_t strIdx = m_dataStrings.size();
                m_dataStrings.push_back(str);
                emit("l $data_str." + std::to_string(strIdx));
            }
        }
        emit(" }\n");
        
        // Emit type tags (0=INT, 1=DOUBLE, 2=STRING)
        emit("export data $__basic_data_types = { ");
        for (size_t i = 0; i < m_dataValues.size(); ++i) {
            if (i > 0) emit(", ");
            
            const DataValue& val = m_dataValues[i];
            if (std::holds_alternative<int>(val)) {
                emit("b 0");
            } else if (std::holds_alternative<double>(val)) {
                emit("b 1");
            } else {
                emit("b 2");
            }
        }
        emit(" }\n");
        
        // Emit data pointer (initialized to 0)
        emit("export data $__basic_data_ptr = { l 0 }\n");
        emit("\n");
    } else {
        // Emit empty DATA symbols for programs without DATA statements
        emitComment("No DATA statements - empty symbols");
        emit("export data $__basic_data = { l 0 }\n");
        emit("export data $__basic_data_types = { b 0 }\n");
        emit("export data $__basic_data_ptr = { l 0 }\n");
        emit("\n");
    }
    
    // Emit GOSUB return stack
    emitComment("GOSUB return stack");
    emit("export data $return_stack = { w 0, w 0, w 0, w 0, w 0, w 0, w 0, w 0, w 0, w 0, w 0, w 0, w 0, w 0, w 0, w 0 }\n");
    emit("export data $return_sp = { w 0 }\n");
    emit("\n");
    
    // Emit string literals as data objects
    if (!m_dataStrings.empty()) {
        emitComment("String literals");
        for (size_t i = 0; i < m_dataStrings.size(); ++i) {
            emit("data $data_str." + std::to_string(i) + " = { ");
            emit("b \"" + escapeString(m_dataStrings[i]) + "\", b 0 }\n");
        }
    }
    
    emit("\n");
}

// =============================================================================
// Main Function Emission (CFG-driven)
// =============================================================================

void QBECodeGenerator::emitMainFunction() {
    emitComment("=== MAIN FUNCTION ===");
    emit("\n");
    
    // Function signature
    emit("export function w $main() {\n");
    m_stats.functionsGenerated++;
    
    // Entry block (always @start)
    emit("@start\n");
    m_stats.labelsGenerated++;
    
    emitComment("Initialize runtime");
    emit("    call $basic_runtime_init()\n");
    m_stats.instructionsGenerated++;
    emit("\n");
    
    // Declare all variables upfront (QBE requires this)
    emitComment("Variable declarations");
    
    // Collect all variables from symbol table
    // Note: Variable names in the symbol table are already mangled with type suffixes
    // e.g., "X_INT", "Y_DOUBLE", "S_STRING" (done by semantic analyzer)
    if (m_symbols) {
        for (const auto& [name, varSym] : m_symbols->variables) {
            // Skip FOR EACH variables - they're not real variables
            if (name == "n") {
                continue;
            }
            
            // All INT variables are 64-bit longs
            VariableType effectiveType = varSym.type;
            std::string qbeType = getQBEType(effectiveType);
            
            // Sanitize variable name - replace BASIC suffixes with underscores
            std::string safeName = name;
            for (size_t i = 0; i < safeName.length(); ++i) {
                char c = safeName[i];
                if (c == '%' || c == '$' || c == '#' || c == '!' || c == '&' || c == '@' || c == '^') {
                    safeName[i] = '_';
                }
            }
            
            // Create QBE SSA variable name: %var_<MANGLED_NAME>
            // Examples: %var_X_INT, %var_Y_DOUBLE, %var_S_STRING
            std::string varRef = "%var_" + safeName;
            m_variables[name] = m_variables.size();
            m_varTypes[name] = qbeType;
            
            // Initialize to zero
            if (effectiveType == VariableType::USER_DEFINED) {
                // User-defined type - allocate memory on stack
                size_t typeSize = calculateTypeSize(varSym.typeName);
                emit("    " + varRef + " =l alloc8 " + std::to_string(typeSize) + "\n");
                m_varTypes[name] = "l";  // UDTs are pointers
                m_varTypeNames[name] = varSym.typeName;  // Cache type name
            } else if (effectiveType == VariableType::STRING) {
                // Initialize string as empty StringDescriptor
                emit("    " + varRef + " =l call $string_new_capacity(l 0)\n");
            } else if (effectiveType == VariableType::INT) {
                // All integers are 64-bit longs
                emit("    " + varRef + " =l copy 0\n");
            } else if (effectiveType == VariableType::FLOAT) {
                // FLOAT maps to QBE 's' (32-bit single precision)
                emit("    " + varRef + " =s copy s_0.0\n");
            } else if (effectiveType == VariableType::DOUBLE) {
                // DOUBLE maps to QBE 'd' (64-bit double precision)
                emit("    " + varRef + " =d copy d_0.0\n");
            }
            m_stats.instructionsGenerated++;
            m_stats.variablesUsed++;
        }
        
        // Declare arrays (as descriptors - dope vectors)
        // Each array descriptor is 40 bytes (aligned):
        //   offset 0:  data pointer (8 bytes)
        //   offset 8:  lowerBound (8 bytes)
        //   offset 16: upperBound (8 bytes)
        //   offset 24: elementSize (8 bytes)
        //   offset 32: dimensions (4 bytes)
        //   offset 36: padding (4 bytes)
        for (const auto& [name, arraySym] : m_symbols->arrays) {
            std::string arrayRef = "%arr_" + name;
            m_arrays[name] = m_arrays.size();
            
            // Allocate descriptor on stack (40 bytes, 8-byte aligned)
            emit("    " + arrayRef + " =l alloc8 40\n");
            m_stats.instructionsGenerated++;
            
            // Initialize descriptor to null/zero state
            // Data pointer = NULL
            emit("    storel 0, " + arrayRef + "\n");
            
            // LowerBound = 0
            std::string lowerAddr = allocTemp("l");
            emit("    " + lowerAddr + " =l add " + arrayRef + ", 8\n");
            emit("    storel 0, " + lowerAddr + "\n");
            
            // UpperBound = -1 (indicates uninitialized/empty)
            std::string upperAddr = allocTemp("l");
            emit("    " + upperAddr + " =l add " + arrayRef + ", 16\n");
            emit("    storel -1, " + upperAddr + "\n");
            
            // ElementSize = 0 (will be set by DIM)
            std::string elemSizeAddr = allocTemp("l");
            emit("    " + elemSizeAddr + " =l add " + arrayRef + ", 24\n");
            emit("    storel 0, " + elemSizeAddr + "\n");
            
            // Dimensions = 0 (will be set by DIM)
            std::string dimsAddr = allocTemp("l");
            emit("    " + dimsAddr + " =l add " + arrayRef + ", 32\n");
            emit("    storew 0, " + dimsAddr + "\n");
            
            m_stats.instructionsGenerated += 10;
            m_stats.arraysUsed++;
            
            emitComment("Array descriptor " + name + " allocated (40 bytes)");
        }
    }
    emit("\n");
    
    // Emit all basic blocks from CFG
    emitComment("Program basic blocks (CFG-driven)");
    
    // Jump to entry block (skip @start)
    if (m_cfg && m_cfg->entryBlock >= 0 && m_cfg->entryBlock < m_cfg->getBlockCount()) {
        std::string entryLabel = getBlockLabel(m_cfg->entryBlock);
        emit("    jmp @" + entryLabel + "\n");
        m_stats.instructionsGenerated++;
    }
    emit("\n");
    
    // Emit each basic block
    if (m_cfg) {
        for (const auto& block : m_cfg->blocks) {
            emitBlock(block.get());
        }
    }
    
    // Exit block
    emit("@exit\n");
    m_stats.labelsGenerated++;
    emitComment("Cleanup and return");
    
    // Free all array data (from descriptors)
    if (m_symbols) {
        for (const auto& [name, arraySym] : m_symbols->arrays) {
            std::string arrayRef = "%arr_" + name;
            
            // Load data pointer from descriptor (offset 0)
            std::string dataPtr = allocTemp("l");
            emit("    " + dataPtr + " =l loadl " + arrayRef + "\n");
            
            // Free if not null
            emit("    call $free(l " + dataPtr + ")\n");
            
            m_stats.instructionsGenerated += 2;
        }
    }
    
    emit("    call $basic_runtime_cleanup()\n");
    emit("    ret 0\n");
    m_stats.instructionsGenerated += 2;
    
    emit("}\n");
}

// =============================================================================
// Function Context Management
// =============================================================================

void QBECodeGenerator::enterFunctionContext(const std::string& functionName) {
    m_inFunction = true;
    m_currentFunction = functionName;
    m_localVariables.clear();
    m_sharedVariables.clear();
    
    // Create function context for local array tracking
    // Determine if SUB or FUNCTION and return type
    bool isSub = true;  // Default to SUB
    VariableType returnType = VariableType::VOID;
    
    // Look up function in symbol table to get actual return type
    if (m_symbols && m_symbols->functions.find(functionName) != m_symbols->functions.end()) {
        const auto& funcSym = m_symbols->functions.at(functionName);
        returnType = funcSym.returnType;
        isSub = (returnType == VariableType::VOID);
    }
    
    FunctionContext ctx(functionName, returnType, isSub);
    ctx.tidyExitLabel = "tidy_exit_" + functionName;
    m_functionStack.push(ctx);
}

void QBECodeGenerator::exitFunctionContext() {
    // Pop function context (cleanup happens in emitFunctionEpilogue)
    if (!m_functionStack.empty()) {
        m_functionStack.pop();
    }
    
    m_inFunction = false;
    m_currentFunction = "";
    m_localVariables.clear();
    m_sharedVariables.clear();
}

// =============================================================================
// User-Defined Function Emission
// =============================================================================

void QBECodeGenerator::emitFunction(const std::string& functionName) {
    if (!m_cfg) return;
    
    enterFunctionContext(functionName);
    
    // Check if this is a DEF FN function (single-line expression function)
    bool isDefFn = (m_cfg->defStatement != nullptr);
    
    // Emit function header comment
    emitComment("=============================================================================");
    if (isDefFn) {
        emitComment("DEF FN: " + functionName);
    } else {
        emitComment("Function: " + functionName);
    }
    emitComment("=============================================================================");
    emit("\n");
    
    // Determine return type
    std::string qbeReturnType = "l";  // Default to long (64-bit INT)
    if (m_cfg->returnType == VariableType::DOUBLE || m_cfg->returnType == VariableType::FLOAT) {
        qbeReturnType = "d";  // Double
    } else if (m_cfg->returnType == VariableType::STRING) {
        qbeReturnType = "l";  // Pointer (long)
    } else if (m_cfg->returnType == VariableType::UNKNOWN) {
        qbeReturnType = "w";  // SUBs return void (we use 0)
    }
    
    // Build parameter list
    std::string paramList;
    for (size_t i = 0; i < m_cfg->parameters.size(); ++i) {
        if (i > 0) paramList += ", ";
        
        std::string paramType = "l";  // Default to long (64-bit INT)
        if (i < m_cfg->parameterTypes.size()) {
            VariableType vt = m_cfg->parameterTypes[i];
            if (vt == VariableType::DOUBLE || vt == VariableType::FLOAT) {
                paramType = "d";
            } else if (vt == VariableType::STRING) {
                paramType = "l";  // String pointer
            } else if (vt == VariableType::INT) {
                paramType = "l";  // INT is 64-bit long
            }
        }
        
        paramList += paramType + " %" + m_cfg->parameters[i];
    }
    
    // Emit function signature (function name already mangled by parser)
    emit("export function " + qbeReturnType + " $" + functionName + "(" + paramList + ") {\n");
    emit("@start\n");
    m_stats.labelsGenerated++;
    
    // Special handling for DEF FN - just evaluate expression and return
    if (isDefFn && m_cfg->defStatement && m_cfg->defStatement->body) {
        emitComment("DEF FN body - single expression");
        
        // DEF FN is just a named expression with parameters
        // Register ALL possible name variations for parameters so they can be found
        // Parameters from the DefStatement are the original names (e.g., "N%", "X$")
        m_defFnParams.clear();  // Clear any previous DEF FN parameters
        
        for (size_t i = 0; i < m_cfg->defStatement->parameters.size(); ++i) {
            std::string originalParam = m_cfg->defStatement->parameters[i];
            std::string qbeParam = "%" + originalParam;
            
            // Strip the BASIC type suffix to get base name
            std::string baseName = originalParam;
            if (!baseName.empty()) {
                char lastChar = baseName.back();
                if (lastChar == '%' || lastChar == '$' || lastChar == '#' || lastChar == '!') {
                    baseName.pop_back();
                }
            }
            
            // Register ALL possible lookup variations in the DEF FN params map:
            // 1. Original name with suffix (e.g., "N%")
            m_defFnParams[originalParam] = qbeParam;
            
            // 2. Base name without suffix (e.g., "N")
            if (baseName != originalParam) {
                m_defFnParams[baseName] = qbeParam;
            }
            
            // 3. Normalized name with type suffix (e.g., "N_INT", "N_DOUBLE")
            // The semantic analyzer might normalize variable names in expressions
            if (i < m_cfg->parameterTypes.size()) {
                VariableType paramType = m_cfg->parameterTypes[i];
                std::string normalizedSuffix = "";
                switch (paramType) {
                    case VariableType::INT: normalizedSuffix = "_INT"; break;
                    case VariableType::DOUBLE: normalizedSuffix = "_DOUBLE"; break;
                    case VariableType::FLOAT: normalizedSuffix = "_FLOAT"; break;
                    case VariableType::STRING: normalizedSuffix = "_STRING"; break;
                    default: normalizedSuffix = "_DOUBLE"; break;  // Default fallback
                }
                if (!normalizedSuffix.empty()) {
                    std::string normalizedName = baseName + normalizedSuffix;
                    m_defFnParams[normalizedName] = qbeParam;
                }
            }
            
            // 4. Sanitized version (type suffix chars replaced with _)
            std::string sanitized = originalParam;
            for (size_t j = 0; j < sanitized.length(); ++j) {
                char c = sanitized[j];
                if (c == '%' || c == '$' || c == '#' || c == '!') {
                    sanitized[j] = '_';
                }
            }
            if (sanitized != originalParam) {
                m_defFnParams[sanitized] = qbeParam;
            }
        }
        
        // Evaluate the expression - parameters will be resolved via m_localVariables
        std::string resultTemp = emitExpression(m_cfg->defStatement->body.get());
        
        // Return the result
        emit("    ret " + resultTemp + "\n");
        m_stats.instructionsGenerated++;
        emit("}\n");
        emit("\n");
        
        exitFunctionContext();
        return;
    }
    
    // Regular FUNCTION/SUB handling
    // Initialize function return variable (for FUNCTIONs)
    if (m_cfg->returnType != VariableType::UNKNOWN) {
        std::string returnVar = "%var_" + functionName;
        std::string initValue = "0";
        if (m_cfg->returnType == VariableType::DOUBLE || m_cfg->returnType == VariableType::FLOAT) {
            emit("    " + returnVar + " =d copy d_0.0\n");
        } else if (m_cfg->returnType == VariableType::STRING) {
            // Initialize to empty string via runtime
            emit("    " + returnVar + " =l call $basic_empty_string()\n");
        } else {
            // INT type - now 64-bit long
            emit("    " + returnVar + " =l copy " + initValue + "\n");
        }
        m_stats.instructionsGenerated++;
    }
    
    emitComment("Function body");
    
    // Jump to entry block
    if (m_cfg->entryBlock >= 0 && m_cfg->entryBlock < m_cfg->getBlockCount()) {
        std::string entryLabel = getBlockLabel(m_cfg->entryBlock);
        emit("    jmp @" + entryLabel + "\n");
        m_stats.instructionsGenerated++;
    }
    emit("\n");
    
    // Emit each basic block
    for (const auto& block : m_cfg->blocks) {
        emitBlock(block.get());
    }
    
    // Tidy exit block - cleanup local arrays, then fall through to exit
    if (!m_functionStack.empty()) {
        emit("@" + m_functionStack.top().tidyExitLabel + "\n");
        m_stats.labelsGenerated++;
        emitComment("Cleanup local arrays");
        
        // Free all local heap-allocated arrays
        for (const auto& arrayName : m_functionStack.top().localArrays) {
            std::string arrayRef = "%arr_" + arrayName;
            emit("    call $free(l " + arrayRef + ")\n");
            m_stats.instructionsGenerated++;
        }
        
        if (!m_functionStack.top().localArrays.empty()) {
            emitComment("Fall through to exit");
        }
    }
    
    // Exit block - return the function-name variable (or 0 for SUBs)
    emit("@exit\n");
    m_stats.labelsGenerated++;
    emitComment("Return from function");
    
    if (m_cfg->returnType != VariableType::UNKNOWN) {
        std::string returnVar = "%var_" + functionName;
        if (m_cfg->returnType == VariableType::DOUBLE || m_cfg->returnType == VariableType::FLOAT) {
            emit("    %retval =d copy " + returnVar + "\n");
            emit("    ret %retval\n");
        } else if (m_cfg->returnType == VariableType::STRING) {
            emit("    %retval =l copy " + returnVar + "\n");
            emit("    ret %retval\n");
        } else {
            // INT type - now 64-bit long
            emit("    %retval =l copy " + returnVar + "\n");
            emit("    ret %retval\n");
        }
    } else {
        // SUB returns 0
        emit("    ret 0\n");
    }
    m_stats.instructionsGenerated += 2;
    
    emit("}\n");
    emit("\n");
    
    exitFunctionContext();
}

// =============================================================================
// Basic Block Emission
// =============================================================================

std::string QBECodeGenerator::getComparisonOp(TokenType op) {
    // Integer comparisons use 64-bit long comparisons (cs*l)
    switch (op) {
        case TokenType::EQUAL: return "ceql";
        case TokenType::NOT_EQUAL: return "cnel";
        case TokenType::LESS_THAN: return "csltl";
        case TokenType::LESS_EQUAL: return "cslel";
        case TokenType::GREATER_THAN: return "csgtl";
        case TokenType::GREATER_EQUAL: return "csgel";
        default: return "ceql"; // Default to equal
    }
}

std::string QBECodeGenerator::getComparisonOpDouble(TokenType op) {
    // QBE floating point comparisons use c*d (no 's' prefix)
    // Integer comparisons use cs*w (with 's' for signed)
    switch (op) {
        case TokenType::EQUAL: return "ceqd";
        case TokenType::NOT_EQUAL: return "cned";
        case TokenType::LESS_THAN: return "cltd";
        case TokenType::LESS_EQUAL: return "cled";
        case TokenType::GREATER_THAN: return "cgtd";
        case TokenType::GREATER_EQUAL: return "cged";
        default: return "ceqd"; // Default to equal
    }
}

void QBECodeGenerator::emitBlock(const BasicBlock* block) {
    if (!block) return;
    
    // Store current block for statement handlers
    m_currentBlock = block;
    m_lastStatementWasTerminator = false;
    
    // Emit block label
    std::string label = getBlockLabel(block->id);
    emit("@" + label + "\n");
    m_stats.labelsGenerated++;
    
    // Add debug comment with block info
    if (m_config.emitComments) {
        std::string comment = "Block " + std::to_string(block->id);
        if (!block->label.empty()) {
            comment += " (" + block->label + ")";
        }
        if (!block->lineNumbers.empty()) {
            comment += " [Lines: ";
            bool first = true;
            for (int line : block->lineNumbers) {
                if (!first) comment += ", ";
                comment += std::to_string(line);
                first = false;
            }
            comment += "]";
        }
        emitComment(comment);
    }
    
    // Check if this is a FOR loop check block (empty block that needs condition check)
    if (block->statements.empty() && !block->label.empty() && 
        block->label.find("FOR Loop Check") != std::string::npos) {
        
        // Find the FOR loop structure for this check block
        for (const auto& pair : m_cfg->forLoopStructure) {
            if (pair.second.checkBlock == block->id) {
                const auto& forBlocks = pair.second;
                std::string varName = forBlocks.variable;
                
                // Emit loop condition check with STEP sign awareness
                std::string varRef = getVariableRef(varName);
                // Sanitize variable name for QBE (replace %, $, #, !, &, @, ^ with _)
                std::string safeVarName = varName;
                for (size_t i = 0; i < safeVarName.length(); ++i) {
                    char c = safeVarName[i];
                    if (c == '%' || c == '$' || c == '#' || c == '!' || c == '&' || c == '@' || c == '^') {
                        safeVarName[i] = '_';
                    }
                }
                std::string endVar = "%end_" + safeVarName;
                std::string stepVar = "%step_" + safeVarName;

                // Determine if step is negative: isNeg = (step &lt; 0)
                std::string isNeg = allocTemp("w");
                emit("    " + isNeg + " =w csltl " + stepVar + ", 0\n");
                m_stats.instructionsGenerated++;

                // condPos: var &lt;= end (for non-negative step)
                std::string condPos = allocTemp("w");
                emit("    " + condPos + " =w cslel " + varRef + ", " + endVar + "\n");
                m_stats.instructionsGenerated++;

                // condNeg: var >= end (for negative step)
                std::string condNeg = allocTemp("w");
                emit("    " + condNeg + " =w csgel " + varRef + ", " + endVar + "\n");
                m_stats.instructionsGenerated++;

                // Select correct condition based on step sign
                std::string notNeg = allocTemp("w");
                emit("    " + notNeg + " =w ceqw " + isNeg + ", 0\n");
                m_stats.instructionsGenerated++;

                std::string condFromNeg = allocTemp("w");
                emit("    " + condFromNeg + " =w and " + isNeg + ", " + condNeg + "\n");
                m_stats.instructionsGenerated++;

                std::string condFromPos = allocTemp("w");
                emit("    " + condFromPos + " =w and " + notNeg + ", " + condPos + "\n");
                m_stats.instructionsGenerated++;

                std::string condTemp = allocTemp("w");
                emit("    " + condTemp + " =w or " + condFromNeg + ", " + condFromPos + "\n");
                m_stats.instructionsGenerated++;

                // Store condition for CFG to emit conditional branch
                m_lastCondition = condTemp;
                break;
            }
        }
    }
    
    // Check if this is a FOR EACH...IN loop header block (needs condition check)
    else if (block->statements.empty() && !block->label.empty() && 
        block->label.find("FOR...IN Loop Header") != std::string::npos) {
        
        emitComment("FOR EACH...IN loop condition check");
        
        // Find the loop variable from the loop context
        // We need to check: index < array_size
        std::string varName = "";
        
        // Try to find from loop stack or infer from next block
        if (!m_loopStack.empty()) {
            varName = m_loopStack.back().forVariable;
        }
        
        if (varName.empty()) {
            // Try to infer from next block's statements
            if (block->id + 1 < m_cfg->getBlockCount()) {
                const BasicBlock* nextBlock = m_cfg->getBlock(block->id + 1);
                if (nextBlock && !nextBlock->statements.empty()) {
                    // Look for FOR EACH statement in previous blocks or context
                    // For now, we'll need to track this better
                    emitComment("Warning: Could not determine FOR EACH variable");
                }
            }
        }
        
        if (!varName.empty()) {
            std::string safeVarName = sanitizeQBEVariableName(varName);
            std::string indexVar = "%foreach_idx_" + safeVarName;
            std::string sizeVar = "%foreach_size_" + safeVarName;
            
            // Emit condition: index < size
            std::string condTemp = allocTemp("w");
            emit("    " + condTemp + " =w csltl " + indexVar + ", " + sizeVar + "\n");
            m_stats.instructionsGenerated++;
            
            // Store condition for CFG to emit conditional branch
            m_lastCondition = condTemp;
        }
    }
    
    // Check if this is a SELECT CASE test block
    // Test blocks are empty but need to emit comparison logic
    else if (block->statements.empty() && !block->label.empty() && 
        block->label.find("CASE") != std::string::npos && 
        block->label.find("Test") != std::string::npos) {
        
        // This is a SELECT CASE test block
        // We need to emit comparison logic here
        // The comparison value and test value will be passed via the CFG context
        // For now, emit a placeholder that will be filled by the proper SELECT CASE implementation
        emitComment("SELECT CASE test block - comparison logic needed");
        
        // We need the SELECT CASE expression value and the CASE value(s)
        // This requires passing context from the SELECT block through the CFG
        // For now, we'll handle this in emitCase by storing the select value globally
        // Look up which SELECT CASE this test block belongs to
        auto selectCaseIt = m_programCFG->mainCFG->selectCaseInfo.find(block->id);
        if (selectCaseIt != m_programCFG->mainCFG->selectCaseInfo.end()) {
            const auto& selectInfo = selectCaseIt->second;
            const CaseStatement* caseStmt = selectInfo.caseStatement;
            
            // Find which test block index this is
            size_t testIndex = 0;
            for (size_t i = 0; i < selectInfo.testBlocks.size(); i++) {
                if (selectInfo.testBlocks[i] == block->id) {
                    testIndex = i;
                    break;
                }
            }
            
            if (testIndex >= caseStmt->whenClauses.size()) {
                emitComment("ERROR: Test block index out of range");
                m_lastCondition = allocTemp("w");
                emit("    " + m_lastCondition + " =w copy 0\n");
                m_stats.instructionsGenerated++;
            } else {
                const auto& clause = caseStmt->whenClauses[testIndex];
                
                // Evaluate the SELECT CASE expression to get the value to compare
                std::string selectValue = emitExpression(caseStmt->caseExpression.get());
                VariableType selectType = inferExpressionType(caseStmt->caseExpression.get());
                std::string selectQBEType = getQBEType(selectType);
            
                if (clause.isRange) {
                    // CASE x TO y range - check if value is between start and end (inclusive)
                    std::string startTemp = emitExpression(clause.rangeStart.get());
                    std::string endTemp = emitExpression(clause.rangeEnd.get());
                    
                    // Check: selectValue >= start AND selectValue <= end
                    std::string cmpGE = allocTemp("w");
                    std::string cmpLE = allocTemp("w");
                    
                    if (selectQBEType == "d") {
                        std::string selectD = allocTemp("d");
                        emit("    " + selectD + " =d sltof " + selectValue + "\n");
                        m_stats.instructionsGenerated++;
                        emit("    " + cmpGE + " =w cged " + selectD + ", " + startTemp + "\n");
                        m_stats.instructionsGenerated++;
                        emit("    " + cmpLE + " =w cled " + selectD + ", " + endTemp + "\n");
                        m_stats.instructionsGenerated++;
                    } else {
                        // Convert start and end to word if needed
                        std::string startWord = startTemp;
                        std::string endWord = endTemp;
                        if (selectQBEType == "w" || selectQBEType == "l") {
                            // Need to convert double to int
                            std::string tempStart = allocTemp("w");
                            emit("    " + tempStart + " =w dtosi " + startTemp + "\n");
                            m_stats.instructionsGenerated++;
                            startWord = tempStart;
                            
                            std::string tempEnd = allocTemp("w");
                            emit("    " + tempEnd + " =w dtosi " + endTemp + "\n");
                            m_stats.instructionsGenerated++;
                            endWord = tempEnd;
                        }
                        emit("    " + cmpGE + " =w csgew " + selectValue + ", " + startWord + "\n");
                        m_stats.instructionsGenerated++;
                        emit("    " + cmpLE + " =w cslew " + selectValue + ", " + endWord + "\n");
                        m_stats.instructionsGenerated++;
                    }
                    
                    // AND the two comparisons together
                    std::string andTemp = allocTemp("w");
                    emit("    " + andTemp + " =w and " + cmpGE + ", " + cmpLE + "\n");
                    m_stats.instructionsGenerated++;
                    m_lastCondition = andTemp;
                } else if (clause.isCaseIs) {
                    // CASE IS condition - evaluate right expression and compare
                    std::string rightTemp = emitExpression(clause.caseIsRightExpr.get());
                    std::string cmpTemp = allocTemp("w");
                    std::string opStr;
                    if (selectQBEType == "d") {
                        std::string selectD = allocTemp("d");
                        emit("    " + selectD + " =d sltof " + selectValue + "\n");
                        m_stats.instructionsGenerated++;
                        opStr = getComparisonOpDouble(clause.caseIsOperator);
                        emit("    " + cmpTemp + " =w " + opStr + " " + selectD + ", " + rightTemp + "\n");
                    } else {
                        // Convert to word
                        std::string rightWord = rightTemp;
                        std::string temp = allocTemp("w");
                        emit("    " + temp + " =w dtosi " + rightTemp + "\n");
                        m_stats.instructionsGenerated++;
                        rightWord = temp;
                        opStr = getComparisonOp(clause.caseIsOperator);
                        emit("    " + cmpTemp + " =w " + opStr + " " + selectValue + ", " + rightWord + "\n");
                    }
                    m_stats.instructionsGenerated++;
                    m_lastCondition = cmpTemp;
                } else if (clause.values.size() == 1) {
                    // Single value comparison
                    std::string valueTemp = emitExpression(clause.values[0].get());
                    std::string cmpTemp = allocTemp("w");
                    if (selectQBEType == "d") {
                        std::string selectD = allocTemp("d");
                        emit("    " + selectD + " =d sltof " + selectValue + "\n");
                        m_stats.instructionsGenerated++;
                        std::string opStr = "ceqd"; // equal for double
                        emit("    " + cmpTemp + " =w " + opStr + " " + selectD + ", " + valueTemp + "\n");
                    } else {
                        std::string valueWord = valueTemp;
                        std::string temp = allocTemp("w");
                        emit("    " + temp + " =w dtosi " + valueTemp + "\n");
                        m_stats.instructionsGenerated++;
                        valueWord = temp;
                        emit("    " + cmpTemp + " =w ceqw " + selectValue + ", " + valueWord + "\n");
                    }
                    m_stats.instructionsGenerated++;
                    m_lastCondition = cmpTemp;
                } else if (clause.values.size() > 1) {
                    // Multiple values - OR them together
                    std::vector<std::string> comparisons;
                    for (const auto& valueExpr : clause.values) {
                        std::string valueTemp = emitExpression(valueExpr.get());
                        std::string cmpTemp = allocTemp("w");
                        if (selectQBEType == "d") {
                            std::string selectD = allocTemp("d");
                            emit("    " + selectD + " =d sltof " + selectValue + "\n");
                            m_stats.instructionsGenerated++;
                            std::string opStr = "ceqd";
                            emit("    " + cmpTemp + " =w " + opStr + " " + selectD + ", " + valueTemp + "\n");
                        } else {
                            std::string valueWord = valueTemp;
                            std::string temp = allocTemp("w");
                            emit("    " + temp + " =w dtosi " + valueTemp + "\n");
                            m_stats.instructionsGenerated++;
                            valueWord = temp;
                            emit("    " + cmpTemp + " =w ceqw " + selectValue + ", " + valueWord + "\n");
                        }
                        m_stats.instructionsGenerated++;
                        comparisons.push_back(cmpTemp);
                    }
                    
                    // OR all comparisons
                    std::string orTemp = comparisons[0];
                    for (size_t i = 1; i < comparisons.size(); i++) {
                        std::string newOr = allocTemp("w");
                        emit("    " + newOr + " =w or " + orTemp + ", " + comparisons[i] + "\n");
                        m_stats.instructionsGenerated++;
                        orTemp = newOr;
                    }
                    m_lastCondition = orTemp;
                } else {
                    // Invalid case
                    emitComment("ERROR: Empty CASE clause");
                    m_lastCondition = allocTemp("w");
                    emit("    " + m_lastCondition + " =w copy 0\n");
                    m_stats.instructionsGenerated++;
                }
            }
        }
    }
    
    // Check if this is a DO loop header block and push loop context
    auto doStructIt = m_cfg->doLoopStructure.find(block->id);
    if (doStructIt != m_cfg->doLoopStructure.end()) {
        const auto& doBlocks = doStructIt->second;
        std::string exitLabel = getBlockLabel(doBlocks.exitBlock);
        pushLoop(exitLabel, "", "DO", "");
    }
    
    // Check if this is an "After DO" exit block and pop loop context
    if (block->label.find("After DO") != std::string::npos && block->isLoopExit) {
        // Pop the DO loop context
        popLoop();
    }
    
    // Emit all statements in the block
    for (const Statement* stmt : block->statements) {
        if (stmt) {
            // Add line number comment if available
            if (m_config.emitComments) {
                int lineNum = block->getLineNumber(stmt);
                if (lineNum > 0) {
                    emitComment("Line " + std::to_string(lineNum));
                }
            }
            emitStatement(stmt);
            if (m_lastStatementWasTerminator) {
                break;
            }
        }
    }
    
    // Emit control flow based on successors
    // Only emit if last statement didn't already emit a terminator
    if (m_lastStatementWasTerminator) {
        // Statement already handled control flow (GOTO, RETURN, END)
        emit("\n");
        m_currentBlock = nullptr;
        return;
    }
    
    if (block->successors.empty()) {
        // No successors - jump to exit (tidy_exit for functions)
        emit("    jmp @" + getFunctionExitLabel() + "\n");
        m_stats.instructionsGenerated++;
    } else if (block->successors.size() == 1) {
        // Single successor - unconditional jump (if not fallthrough to next block)
        int nextBlockId = block->successors[0];
        
        // Check if this is a fallthrough to the next sequential block
        bool isFallthrough = (nextBlockId == block->id + 1);
        
        if (!isFallthrough) {
            std::string nextLabel = getBlockLabel(nextBlockId);
            emit("    jmp @" + nextLabel + "\n");
            m_stats.instructionsGenerated++;
        }
        // Otherwise, fall through naturally
    } else if (block->successors.size() == 2) {
        // Two successors - conditional branch
        // The last statement should have stored the condition in m_lastCondition
        if (!m_lastCondition.empty()) {
            std::string trueLabel = getBlockLabel(block->successors[0]);
            std::string falseLabel = getBlockLabel(block->successors[1]);
            emit("    jnz " + m_lastCondition + ", @" + trueLabel + ", @" + falseLabel + "\n");
            m_stats.instructionsGenerated++;
            m_lastCondition.clear();  // Reset for next block
        } else {
            // Fallback: jump to first successor
            std::string nextLabel = getBlockLabel(block->successors[0]);
            emit("    jmp @" + nextLabel + "\n");
            m_stats.instructionsGenerated++;
        }
    } else if (block->successors.size() > 2) {
        // Multiple successors (e.g., ON GOTO/GOSUB or SELECT CASE)
        // For now, just jump to first successor
        // TODO: Implement multi-way branches properly
        std::string nextLabel = getBlockLabel(block->successors[0]);
        emit("    jmp @" + nextLabel + "\n");
        m_stats.instructionsGenerated++;
    }
    
    emit("\n");
    
    // Clear current block
    m_currentBlock = nullptr;
}

} // namespace FasterBASIC