//
// qbe_codegen_helpers.cpp
// FasterBASIC QBE Code Generator - Helper Functions
//
// This file contains utility functions:
// - Code emission helpers (emit, emitLine, emitComment, emitLabel)
// - Temporary variable allocation
// - Label generation
// - Type mapping and conversion
// - Variable/array reference management
// - String escaping
// - Loop context management
//

#include "../fasterbasic_qbe_codegen.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cctype>

namespace FasterBASIC {

// =============================================================================
// Code Emission Helpers
// =============================================================================

void QBECodeGenerator::emit(const std::string& code) {
    m_output << code;
}

void QBECodeGenerator::emitLine(const std::string& code) {
    m_output << code << "\n";
}

void QBECodeGenerator::emitComment(const std::string& comment) {
    if (m_config.emitComments) {
        m_output << "    # " << comment << "\n";
    }
}

void QBECodeGenerator::emitLabel(const std::string& label) {
    m_output << "@" << label << "\n";
    m_stats.labelsGenerated++;
}

// =============================================================================
// Temporary Variable Management
// =============================================================================

std::string QBECodeGenerator::allocTemp(const std::string& qbeType) {
    std::string tempName = "%t" + std::to_string(m_tempCounter++);
    m_tempTypes[tempName] = qbeType;
    return tempName;
}

std::string QBECodeGenerator::allocLabel() {
    return "L" + std::to_string(m_labelCounter++);
}

void QBECodeGenerator::freeTemp(const std::string& temp) {
    // QBE manages SSA temporaries automatically
    // This is a no-op but kept for API consistency
}

// =============================================================================
// Label Generation
// =============================================================================

std::string QBECodeGenerator::makeLabel(const std::string& prefix) {
    return prefix + "_" + std::to_string(m_labelCounter++);
}

std::string QBECodeGenerator::getBlockLabel(int blockId) {
    auto it = m_labels.find(blockId);
    if (it != m_labels.end()) {
        return it->second;
    }
    
    std::string label = "block_" + std::to_string(blockId);
    m_labels[blockId] = label;
    return label;
}

std::string QBECodeGenerator::getLineLabel(int lineNumber) {
    int key = lineNumber + 1000000;  // Offset to avoid collision with block IDs
    auto it = m_labels.find(key);
    if (it != m_labels.end()) {
        return it->second;
    }
    
    std::string label = "line_" + std::to_string(lineNumber);
    m_labels[key] = label;
    return label;
}

std::string QBECodeGenerator::getFunctionExitLabel() {
    // If in a function, return tidy_exit label for cleanup
    // Otherwise, return regular exit label
    if (!m_functionStack.empty()) {
        return m_functionStack.top().tidyExitLabel;
    }
    return "exit";
}

int QBECodeGenerator::getFallthroughBlock(const Statement* stmt) const {
    if (!m_cfg) {
        return -1;
    }

    int basicLine = 0;
    if (m_currentBlock && stmt) {
        basicLine = m_currentBlock->getLineNumber(stmt);
    }
    if (basicLine <= 0 && stmt) {
        basicLine = stmt->location.line;
    }
    if (basicLine > 0) {
        int candidate = m_cfg->getBlockForLineOrNext(basicLine + 1);
        if (candidate >= 0 && (!m_currentBlock || candidate != m_currentBlock->id)) {
            return candidate;
        }
    }

    if (m_currentBlock) {
        for (const auto& edge : m_cfg->edges) {
            if (edge.sourceBlock == m_currentBlock->id && edge.type == EdgeType::FALLTHROUGH) {
                return edge.targetBlock;
            }
        }
    }

    if (m_currentBlock) {
        int nextSequential = m_currentBlock->id + 1;
        if (nextSequential < m_cfg->getBlockCount()) {
            return nextSequential;
        }
    }

    return -1;
}

// =============================================================================
// Type System - QBE Type Mapping
// =============================================================================

std::string QBECodeGenerator::getQBEType(VariableType type) {
    switch (type) {
        case VariableType::INT:
            return "l";  // long (64-bit) - BASIC integers are always 64-bit
        case VariableType::FLOAT:
            return "s";  // single precision float (32-bit)
        case VariableType::DOUBLE:
            return "d";  // double precision float (64-bit)
        case VariableType::STRING:
            return "l";  // long (64-bit pointer)
        case VariableType::UNICODE:
            return "l";  // long (64-bit pointer)
        case VariableType::ADAPTIVE:
            return "l";  // ADAPTIVE should be resolved before reaching here, default to long
        default:
            return "l";  // default to long (64-bit)
    }
}

// =============================================================================
// TypeDescriptor-based Type System (New QBE-aligned system)
// =============================================================================

std::string QBECodeGenerator::getQBETypeD(const TypeDescriptor& typeDesc) {
    // Use the TypeDescriptor's built-in QBE type mapping
    return typeDesc.toQBEType();
}

// Get the actual QBE type that an expression produces
// This is important because comparisons return 'w' but variables use 'l'
std::string QBECodeGenerator::getActualQBEType(const Expression* expr) {
    if (!expr) return "d";  // Default to double
    
    ASTNodeType nodeType = expr->getType();
    
    // Comparisons always return 'w' (32-bit word for boolean result)
    if (nodeType == ASTNodeType::EXPR_BINARY) {
        const BinaryExpression* binExpr = static_cast<const BinaryExpression*>(expr);
        TokenType op = binExpr->op;
        
        bool comparisonOp = (op == TokenType::EQUAL ||
                            op == TokenType::NOT_EQUAL ||
                            op == TokenType::LESS_THAN ||
                            op == TokenType::LESS_EQUAL ||
                            op == TokenType::GREATER_THAN ||
                            op == TokenType::GREATER_EQUAL);
        
        if (comparisonOp) {
            return "w";  // Comparisons return word (boolean)
        }
        
        // For AND/OR/XOR, recursively check operands to determine size
        bool logicalOp = (op == TokenType::AND || 
                         op == TokenType::OR || 
                         op == TokenType::XOR);
        if (logicalOp) {
            std::string leftType = getActualQBEType(binExpr->left.get());
            std::string rightType = getActualQBEType(binExpr->right.get());
            // If either is 'l', result is 'l', otherwise 'w'
            if (leftType == "l" || rightType == "l") {
                return "l";
            }
            return "w";
        }
    }
    
    // For unary expressions, handle NOT operator
    if (nodeType == ASTNodeType::EXPR_UNARY) {
        const UnaryExpression* unaryExpr = static_cast<const UnaryExpression*>(expr);
        if (unaryExpr->op == TokenType::NOT) {
            return "w";  // NOT always returns 'w' (32-bit) like other integer intrinsics
        }
        // For unary + or -, return the type of the operand
        return getActualQBEType(unaryExpr->expr.get());
    }
    
    // For function calls, check if it's an intrinsic that returns 'w' instead of 'l'
    if (nodeType == ASTNodeType::EXPR_FUNCTION_CALL) {
        const FunctionCallExpression* funcExpr = static_cast<const FunctionCallExpression*>(expr);
        std::string upper = funcExpr->name;
        for (char& c : upper) c = std::toupper(c);
        
        // These intrinsics return 'w' (32-bit) even though their semantic type is INT
        // They always return 'w' regardless of argument types
        if (upper == "SGN" || upper == "FIX" || upper == "CINT" || upper == "INT" ||
            upper == "RAND" || upper == "ASC" || upper == "INSTR" ||
            upper == "CSRLIN" || upper == "POS" || upper == "ERR" || upper == "ERL") {
            return "w";
        }
        
        // LEN returns 'l' (64-bit) for string length
        if (upper == "LEN") {
            return "l";
        }
        
        // ABS and MIN/MAX return the same type as their arguments
        if (upper == "ABS" && funcExpr->arguments.size() == 1) {
            return getActualQBEType(funcExpr->arguments[0].get());
        }
        
        if ((upper == "MIN" || upper == "MAX") && funcExpr->arguments.size() == 2) {
            std::string leftType = getActualQBEType(funcExpr->arguments[0].get());
            std::string rightType = getActualQBEType(funcExpr->arguments[1].get());
            // If either is 'l' or 'd', promote to that type
            if (leftType == "d" || rightType == "d") return "d";
            if (leftType == "l" || rightType == "l") return "l";
            return "w";
        }
    }
    
    // For array access expressions, check the actual loaded type
    // INT arrays are loaded as 'w' (32-bit), but variables use 'l' (64-bit)
    if (nodeType == ASTNodeType::EXPR_ARRAY_ACCESS) {
        const ArrayAccessExpression* arrayExpr = static_cast<const ArrayAccessExpression*>(expr);
        
        // Get the element type from symbol table
        if (m_symbols && m_symbols->arrays.find(arrayExpr->name) != m_symbols->arrays.end()) {
            const auto& arraySym = m_symbols->arrays.at(arrayExpr->name);
            
            // Use TypeDescriptor if available
            if (arraySym.elementTypeDesc.baseType != BaseType::UNKNOWN) {
                return arraySym.elementTypeDesc.toQBEType();
            }
            
            // Fall back to legacy type
            if (arraySym.type == VariableType::INT) {
                return "w";  // INT arrays load as word (32-bit)
            } else if (arraySym.type == VariableType::DOUBLE || arraySym.type == VariableType::FLOAT) {
                return "d";  // Floating point
            } else if (arraySym.type == VariableType::STRING) {
                return "l";  // String pointers
            }
        }
    }
    
    // For other expressions, use the semantic type
    VariableType semType = inferExpressionType(expr);
    return getQBEType(semType);
}

std::string QBECodeGenerator::getQBEMemOpD(const TypeDescriptor& typeDesc) {
    // Use the TypeDescriptor's built-in QBE memory operation suffix (for stores)
    return typeDesc.toQBEMemOp();
}

std::string QBECodeGenerator::getQBELoadOpD(const TypeDescriptor& typeDesc) {
    // Use the TypeDescriptor's built-in QBE load operation suffix (with sign extension)
    return typeDesc.toQBELoadOp();
}

TypeDescriptor QBECodeGenerator::getVariableTypeD(const std::string& varName) {
    // FOR loop indices are ALWAYS INTEGER - check plain name without suffix
    std::string plainName = stripTypeSuffix(varName);
    if (m_forLoopVariables.count(plainName) > 0) {
        return TypeDescriptor(BaseType::INTEGER);
    }
    
    // Check if this is a function return variable (function name = return variable)
    if (m_inFunction && m_cfg && varName == m_currentFunction) {
        // Convert legacy return type to TypeDescriptor
        return legacyTypeToDescriptor(m_cfg->returnType);
    }
    
    // Check if this is a function parameter
    if (m_inFunction && m_cfg) {
        for (size_t i = 0; i < m_cfg->parameters.size(); ++i) {
            if (m_cfg->parameters[i] == varName) {
                // Found the parameter - return its type
                if (i < m_cfg->parameterTypes.size()) {
                    return legacyTypeToDescriptor(m_cfg->parameterTypes[i]);
                }
                break;
            }
        }
    }
    
    // Check symbol table for TypeDescriptor
    if (m_symbols) {
        auto it = m_symbols->variables.find(varName);
        if (it != m_symbols->variables.end()) {
            // Use the new TypeDescriptor if available
            if (it->second.typeDesc.baseType != BaseType::UNKNOWN) {
                return it->second.typeDesc;
            }
            // Fall back to legacy type
            return legacyTypeToDescriptor(it->second.type);
        }
    }
    
    // Fall back to type suffix inference
    char suffix = getTypeSuffix(varName);
    return tokenSuffixToDescriptor(getTokenTypeFromSuffix(suffix));
}

// Helper to convert suffix character to TokenType
TokenType QBECodeGenerator::getTokenTypeFromSuffix(char suffix) {
    switch (suffix) {
        case '%': return TokenType::TYPE_INT;
        case '!': return TokenType::TYPE_FLOAT;
        case '#': return TokenType::TYPE_DOUBLE;
        case '$': return TokenType::TYPE_STRING;
        case '@': return TokenType::TYPE_BYTE;
        case '^': return TokenType::TYPE_SHORT;
        case '&': return TokenType::TYPE_INT;  // LONG uses same token as INT
        default: return TokenType::TYPE_DOUBLE;  // Default numeric type
    }
}

std::string QBECodeGenerator::getQBETypeFromSuffix(char suffix) {
    switch (suffix) {
        case '%': return "w";  // INTEGER
        case '!': return "d";  // SINGLE (QBE doesn't support single, use double)
        case '#': return "d";  // DOUBLE
        case '$': return "l";  // STRING (pointer)
        case '&': return "l";  // LONG (64-bit int in QBE is 'l')
        default: return "d";   // Default to DOUBLE (like Lua, numbers are doubles by default)
    }
}

// =============================================================================
// Type System - Type Suffix Detection
// =============================================================================

char QBECodeGenerator::getTypeSuffix(const std::string& varName) {
    if (varName.empty()) return '#';  // Default to DOUBLE
    char last = varName.back();
    if (last == '%' || last == '!' || last == '#' || last == '$' || 
        last == '&' || last == '@' || last == '^') {
        return last;
    }
    return '#';  // Default to DOUBLE (numeric default in BASIC)
}

VariableType QBECodeGenerator::getVariableType(const std::string& varName) {
    // FOR loop indices are ALWAYS integers (hard rule in BASIC)
    // Check plain name without suffix
    std::string plainName = stripTypeSuffix(varName);
    if (m_forLoopVariables.count(plainName) > 0) {
        return VariableType::INT;
    }
    
    // Check if this is a LOCAL variable in the current function
    if (m_inFunction) {
        auto localIt = m_localVariableTypes.find(varName);
        if (localIt != m_localVariableTypes.end()) {
            return localIt->second;
        }
    }
    
    // Check if this is a function return variable (function name = return variable)
    if (m_inFunction && m_cfg && varName == m_currentFunction) {
        return m_cfg->returnType;
    }
    
    // Check if this is a function parameter - try multiple name variations
    if (m_inFunction && m_cfg) {
        // Try exact match first
        for (size_t i = 0; i < m_cfg->parameters.size(); ++i) {
            if (m_cfg->parameters[i] == varName) {
                // Found the parameter - return its type
                if (i < m_cfg->parameterTypes.size()) {
                    return m_cfg->parameterTypes[i];
                }
                break;
            }
        }
        
        // Try matching base names (without suffix)
        std::string varBase = stripTypeSuffix(varName);
        for (size_t i = 0; i < m_cfg->parameters.size(); ++i) {
            std::string paramBase = stripTypeSuffix(m_cfg->parameters[i]);
            if (paramBase == varBase) {
                // Found the parameter - return its type
                if (i < m_cfg->parameterTypes.size()) {
                    return m_cfg->parameterTypes[i];
                }
                break;
            }
        }
    }
    
    // First check symbol table
    if (m_symbols) {
        auto it = m_symbols->variables.find(varName);
        if (it != m_symbols->variables.end()) {
            return it->second.type;
        }
    }
    
    // Fall back to type suffix
    char suffix = getTypeSuffix(varName);
    switch (suffix) {
        case '%': return VariableType::INT;
        case '!': return VariableType::FLOAT;
        case '#': return VariableType::DOUBLE;
        case '$': return VariableType::STRING;
        case '&': return VariableType::INT;  // LONG treated as INT in our system
        default: return VariableType::DOUBLE;   // Default numeric type is DOUBLE (like Lua)
    }
}

// =============================================================================
// Variable and Array References
// =============================================================================

// Sanitize variable name for QBE - replace BASIC suffixes with underscores
std::string QBECodeGenerator::sanitizeQBEVariableName(const std::string& varName) {
    std::string result = varName;
    // Replace BASIC type suffix characters with safe equivalents
    for (size_t i = 0; i < result.length(); ++i) {
        char c = result[i];
        if (c == '%' || c == '$' || c == '#' || c == '!' || c == '&' || c == '@' || c == '^') {
            result[i] = '_';
        }
    }
    return result;
}

std::string QBECodeGenerator::getVariableRef(const std::string& varName) {
    // Sanitize the variable name first
    std::string safeName = sanitizeQBEVariableName(varName);
    
    // Check if this is a FOR EACH loop variable
    LoopContext* loop = getCurrentLoop();
    if (loop && loop->isForEach && varName == loop->forVariable) {
        // This is a FOR EACH variable - generate array element access
        std::string elemAddr = allocTemp("l");
        std::string dataPtr = allocTemp("l");
        std::string offsetTemp = allocTemp("l");
        
        // Load data pointer from array descriptor
        emit("    " + dataPtr + " =l loadl " + loop->forEachArrayDesc + "\n");
        m_stats.instructionsGenerated++;
        
        // Calculate offset: index * 8
        emit("    " + offsetTemp + " =l mul " + loop->forEachIndex + ", 8\n");
        m_stats.instructionsGenerated++;
        
        // Calculate element address
        emit("    " + elemAddr + " =l add " + dataPtr + ", " + offsetTemp + "\n");
        m_stats.instructionsGenerated++;
        
        // Load and return element value based on type
        std::string elemVal = allocTemp(getQBEType(loop->forEachElemType));
        if (loop->forEachElemType == VariableType::STRING || 
            loop->forEachElemType == VariableType::INT) {
            emit("    " + elemVal + " =l loadl " + elemAddr + "\n");
        } else {
            emit("    " + elemVal + " =d loadd " + elemAddr + "\n");
        }
        m_stats.instructionsGenerated++;
        return elemVal;
    }
    
    // IMPORTANT: Check function scope FIRST to implement proper variable shadowing
    // Function parameters and local variables should shadow globals and FOR loop variables
    if (m_inFunction && m_cfg) {
        // First check DEF FN parameters (these have priority for DEF FN functions)
        if (!m_defFnParams.empty()) {
            // Check all variations of the variable name
            if (m_defFnParams.count(varName) > 0) {
                return m_defFnParams.at(varName);
            }
            if (m_defFnParams.count(safeName) > 0) {
                return m_defFnParams.at(safeName);
            }
        }
        
        // Check if varName is a parameter of the current function
        // Need to check multiple variations because parameter names might be normalized
        std::string plainName = stripTypeSuffix(varName);
        std::string safePlainName = stripTypeSuffix(safeName);
        
        for (const auto& param : m_cfg->parameters) {
            std::string paramPlain = stripTypeSuffix(param);
            
            // Match if: exact match, safe match, or plain name matches
            if (param == varName || param == safeName || 
                paramPlain == plainName || paramPlain == safePlainName) {
                // This is a parameter - use the original parameter name from function signature
                return "%" + param;
            }
        }
        
        // Check if this is a local variable
        if (m_localVariables.count(varName) > 0 || m_localVariables.count(safeName) > 0) {
            // Local variable - use %local_ prefix
            return "%local_" + safeName;
        }
        
        // Check if this is explicitly shared
        // If SHARED, we need to fall through to load from global storage below
        if (m_sharedVariables.count(varName) > 0 || m_sharedVariables.count(safeName) > 0) {
            // Fall through to global variable loading below
        } else {
            // Not a parameter, not local, not shared - default to shared for backward compatibility
            // (In proper BASIC, undeclared vars in functions should error, but we're lenient)
            return "%var_" + safeName;
        }
    }
    
    // FOR loop variables use plain names without suffix (but only outside functions)
    std::string plainName = stripTypeSuffix(safeName);
    if (m_forLoopVariables.count(plainName) > 0) {
        return "%var_" + plainName;
    }
    
    // Check if this is a GLOBAL variable (only after checking function scope)
    if (m_symbols) {
        auto it = m_symbols->variables.find(varName);
        if (it != m_symbols->variables.end() && it->second.isGlobal) {
            // Generate efficient pointer arithmetic load sequence for global variable
            int slot = it->second.globalOffset;
            VariableType type = it->second.type;
            
            // Calculate address using global vector label directly (pre-calculate byte offset: slot * 8)
            int byteOffset = slot * 8;
            std::string addr = allocTemp("l");
            emit("    " + addr + " =l add $__global_vector, " + std::to_string(byteOffset) + "\n");
            
            // Load value into cache variable
            std::string cache = allocTemp(getQBEType(type));
            if (type == VariableType::DOUBLE) {
                emit("    " + cache + " =d loadd " + addr + "\n");
            } else {
                emit("    " + cache + " =l loadl " + addr + "\n");
            }
            
            m_stats.instructionsGenerated += 2;
            return cache;  // Return cache temp
        }
    }
    
    // Outside functions, all variables are global
    // Variables are SSA temporaries in QBE
    // We use a consistent naming scheme: %var_<MANGLED_NAME>
    // 
    // Note: varName is already mangled by the semantic analyzer with type suffixes:
    //   X%  -> "X_INT"     -> %var_X_INT
    //   Y#  -> "Y_DOUBLE"  -> %var_Y_DOUBLE
    //   S$  -> "S_STRING"  -> %var_S_STRING
    //   Z   -> "Z_FLOAT"   -> %var_Z_FLOAT (default SINGLE type)
    //
    // This makes the QBE IL self-documenting with explicit types in variable names
    return "%var_" + safeName;
}

std::string QBECodeGenerator::stripTypeSuffix(const std::string& varName) {
    // Strip common type suffix patterns: _INT, _FLOAT, _DOUBLE, _STRING, _BYTE, _SHORT, etc.
    size_t pos = varName.find('_');
    if (pos != std::string::npos) {
        return varName.substr(0, pos);
    }
    return varName;
}

std::string QBECodeGenerator::getArrayRef(const std::string& arrayName) {
    // Arrays are pointers stored in temporaries
    // We use a consistent naming scheme: %arr_<MANGLED_NAME>
    // 
    // Note: arrayName is already mangled with type suffix by semantic analyzer
    //   A%()  -> "A_INT"     -> %arr_A_INT
    //   B#()  -> "B_DOUBLE"  -> %arr_B_DOUBLE
    return "%arr_" + arrayName;
}

void QBECodeGenerator::declareVariable(const std::string& varName, VariableType type) {
    if (m_variables.find(varName) == m_variables.end()) {
        m_variables[varName] = m_variables.size();
        m_varTypes[varName] = getQBEType(type);
    }
}

void QBECodeGenerator::declareArray(const std::string& arrayName, VariableType type) {
    if (m_arrays.find(arrayName) == m_arrays.end()) {
        m_arrays[arrayName] = m_arrays.size();
    }
}

// =============================================================================
// String Utilities
// =============================================================================

std::string QBECodeGenerator::escapeString(const std::string& str) {
    std::string result;
    result.reserve(str.length());
    
    for (char c : str) {
        switch (c) {
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            case '\\': result += "\\\\"; break;
            case '"': result += "\\\""; break;
            default:
                if (c >= 32 && c < 127) {
                    result += c;
                } else {
                    // Escape non-printable characters
                    char buf[8];
                    snprintf(buf, sizeof(buf), "\\x%02x", static_cast<unsigned char>(c));
                    result += buf;
                }
                break;
        }
    }
    
    return result;
}

std::string QBECodeGenerator::getRuntimeFunction(const std::string& operation, VariableType type) {
    // Map operation name and type to runtime function name
    std::string prefix = "basic_";
    
    if (operation == "print") {
        switch (type) {
            case VariableType::INT: return "basic_print_int";
            case VariableType::DOUBLE: return "basic_print_double";
            case VariableType::FLOAT: return "basic_print_float";
            case VariableType::STRING: return "basic_print_string";
            default: return "basic_print_int";
        }
    }
    
    if (operation == "input") {
        switch (type) {
            case VariableType::INT: return "basic_input_int";
            case VariableType::DOUBLE: return "basic_input_double";
            case VariableType::STRING: return "basic_input_string";
            default: return "basic_input_int";
        }
    }
    
    return prefix + operation;
}

// =============================================================================
// Constant Expression Evaluation
// =============================================================================

bool QBECodeGenerator::isConstantExpression(const Expression* expr) {
    if (!expr) return false;
    
    ASTNodeType nodeType = expr->getType();
    return (nodeType == ASTNodeType::EXPR_NUMBER || 
            nodeType == ASTNodeType::EXPR_STRING);
}

int QBECodeGenerator::evaluateConstantInt(const Expression* expr) {
    if (!expr) return 0;
    
    ASTNodeType nodeType = expr->getType();
    if (nodeType == ASTNodeType::EXPR_NUMBER) {
        const NumberExpression* numExpr = static_cast<const NumberExpression*>(expr);
        return static_cast<int>(numExpr->value);
    }
    
    return 0;
}

double QBECodeGenerator::evaluateConstantDouble(const Expression* expr) {
    if (!expr) return 0.0;
    
    ASTNodeType nodeType = expr->getType();
    if (nodeType == ASTNodeType::EXPR_NUMBER) {
        const NumberExpression* numExpr = static_cast<const NumberExpression*>(expr);
        return numExpr->value;
    }
    
    return 0.0;
}

// =============================================================================
// Loop Context Management
// =============================================================================

QBECodeGenerator::LoopContext* QBECodeGenerator::pushLoop(const std::string& exitLabel,
                               const std::string& continueLabel,
                               const std::string& type,
                               const std::string& forVariable,
                               bool isForEach) {
    m_loopStack.push_back({exitLabel, continueLabel, type, forVariable, isForEach});
    return &m_loopStack.back();
}

void QBECodeGenerator::popLoop() {
    if (!m_loopStack.empty()) {
        m_loopStack.pop_back();
    }
}

QBECodeGenerator::LoopContext* QBECodeGenerator::getCurrentLoop() {
    if (m_loopStack.empty()) return nullptr;
    return &m_loopStack.back();
}

// =============================================================================
// GOSUB Return Stack Management
// =============================================================================

void QBECodeGenerator::pushGosubReturn(const std::string& returnLabel) {
    m_gosubReturnLabels.push_back(returnLabel);
}

void QBECodeGenerator::popGosubReturn() {
    if (!m_gosubReturnLabels.empty()) {
        m_gosubReturnLabels.pop_back();
    }
}

std::string QBECodeGenerator::getCurrentGosubReturn() {
    if (m_gosubReturnLabels.empty()) return "";
    return m_gosubReturnLabels.back();
}

// =============================================================================
// Type Inference and Promotion
// =============================================================================

VariableType QBECodeGenerator::inferExpressionType(const Expression* expr) {
    if (!expr) return VariableType::DOUBLE;  // Default to DOUBLE
    
    ASTNodeType nodeType = expr->getType();
    
    switch (nodeType) {
        case ASTNodeType::EXPR_NUMBER: {
            const NumberExpression* numExpr = static_cast<const NumberExpression*>(expr);
            // Check if this is an integer literal (no decimal point, no exponent)
            double value = numExpr->value;
            bool isInteger = (value == std::floor(value)) && 
                             (value >= std::numeric_limits<int64_t>::min()) && 
                             (value <= std::numeric_limits<int64_t>::max());
            
            if (isInteger) {
                // Integer literals without decimal point are INT type
                // This allows "2 * x&" to be an integer operation
                return VariableType::INT;
            } else {
                // Floating point literals are DOUBLE
                return VariableType::DOUBLE;
            }
        }
        
        case ASTNodeType::EXPR_STRING:
            return VariableType::STRING;
        
        case ASTNodeType::EXPR_VARIABLE: {
            const VariableExpression* varExpr = static_cast<const VariableExpression*>(expr);
            return getVariableType(varExpr->name);
        }
        
        case ASTNodeType::EXPR_BINARY: {
            const BinaryExpression* binExpr = static_cast<const BinaryExpression*>(expr);
            
            // Check operator type first - some operations always return INT
            TokenType op = binExpr->op;
            bool integerOnlyOp = (op == TokenType::MOD || 
                                  op == TokenType::AND || 
                                  op == TokenType::OR || 
                                  op == TokenType::XOR);
            
            // Comparison operations also return INT (0 or 1)
            bool comparisonOp = (op == TokenType::EQUAL ||
                                op == TokenType::NOT_EQUAL ||
                                op == TokenType::LESS_THAN ||
                                op == TokenType::LESS_EQUAL ||
                                op == TokenType::GREATER_THAN ||
                                op == TokenType::GREATER_EQUAL);
            
            if (integerOnlyOp || comparisonOp) {
                return VariableType::INT;
            }
            
            // For arithmetic operations, infer from operand types
            VariableType leftType = inferExpressionType(binExpr->left.get());
            VariableType rightType = inferExpressionType(binExpr->right.get());
            
            // Special case: string concatenation
            if (op == TokenType::PLUS && leftType == VariableType::STRING && rightType == VariableType::STRING) {
                return VariableType::STRING;
            }
            
            // Division operator (/) always returns DOUBLE in BASIC
            // Use \ (INT_DIVIDE) for integer division
            if (op == TokenType::DIVIDE) {
                return VariableType::DOUBLE;
            }
            
            // Type promotion rules: DOUBLE is the default numeric type
            // Only use INT if BOTH operands are explicitly INT
            if (leftType == VariableType::DOUBLE || rightType == VariableType::DOUBLE) {
                return VariableType::DOUBLE;
            }
            if (leftType == VariableType::FLOAT || rightType == VariableType::FLOAT) {
                return VariableType::DOUBLE;  // FLOAT and DOUBLE both map to QBE 'd'
            }
            if (leftType == VariableType::INT && rightType == VariableType::INT) {
                return VariableType::INT;
            }
            return VariableType::DOUBLE;  // Default to DOUBLE
        }
        
        case ASTNodeType::EXPR_MEMBER_ACCESS: {
            // Member access - need to look up the field type
            const MemberAccessExpression* memberExpr = static_cast<const MemberAccessExpression*>(expr);
            
            // Get the base type name (could be variable or array element)
            std::string baseTypeName = inferMemberAccessType(memberExpr->object.get());
            if (baseTypeName.empty()) {
                return VariableType::DOUBLE;  // Default if can't determine
            }
            
            // Look up the field in the type
            const TypeSymbol* typeSymbol = getTypeSymbol(baseTypeName);
            if (!typeSymbol) {
                return VariableType::DOUBLE;
            }
            
            const TypeSymbol::Field* field = typeSymbol->findField(memberExpr->memberName);
            if (!field) {
                return VariableType::DOUBLE;
            }
            
            // Return the field's type
            if (field->isBuiltIn) {
                return field->builtInType;
            } else {
                // Nested UDT - return as USER_DEFINED
                return VariableType::USER_DEFINED;
            }
        }
        
        case ASTNodeType::EXPR_FUNCTION_CALL: {
            // Function call - look up return type
            const FunctionCallExpression* funcExpr = static_cast<const FunctionCallExpression*>(expr);
            
            // Check if this is a user-defined function in the CFG
            if (m_programCFG) {
                const ControlFlowGraph* funcCFG = m_programCFG->getFunctionCFG(funcExpr->name);
                if (funcCFG) {
                    return funcCFG->returnType;
                }
            }
            
            // Check symbol table
            if (m_symbols) {
                auto it = m_symbols->functions.find(funcExpr->name);
                if (it != m_symbols->functions.end()) {
                    return it->second.returnType;
                }
            }
            
            // Check builtin functions
            std::string upper = funcExpr->name;
            for (char& c : upper) c = std::toupper(c);

            // Any builtin with trailing $ or _STRING suffix returns a pointer
            if (!upper.empty() && upper.back() == '$') {
                return VariableType::STRING;
            }
            // Check for mangled string function names (e.g., STR_STRING, CHR_STRING)
            if (upper.length() > 7 && upper.substr(upper.length() - 7) == "_STRING") {
                return VariableType::STRING;
            }
            
            // String functions return STRING
            if (upper == "CHR$" || upper == "LEFT$" || upper == "RIGHT$" || upper == "MID$" ||
                upper == "__STRING_SLICE" || upper == "STR$" || upper == "SPACE$" || upper == "STRING$" || 
                upper == "UCASE$" || upper == "LCASE$" || upper == "TRIM$" || 
                upper == "LTRIM$" || upper == "RTRIM$" || upper == "JOIN$" || upper == "SPLIT$") {
                return VariableType::STRING;
            }
            
            // Integer functions return INT (LEN, ASC, INSTR return 64-bit or 32-bit ints)
            if (upper == "LEN" || upper == "ASC" || upper == "INSTR" || upper == "STRTYPE" || 
                upper == "RAND" || upper == "FIX" || upper == "CINT" || upper == "SGN" ||
                upper == "MIN" || upper == "MAX" || upper == "CSRLIN" || upper == "POS" ||
                upper == "INT" || upper == "ERR" || upper == "ERL") {
                return VariableType::INT;
            }
            
            // ABS returns the same type as its argument
            if (upper == "ABS" && funcExpr->arguments.size() == 1) {
                VariableType argType = inferExpressionType(funcExpr->arguments[0].get());
                return argType;
            }
            
            // Math functions return DOUBLE
            if (upper == "VAL" || upper == "RND" || upper == "SIN" || upper == "COS" || 
                upper == "TAN" || upper == "SQRT") {
                return VariableType::DOUBLE;
            }
            
            // Default for unknown functions
            return VariableType::DOUBLE;
        }
        
        case ASTNodeType::EXPR_ARRAY_ACCESS: {
            const ArrayAccessExpression* arrayExpr = static_cast<const ArrayAccessExpression*>(expr);
            
            // Look up array type from symbol table
            if (m_symbols && m_symbols->arrays.find(arrayExpr->name) != m_symbols->arrays.end()) {
                return m_symbols->arrays.at(arrayExpr->name).type;
            }
            
            // Default to DOUBLE if not found
            return VariableType::DOUBLE;
        }
        
        case ASTNodeType::EXPR_UNARY: {
            const UnaryExpression* unaryExpr = static_cast<const UnaryExpression*>(expr);
            
            // NOT always returns INT
            if (unaryExpr->op == TokenType::NOT) {
                return VariableType::INT;
            }
            
            // Unary + and - preserve the type of the operand
            return inferExpressionType(unaryExpr->expr.get());
        }
        
        case ASTNodeType::EXPR_IIF: {
            const IIFExpression* iifExpr = static_cast<const IIFExpression*>(expr);
            
            // Infer types from both branches
            VariableType trueType = inferExpressionType(iifExpr->trueValue.get());
            VariableType falseType = inferExpressionType(iifExpr->falseValue.get());
            
            // If both branches are the same type, return that type
            if (trueType == falseType) {
                return trueType;
            }
            
            // If one is STRING, return STRING (strings don't auto-convert)
            if (trueType == VariableType::STRING || falseType == VariableType::STRING) {
                return VariableType::STRING;
            }
            
            // Numeric type promotion: DOUBLE > FLOAT > INT
            if (trueType == VariableType::DOUBLE || falseType == VariableType::DOUBLE) {
                return VariableType::DOUBLE;
            }
            if (trueType == VariableType::FLOAT || falseType == VariableType::FLOAT) {
                return VariableType::DOUBLE;
            }
            
            // Default to DOUBLE if types are incompatible
            return VariableType::DOUBLE;
        }
        
        default:
            return VariableType::DOUBLE;
    }
}

std::string QBECodeGenerator::promoteToType(const std::string& value, 
                                           VariableType fromType, 
                                           VariableType toType,
                                           const std::string& actualQBEType) {
    // If types match, no promotion needed
    if (fromType == toType) return value;
    
    // Get QBE types to check for actual representation
    // Use actualQBEType if provided (for INT types where actual may be w or l)
    std::string fromQBE = actualQBEType.empty() ? getQBEType(fromType) : actualQBEType;
    std::string toQBE = getQBEType(toType);
    
    // If QBE types match, no conversion needed (e.g., INT->INT both map to 'l' or 'w')
    // Note: We use 'l' for INT in some contexts and 'w' in others depending on the operation
    // For runtime function returns that are 'l' being assigned to INT variables, no conversion
    if (fromQBE == toQBE) return value;
    
    // Promote INT to DOUBLE (w or l -> d)
    if (fromType == VariableType::INT && toType == VariableType::DOUBLE) {
        std::string temp = allocTemp("d");
        if (fromQBE == "w") {
            // Word to double: first extend to long, then convert
            std::string longTemp = allocTemp("l");
            emit("    " + longTemp + " =l extsw " + value + "\n");
            emit("    " + temp + " =d sltof " + longTemp + "\n");
            m_stats.instructionsGenerated += 2;
        } else {
            // Long to double: direct conversion
            emit("    " + temp + " =d sltof " + value + "\n");
            m_stats.instructionsGenerated++;
        }
        return temp;
    }
    
    // Promote INT to FLOAT (w or l -> s)
    if (fromType == VariableType::INT && toType == VariableType::FLOAT) {
        std::string temp = allocTemp("s");
        if (fromQBE == "w") {
            // Word to float: first extend to long, then convert
            std::string longTemp = allocTemp("l");
            emit("    " + longTemp + " =l extsw " + value + "\n");
            emit("    " + temp + " =s sltof " + longTemp + "\n");
            m_stats.instructionsGenerated += 2;
        } else {
            // Long to float: direct conversion
            emit("    " + temp + " =s sltof " + value + "\n");
            m_stats.instructionsGenerated++;
        }
        return temp;
    }
    
    // Convert DOUBLE to FLOAT (d -> s)
    if (fromType == VariableType::DOUBLE && toType == VariableType::FLOAT) {
        std::string temp = allocTemp("s");
        emit("    " + temp + " =s truncd " + value + "\n");
        m_stats.instructionsGenerated++;
        return temp;
    }
    
    // Convert FLOAT to DOUBLE (s -> d)
    if (fromType == VariableType::FLOAT && toType == VariableType::DOUBLE) {
        std::string temp = allocTemp("d");
        emit("    " + temp + " =d exts " + value + "\n");
        m_stats.instructionsGenerated++;
        return temp;
    }
    
    // Demote DOUBLE to INT (d -> w or l)
    if (fromType == VariableType::DOUBLE && toType == VariableType::INT) {
        // Convert double to long (default INT representation)
        std::string temp = allocTemp("l");
        emit("    " + temp + " =l dtosi " + value + "\n");
        m_stats.instructionsGenerated++;
        return temp;
    }
    
    // Demote FLOAT to INT (s -> w or l)
    if (fromType == VariableType::FLOAT && toType == VariableType::INT) {
        std::string temp = allocTemp("l");
        emit("    " + temp + " =l stosi " + value + "\n");
        m_stats.instructionsGenerated++;
        return temp;
    }
    
    // String conversions (handled by runtime functions, not implemented here yet)
    // if (fromType == VariableType::INT && toType == VariableType::STRING) {
    //     return emitIntToString(value);
    // }
    // 
    // if (fromType == VariableType::DOUBLE && toType == VariableType::STRING) {
    //     return emitDoubleToString(value);
    // }
    // 
    // if (fromType == VariableType::STRING && toType == VariableType::INT) {
    //     return emitStringToInt(value);
    // }
    // 
    // if (fromType == VariableType::STRING && toType == VariableType::DOUBLE) {
    //     return emitStringToDouble(value);
    // }
    
    // No conversion needed/available
    return value;
}

// =============================================================================
// Utility Functions
// =============================================================================

std::string QBECodeGenerator::toUpper(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::toupper(c); });
    return result;
}

