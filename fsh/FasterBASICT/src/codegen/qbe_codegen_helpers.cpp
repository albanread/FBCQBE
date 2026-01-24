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
    return "%t" + std::to_string(m_tempCounter++);
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

// =============================================================================
// Type System - QBE Type Mapping
// =============================================================================

std::string QBECodeGenerator::getQBEType(VariableType type) {
    switch (type) {
        case VariableType::INT:
            return "w";  // word (32-bit)
        case VariableType::FLOAT:
            return "d";  // QBE doesn't support single precision, use double
        case VariableType::DOUBLE:
            return "d";  // double precision float
        case VariableType::STRING:
            return "l";  // long (64-bit pointer)
        case VariableType::UNICODE:
            return "l";  // long (64-bit pointer)
        default:
            return "w";  // default to word
    }
}

std::string QBECodeGenerator::getQBETypeFromSuffix(char suffix) {
    switch (suffix) {
        case '%': return "w";  // INTEGER
        case '!': return "d";  // SINGLE (QBE doesn't support single, use double)
        case '#': return "d";  // DOUBLE
        case '$': return "l";  // STRING (pointer)
        case '&': return "l";  // LONG (64-bit int in QBE is 'l')
        default: return "w";   // Default to INTEGER
    }
}

// =============================================================================
// Type System - Type Suffix Detection
// =============================================================================

char QBECodeGenerator::getTypeSuffix(const std::string& varName) {
    if (varName.empty()) return '%';
    char last = varName.back();
    if (last == '%' || last == '!' || last == '#' || last == '$' || last == '&') {
        return last;
    }
    return '%';  // Default to integer
}

VariableType QBECodeGenerator::getVariableType(const std::string& varName) {
    // FOR loop indices are ALWAYS integers (hard rule in BASIC)
    if (m_forLoopVariables.count(varName) > 0) {
        return VariableType::INT;
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
        default: return VariableType::INT;   // Default in BASIC
    }
}

// =============================================================================
// Variable and Array References
// =============================================================================

std::string QBECodeGenerator::getVariableRef(const std::string& varName) {
    // Check if we're in a function and if this is a parameter
    if (m_inFunction && m_cfg) {
        // Check if varName is a parameter of the current function
        for (const auto& param : m_cfg->parameters) {
            if (param == varName) {
                // This is a parameter - use it directly without var_ prefix
                return "%" + varName;
            }
        }
        
        // Check if this is a local variable
        if (m_localVariables.count(varName) > 0) {
            // Local variable - use %local_ prefix
            return "%local_" + varName;
        }
        
        // Check if this is explicitly shared (or default to shared if not local)
        // In functions, variables are local unless declared SHARED
        if (m_sharedVariables.count(varName) > 0) {
            // Shared (global) variable - use %var_ prefix
            return "%var_" + varName;
        }
        
        // Default: if not local and not shared, treat as shared for backward compatibility
        // (In proper BASIC, undeclared vars in functions should error, but we're lenient)
        return "%var_" + varName;
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
    return "%var_" + varName;
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

void QBECodeGenerator::pushLoop(const std::string& exitLabel, 
                               const std::string& continueLabel,
                               const std::string& type) {
    m_loopStack.push_back({exitLabel, continueLabel, type});
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
    if (!expr) return VariableType::INT;
    
    ASTNodeType nodeType = expr->getType();
    
    switch (nodeType) {
        case ASTNodeType::EXPR_NUMBER: {
            const NumberExpression* numExpr = static_cast<const NumberExpression*>(expr);
            double value = numExpr->value;
            if (value == static_cast<int>(value)) {
                return VariableType::INT;
            }
            return VariableType::DOUBLE;
        }
        
        case ASTNodeType::EXPR_STRING:
            return VariableType::STRING;
        
        case ASTNodeType::EXPR_VARIABLE: {
            const VariableExpression* varExpr = static_cast<const VariableExpression*>(expr);
            return getVariableType(varExpr->name);
        }
        
        case ASTNodeType::EXPR_BINARY: {
            const BinaryExpression* binExpr = static_cast<const BinaryExpression*>(expr);
            VariableType leftType = inferExpressionType(binExpr->left.get());
            VariableType rightType = inferExpressionType(binExpr->right.get());
            
            // Type promotion rules
            if (leftType == VariableType::DOUBLE || rightType == VariableType::DOUBLE) {
                return VariableType::DOUBLE;
            }
            if (leftType == VariableType::FLOAT || rightType == VariableType::FLOAT) {
                return VariableType::FLOAT;
            }
            return VariableType::INT;
        }
        
        default:
            return VariableType::INT;
    }
}

std::string QBECodeGenerator::promoteToType(const std::string& value, 
                                           VariableType fromType, 
                                           VariableType toType) {
    // If types match, no promotion needed
    if (fromType == toType) return value;
    
    // Promote INT to DOUBLE
    if (fromType == VariableType::INT && toType == VariableType::DOUBLE) {
        return emitIntToDouble(value);
    }
    
    // Demote DOUBLE to INT
    if (fromType == VariableType::DOUBLE && toType == VariableType::INT) {
        return emitDoubleToInt(value);
    }
    
    // String conversions
    if (fromType == VariableType::INT && toType == VariableType::STRING) {
        return emitIntToString(value);
    }
    
    if (fromType == VariableType::DOUBLE && toType == VariableType::STRING) {
        return emitDoubleToString(value);
    }
    
    if (fromType == VariableType::STRING && toType == VariableType::INT) {
        return emitStringToInt(value);
    }
    
    if (fromType == VariableType::STRING && toType == VariableType::DOUBLE) {
        return emitStringToDouble(value);
    }
    
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

} // namespace FasterBASIC