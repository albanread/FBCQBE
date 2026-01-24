//
// qbe_codegen_expressions.cpp
// FasterBASIC QBE Code Generator - Expression Emission
//
// This file contains all expression emission code:
// - emitExpression() dispatcher
// - Binary operations (arithmetic, comparison, logical)
// - Unary operations (negation, NOT)
// - Literals (numbers, strings)
// - Variables and array access
// - Function calls
//

#include "../fasterbasic_qbe_codegen.h"
#include <sstream>

namespace FasterBASIC {

// =============================================================================
// Expression Dispatcher
// =============================================================================

std::string QBECodeGenerator::emitExpression(const Expression* expr) {
    if (!expr) {
        std::string temp = allocTemp("w");
        emit("    " + temp + " =w copy 0\n");
        return temp;
    }
    
    // Dispatch based on AST node type
    ASTNodeType nodeType = expr->getType();
    
    switch (nodeType) {
        case ASTNodeType::EXPR_NUMBER:
            return emitNumberLiteral(static_cast<const NumberExpression*>(expr));
            
        case ASTNodeType::EXPR_STRING:
            return emitStringLiteral(static_cast<const StringExpression*>(expr));
            
        case ASTNodeType::EXPR_VARIABLE:
            return emitVariableRef(static_cast<const VariableExpression*>(expr));
            
        case ASTNodeType::EXPR_BINARY:
            return emitBinaryOp(static_cast<const BinaryExpression*>(expr));
            
        case ASTNodeType::EXPR_UNARY:
            return emitUnaryOp(static_cast<const UnaryExpression*>(expr));
            
        case ASTNodeType::EXPR_FUNCTION_CALL:
            return emitFunctionCall(static_cast<const FunctionCallExpression*>(expr));
            
        case ASTNodeType::EXPR_ARRAY_ACCESS:
            return emitArrayAccessExpr(static_cast<const ArrayAccessExpression*>(expr));
            
        default:
            emitComment("TODO: Unhandled expression type " + std::to_string(static_cast<int>(nodeType)));
            std::string temp = allocTemp("w");
            emit("    " + temp + " =w copy 0\n");
            m_stats.instructionsGenerated++;
            return temp;
    }
}

// =============================================================================
// Number Literal
// =============================================================================

std::string QBECodeGenerator::emitNumberLiteral(const NumberExpression* expr) {
    if (!expr) {
        std::string temp = allocTemp("w");
        emit("    " + temp + " =w copy 0\n");
        m_stats.instructionsGenerated++;
        return temp;
    }
    
    // Check if integer or floating point
    double value = expr->value;
    bool isInteger = (value == static_cast<int>(value));
    
    if (isInteger) {
        // Integer constant
        std::string temp = allocTemp("w");
        emit("    " + temp + " =w copy " + std::to_string(static_cast<int>(value)) + "\n");
        m_stats.instructionsGenerated++;
        return temp;
    } else {
        // Double constant
        std::string temp = allocTemp("d");
        std::ostringstream oss;
        oss << std::fixed << value;
        emit("    " + temp + " =d copy d_" + oss.str() + "\n");
        m_stats.instructionsGenerated++;
        return temp;
    }
}

// =============================================================================
// String Literal
// =============================================================================

std::string QBECodeGenerator::emitStringLiteral(const StringExpression* expr) {
    if (!expr) {
        std::string temp = allocTemp("l");
        emit("    " + temp + " =l copy 0\n");
        m_stats.instructionsGenerated++;
        return temp;
    }
    
    return emitStringConstant(expr->value);
}

// =============================================================================
// Variable Reference
// =============================================================================

std::string QBECodeGenerator::emitVariableRef(const VariableExpression* expr) {
    if (!expr) {
        std::string temp = allocTemp("w");
        emit("    " + temp + " =w copy 0\n");
        m_stats.instructionsGenerated++;
        return temp;
    }
    
    // Return the variable reference directly (QBE uses SSA)
    return getVariableRef(expr->name);
}

// =============================================================================
// Binary Operations
// =============================================================================

std::string QBECodeGenerator::emitBinaryOp(const BinaryExpression* expr) {
    if (!expr || !expr->left || !expr->right) {
        std::string temp = allocTemp("w");
        emit("    " + temp + " =w copy 0\n");
        m_stats.instructionsGenerated++;
        return temp;
    }
    
    std::string leftTemp = emitExpression(expr->left.get());
    std::string rightTemp = emitExpression(expr->right.get());
    
    // Determine result type (for now, assume word integers)
    // TODO: Handle type promotion for mixed int/double operations
    std::string resultTemp = allocTemp("w");
    
    // Map TokenType operator to QBE instruction
    TokenType op = expr->op;
    
    switch (op) {
        // Arithmetic operators
        case TokenType::PLUS:
            emit("    " + resultTemp + " =w add " + leftTemp + ", " + rightTemp + "\n");
            break;
            
        case TokenType::MINUS:
            emit("    " + resultTemp + " =w sub " + leftTemp + ", " + rightTemp + "\n");
            break;
            
        case TokenType::MULTIPLY:
            emit("    " + resultTemp + " =w mul " + leftTemp + ", " + rightTemp + "\n");
            break;
            
        case TokenType::DIVIDE:
            emit("    " + resultTemp + " =w div " + leftTemp + ", " + rightTemp + "\n");
            break;
            
        case TokenType::MOD:
            emit("    " + resultTemp + " =w rem " + leftTemp + ", " + rightTemp + "\n");
            break;
            
        // Comparison operators (return 0 or 1)
        case TokenType::EQUAL:
            emit("    " + resultTemp + " =w ceqw " + leftTemp + ", " + rightTemp + "\n");
            break;
            
        case TokenType::NOT_EQUAL:
            emit("    " + resultTemp + " =w cnew " + leftTemp + ", " + rightTemp + "\n");
            break;
            
        case TokenType::LESS_THAN:
            emit("    " + resultTemp + " =w csltw " + leftTemp + ", " + rightTemp + "\n");
            break;
            
        case TokenType::LESS_EQUAL:
            emit("    " + resultTemp + " =w cslew " + leftTemp + ", " + rightTemp + "\n");
            break;
            
        case TokenType::GREATER_THAN:
            emit("    " + resultTemp + " =w csgtw " + leftTemp + ", " + rightTemp + "\n");
            break;
            
        case TokenType::GREATER_EQUAL:
            emit("    " + resultTemp + " =w csgew " + leftTemp + ", " + rightTemp + "\n");
            break;
            
        // Logical operators
        case TokenType::AND:
            emit("    " + resultTemp + " =w and " + leftTemp + ", " + rightTemp + "\n");
            break;
            
        case TokenType::OR:
            emit("    " + resultTemp + " =w or " + leftTemp + ", " + rightTemp + "\n");
            break;
            
        case TokenType::XOR:
            emit("    " + resultTemp + " =w xor " + leftTemp + ", " + rightTemp + "\n");
            break;
            
        default:
            emitComment("Unknown operator: " + std::to_string(static_cast<int>(op)));
            emit("    " + resultTemp + " =w copy 0\n");
            break;
    }
    
    m_stats.instructionsGenerated++;
    return resultTemp;
}

// =============================================================================
// Unary Operations
// =============================================================================

std::string QBECodeGenerator::emitUnaryOp(const UnaryExpression* expr) {
    if (!expr || !expr->expr) {
        std::string temp = allocTemp("w");
        emit("    " + temp + " =w copy 0\n");
        m_stats.instructionsGenerated++;
        return temp;
    }
    
    std::string operandTemp = emitExpression(expr->expr.get());
    std::string resultTemp = allocTemp("w");
    
    TokenType op = expr->op;
    
    switch (op) {
        case TokenType::MINUS:
            // Negation: 0 - operand
            emit("    " + resultTemp + " =w sub 0, " + operandTemp + "\n");
            break;
            
        case TokenType::NOT:
            // Logical NOT: operand == 0 ? 1 : 0
            emit("    " + resultTemp + " =w ceq " + operandTemp + ", 0\n");
            break;
            
        case TokenType::PLUS:
            // Unary plus: just copy
            emit("    " + resultTemp + " =w copy " + operandTemp + "\n");
            break;
            
        default:
            emitComment("Unknown unary operator: " + std::to_string(static_cast<int>(op)));
            emit("    " + resultTemp + " =w copy " + operandTemp + "\n");
            break;
    }
    
    m_stats.instructionsGenerated++;
    return resultTemp;
}

// =============================================================================
// Function Calls
// =============================================================================

std::string QBECodeGenerator::emitFunctionCall(const FunctionCallExpression* expr) {
    if (!expr) {
        std::string temp = allocTemp("w");
        emit("    " + temp + " =w copy 0\n");
        m_stats.instructionsGenerated++;
        return temp;
    }
    
    // Evaluate arguments
    std::vector<std::string> argTemps;
    for (const auto& arg : expr->arguments) {
        argTemps.push_back(emitExpression(arg.get()));
    }
    
    std::string funcName = expr->name;
    
    // Check if this is a user-defined function
    bool isUserFunction = false;
    const ControlFlowGraph* funcCFG = nullptr;
    if (m_programCFG) {
        funcCFG = m_programCFG->getFunctionCFG(funcName);
        isUserFunction = (funcCFG != nullptr);
    }
    
    // Determine return type
    std::string returnType = "w";  // Default to word
    if (isUserFunction && funcCFG) {
        if (funcCFG->returnType == VariableType::DOUBLE || funcCFG->returnType == VariableType::FLOAT) {
            returnType = "d";
        } else if (funcCFG->returnType == VariableType::STRING) {
            returnType = "l";
        }
    }
    
    std::string resultTemp = allocTemp(returnType);
    
    if (isUserFunction) {
        // Call user-defined function
        emit("    " + resultTemp + " =" + returnType + " call $" + funcName + "(");
        
        for (size_t i = 0; i < argTemps.size(); ++i) {
            if (i > 0) emit(", ");
            // TODO: Use proper argument types from function signature
            emit("w " + argTemps[i]);
        }
        
        emit(")\n");
    } else {
        // Call runtime library function
        std::string runtimeFunc = mapToRuntimeFunction(funcName);
        
        // Build function call
        emit("    " + resultTemp + " =w call $" + runtimeFunc + "(");
        
        for (size_t i = 0; i < argTemps.size(); ++i) {
            if (i > 0) emit(", ");
            emit("w " + argTemps[i]);  // TODO: Use proper argument types
        }
        
        emit(")\n");
    }
    
    m_stats.instructionsGenerated++;
    return resultTemp;
}

// =============================================================================
// Array Access Expression
// =============================================================================

std::string QBECodeGenerator::emitArrayAccessExpr(const ArrayAccessExpression* expr) {
    if (!expr) {
        std::string temp = allocTemp("w");
        emit("    " + temp + " =w copy 0\n");
        m_stats.instructionsGenerated++;
        return temp;
    }
    
    // Evaluate indices
    std::vector<std::string> indexTemps;
    for (const auto& indexExpr : expr->indices) {
        indexTemps.push_back(emitExpression(indexExpr.get()));
    }
    
    // Get array pointer
    std::string arrayRef = getArrayRef(expr->name);
    
    // Call runtime array access function
    std::string resultTemp = allocTemp("w");
    emit("    " + resultTemp + " =w call $array_get(l " + arrayRef);
    
    for (const auto& indexTemp : indexTemps) {
        emit(", w " + indexTemp);
    }
    
    emit(")\n");
    
    m_stats.instructionsGenerated++;
    return resultTemp;
}

// =============================================================================
// Helper: Map BASIC Function to Runtime Function
// =============================================================================

std::string QBECodeGenerator::mapToRuntimeFunction(const std::string& basicFunc) {
    // Convert BASIC function names to runtime library names
    // TODO: Build comprehensive mapping table
    
    std::string upper = basicFunc;
    for (char& c : upper) {
        c = std::toupper(c);
    }
    
    // Common math functions
    if (upper == "ABS") return "basic_abs";
    if (upper == "SIN") return "basic_sin";
    if (upper == "COS") return "basic_cos";
    if (upper == "TAN") return "basic_tan";
    if (upper == "SQRT") return "basic_sqrt";
    if (upper == "INT") return "basic_int";
    if (upper == "RND") return "basic_rnd";
    if (upper == "SGN") return "basic_sgn";
    
    // String functions
    if (upper == "LEN") return "str_length";
    if (upper == "LEFT$") return "str_left";
    if (upper == "RIGHT$") return "str_right";
    if (upper == "MID$") return "str_mid";
    if (upper == "CHR$") return "str_chr";
    if (upper == "ASC") return "str_asc";
    if (upper == "STR$") return "int_to_str";
    if (upper == "VAL") return "str_to_int";
    
    // Default: prefix with basic_
    return "basic_" + upper;
}

} // namespace FasterBASIC