std::string QBECodeGenerator::toLower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}

bool QBECodeGenerator::isNumericType(VariableType type) {
    return (type == VariableType::INT || 
            type == VariableType::FLOAT || 
            type == VariableType::DOUBLE);
}

bool QBECodeGenerator::isIntegerType(VariableType type) {
    return (type == VariableType::INT);
}

bool QBECodeGenerator::isFloatingType(VariableType type) {
    return (type == VariableType::FLOAT || 
            type == VariableType::DOUBLE);
}

bool QBECodeGenerator::isStringType(VariableType type) {
    return (type == VariableType::STRING || 
            type == VariableType::UNICODE);
}

// =============================================================================
// User-Defined Type Helpers
// =============================================================================

size_t QBECodeGenerator::calculateTypeSize(const std::string& typeName) {
    // Check cache first
    auto it = m_typeSizes.find(typeName);
    if (it != m_typeSizes.end()) {
        return it->second;
    }
    
    // Get type definition from symbol table
    const TypeSymbol* typeSymbol = getTypeSymbol(typeName);
    if (!typeSymbol) {
        // Type not found - return minimal size
        return 8;
    }
    
    size_t totalSize = 0;
    size_t maxAlignment = 8;  // Default to 8-byte alignment
    
    // Calculate size of each field
    for (const auto& field : typeSymbol->fields) {
        size_t fieldSize = 0;
        size_t fieldAlignment = 0;
        
        if (field.isBuiltIn) {
            // Use TypeDescriptor for accurate size calculation (handles LONG vs INT correctly)
            switch (field.typeDesc.baseType) {
                case BaseType::BYTE:
                case BaseType::UBYTE:
                    fieldSize = 1;
                    fieldAlignment = 1;
                    break;
                case BaseType::SHORT:
                case BaseType::USHORT:
                    fieldSize = 2;
                    fieldAlignment = 2;
                    break;
                case BaseType::INTEGER:
                case BaseType::UINTEGER:
                    fieldSize = 4;
                    fieldAlignment = 4;
                    break;
                case BaseType::LONG:
                case BaseType::ULONG:
                    fieldSize = 8;
                    fieldAlignment = 8;
                    break;
                case BaseType::SINGLE:
                    fieldSize = 4;
                    fieldAlignment = 4;
                    break;
                case BaseType::DOUBLE:
                    fieldSize = 8;
                    fieldAlignment = 8;
                    break;
                case BaseType::STRING:
                case BaseType::UNICODE:
                case BaseType::POINTER:
                    fieldSize = 8;  // Pointer
                    fieldAlignment = 8;
                    break;
                default:
                    fieldSize = 8;
                    fieldAlignment = 8;
                    break;
            }
        } else {
            // Nested user-defined type (recursive)
            fieldSize = calculateTypeSize(field.typeName);
            fieldAlignment = 8;  // Assume 8-byte alignment for nested structs
        }
        
        // Apply alignment padding
        if (totalSize % fieldAlignment != 0) {
            totalSize += fieldAlignment - (totalSize % fieldAlignment);
        }
        
        totalSize += fieldSize;
        
        if (fieldAlignment > maxAlignment) {
            maxAlignment = fieldAlignment;
        }
    }
    
    // Final padding to align to struct alignment
    if (totalSize % maxAlignment != 0) {
        totalSize += maxAlignment - (totalSize % maxAlignment);
    }
    
    // Cache and return
    m_typeSizes[typeName] = totalSize;
    return totalSize;
}

size_t QBECodeGenerator::calculateFieldOffset(const std::string& typeName, const std::string& fieldName) {
    // Check cache first
    auto typeIt = m_fieldOffsets.find(typeName);
    if (typeIt != m_fieldOffsets.end()) {
        auto fieldIt = typeIt->second.find(fieldName);
        if (fieldIt != typeIt->second.end()) {
            return fieldIt->second;
        }
    }
    
    // Get type definition
    const TypeSymbol* typeSymbol = getTypeSymbol(typeName);
    if (!typeSymbol) {
        return 0;
    }
    
    size_t currentOffset = 0;
    
    // Calculate offset for each field
    for (const auto& field : typeSymbol->fields) {
        size_t fieldSize = 0;
        size_t fieldAlignment = 0;
        
        if (field.isBuiltIn) {
            // Use TypeDescriptor for accurate size calculation (handles LONG vs INT correctly)
            switch (field.typeDesc.baseType) {
                case BaseType::BYTE:
                case BaseType::UBYTE:
                    fieldSize = 1;
                    fieldAlignment = 1;
                    break;
                case BaseType::SHORT:
                case BaseType::USHORT:
                    fieldSize = 2;
                    fieldAlignment = 2;
                    break;
                case BaseType::INTEGER:
                case BaseType::UINTEGER:
                    fieldSize = 4;
                    fieldAlignment = 4;
                    break;
                case BaseType::LONG:
                case BaseType::ULONG:
                    fieldSize = 8;
                    fieldAlignment = 8;
                    break;
                case BaseType::SINGLE:
                    fieldSize = 4;
                    fieldAlignment = 4;
                    break;
                case BaseType::DOUBLE:
                    fieldSize = 8;
                    fieldAlignment = 8;
                    break;
                case BaseType::STRING:
                case BaseType::UNICODE:
                case BaseType::POINTER:
                    fieldSize = 8;  // Pointer
                    fieldAlignment = 8;
                    break;
                default:
                    fieldSize = 8;
                    fieldAlignment = 8;
                    break;
            }
        } else {
            fieldSize = calculateTypeSize(field.typeName);
            fieldAlignment = 8;
        }
        
        // Apply alignment padding before this field
        if (currentOffset % fieldAlignment != 0) {
            currentOffset += fieldAlignment - (currentOffset % fieldAlignment);
        }
        
        // Cache this field's offset
        m_fieldOffsets[typeName][field.name] = currentOffset;
        
        // If this is the field we're looking for, return it
        if (field.name == fieldName) {
            return currentOffset;
        }
        
        // Move to next field
        currentOffset += fieldSize;
    }
    
    // Field not found
    return 0;
}

size_t QBECodeGenerator::getFieldOffset(const std::string& typeName, const std::vector<std::string>& memberChain) {
    if (memberChain.empty()) {
        return 0;
    }
    
    size_t totalOffset = 0;
    std::string currentTypeName = typeName;
    
    for (const auto& memberName : memberChain) {
        // Get offset of this member in current type
        size_t memberOffset = calculateFieldOffset(currentTypeName, memberName);
        totalOffset += memberOffset;
        
        // Get the field to determine if it's a nested type
        const TypeSymbol* typeSymbol = getTypeSymbol(currentTypeName);
        if (typeSymbol) {
            const TypeSymbol::Field* field = typeSymbol->findField(memberName);
            if (field && !field->isBuiltIn) {
                // This is a nested type, continue with it
                currentTypeName = field->typeName;
            }
        }
    }
    
    return totalOffset;
}

std::string QBECodeGenerator::inferMemberAccessType(const Expression* expr) {
    if (!expr) return "";
    
    ASTNodeType nodeType = expr->getType();
    
    if (nodeType == ASTNodeType::EXPR_VARIABLE) {
        // Base variable - look up its type
        const VariableExpression* varExpr = static_cast<const VariableExpression*>(expr);
        return getVariableTypeName(varExpr->name);
    } else if (nodeType == ASTNodeType::EXPR_MEMBER_ACCESS) {
        // Nested member access - recursively resolve
        const MemberAccessExpression* memberExpr = static_cast<const MemberAccessExpression*>(expr);
        std::string baseTypeName = inferMemberAccessType(memberExpr->object.get());
        
        if (baseTypeName.empty()) {
            return "";
        }
        
        // Look up the member in the base type
        const TypeSymbol* typeSymbol = getTypeSymbol(baseTypeName);
        if (typeSymbol) {
            const TypeSymbol::Field* field = typeSymbol->findField(memberExpr->memberName);
            if (field) {
                if (field->isBuiltIn) {
                    // Built-in type, no further nesting
                    return "";
                } else {
                    // Return the nested type name
                    return field->typeName;
                }
            }
        }
    } else if (nodeType == ASTNodeType::EXPR_ARRAY_ACCESS) {
        // Array element - if it's an array of UDTs, return the UDT type
        const ArrayAccessExpression* arrayExpr = static_cast<const ArrayAccessExpression*>(expr);
        
        if (m_symbols) {
            auto it = m_symbols->arrays.find(arrayExpr->name);
            if (it != m_symbols->arrays.end()) {
                if (it->second.type == VariableType::USER_DEFINED && !it->second.asTypeName.empty()) {
                    return it->second.asTypeName;
                }
            }
        }
    }
    
    return "";
}

std::string QBECodeGenerator::getVariableTypeName(const std::string& varName) {
    // Check cache first
    auto it = m_varTypeNames.find(varName);
    if (it != m_varTypeNames.end()) {
        return it->second;
    }
    
    // Look up in symbol table
    if (m_symbols) {
        auto varIt = m_symbols->variables.find(varName);
        if (varIt != m_symbols->variables.end()) {
            if (varIt->second.type == VariableType::USER_DEFINED) {
                m_varTypeNames[varName] = varIt->second.typeName;
                return varIt->second.typeName;
            }
        }
    }
    
    return "";
}

const TypeSymbol* QBECodeGenerator::getTypeSymbol(const std::string& typeName) {
    if (!m_symbols) {
        return nullptr;
    }
    
    auto it = m_symbols->types.find(typeName);
    if (it != m_symbols->types.end()) {
        return &it->second;
    }
    
    return nullptr;
}

} // namespace FasterBASIC