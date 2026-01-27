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
#include <cmath>
#include <algorithm>
#include <unordered_set>

namespace FasterBASIC {

// =============================================================================
// Constant Folding Helpers
// =============================================================================

// Check if expression is a number literal and return its value
bool QBECodeGenerator::isNumberLiteral(const Expression* expr, double& value) {
    if (!expr || expr->getType() != ASTNodeType::EXPR_NUMBER) {
        return false;
    }
    const NumberExpression* numExpr = static_cast<const NumberExpression*>(expr);
    value = numExpr->value;
    return true;
}

// Check if two expressions are both number literals
bool QBECodeGenerator::areNumberLiterals(const Expression* expr1, const Expression* expr2, double& val1, double& val2) {
    return isNumberLiteral(expr1, val1) && isNumberLiteral(expr2, val2);
}

// Emit a constant integer value
std::string QBECodeGenerator::emitIntConstant(int64_t value) {
    std::string temp = allocTemp("w");
    emit("    " + temp + " =w copy " + std::to_string(value) + "\n");
    m_stats.instructionsGenerated++;
    return temp;
}

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
            
        case ASTNodeType::EXPR_MEMBER_ACCESS:
            return emitMemberAccessExpr(static_cast<const MemberAccessExpression*>(expr));
            
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
    
    // All numeric literals default to DOUBLE (like Lua)
    // Only use INT when explicitly marked with % suffix or in integer contexts
    double value = expr->value;
    
    // Always emit as double - simplifies type system
    std::string temp = allocTemp("d");
    std::ostringstream oss;
    oss << std::fixed << value;
    emit("    " + temp + " =d copy d_" + oss.str() + "\n");
    m_stats.instructionsGenerated++;
    return temp;
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
    
    // Check if this is a constant (compile-time value)
    // Constants are stored in lowercase for case-insensitive lookup
    if (m_symbols) {
        std::string lowerName = expr->name;
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
        auto it = m_symbols->constants.find(lowerName);
        if (it != m_symbols->constants.end()) {
            // This is a constant - emit its literal value
            const ConstantSymbol& constSym = it->second;
            
            if (constSym.type == ConstantSymbol::Type::INTEGER) {
                std::string temp = allocTemp("w");
                emit("    " + temp + " =w copy " + std::to_string(constSym.intValue) + "\n");
                m_stats.instructionsGenerated++;
                return temp;
            } else if (constSym.type == ConstantSymbol::Type::DOUBLE) {
                std::string temp = allocTemp("d");
                std::ostringstream oss;
                oss << std::fixed << constSym.doubleValue;
                emit("    " + temp + " =d copy d_" + oss.str() + "\n");
                m_stats.instructionsGenerated++;
                return temp;
            } else if (constSym.type == ConstantSymbol::Type::STRING) {
                // String constant - use string literal mechanism
                return emitStringConstant(constSym.stringValue);
            }
        }
    }
    
    // Not a constant - choose variable or array reference
    if (m_symbols) {
        auto arrIt = m_symbols->arrays.find(expr->name);
        if (arrIt != m_symbols->arrays.end()) {
            return getArrayRef(expr->name);
        }
    }

    return getVariableRef(expr->name);
}

// =============================================================================
// Binary Operations
// =============================================================================

std::string QBECodeGenerator::emitBinaryOp(const BinaryExpression* expr) {
    if (!expr || !expr->left || !expr->right) {
        std::string temp = allocTemp("d");
        emit("    " + temp + " =d copy d_0.0\n");
        m_stats.instructionsGenerated++;
        return temp;
    }
    
    // Evaluate operands
    std::string leftTemp = emitExpression(expr->left.get());
    std::string rightTemp = emitExpression(expr->right.get());
    
    // Infer operand types
    VariableType leftType = inferExpressionType(expr->left.get());
    VariableType rightType = inferExpressionType(expr->right.get());
    
    TokenType op = expr->op;
    
    // Special handling for string concatenation
    if (op == TokenType::PLUS && leftType == VariableType::STRING && rightType == VariableType::STRING) {
        return emitStringConcat(leftTemp, rightTemp);
    }
    
    // MOD operation REQUIRES integers
    // AND/OR/XOR work on both booleans (w) and integers (l), so don't force type
    bool requiresInteger = (op == TokenType::MOD);
    
    // Determine operation type
    VariableType opType;
    if (requiresInteger) {
        opType = VariableType::INT;
    } else if (leftType == VariableType::DOUBLE || rightType == VariableType::DOUBLE) {
        opType = VariableType::DOUBLE;  // DOUBLE is default, promotes from INT
    } else if (leftType == VariableType::FLOAT || rightType == VariableType::FLOAT) {
        opType = VariableType::DOUBLE;  // FLOAT maps to DOUBLE in QBE
    } else if (leftType == VariableType::INT && rightType == VariableType::INT) {
        opType = VariableType::INT;     // Both INT → INT result
    } else {
        opType = VariableType::DOUBLE;  // Default to DOUBLE
    }
    
    // Promote operands to operation type
    if (leftType != opType) {
        leftTemp = promoteToType(leftTemp, leftType, opType);
    }
    if (rightType != opType) {
        rightTemp = promoteToType(rightTemp, rightType, opType);
    }
    
    // Get QBE type suffix
    std::string qbeType = getQBEType(opType);
    std::string typeSuffix = (opType == VariableType::INT) ? "l" : "d";  // INT is now 64-bit long
    
    std::string resultTemp = allocTemp(qbeType);
    
    switch (op) {
        // Arithmetic operators
        case TokenType::PLUS:
            emit("    " + resultTemp + " =" + typeSuffix + " add " + leftTemp + ", " + rightTemp + "\n");
            break;
            
        case TokenType::MINUS:
            emit("    " + resultTemp + " =" + typeSuffix + " sub " + leftTemp + ", " + rightTemp + "\n");
            break;
            
        case TokenType::MULTIPLY:
            emit("    " + resultTemp + " =" + typeSuffix + " mul " + leftTemp + ", " + rightTemp + "\n");
            break;
            
        case TokenType::DIVIDE:
            emit("    " + resultTemp + " =" + typeSuffix + " div " + leftTemp + ", " + rightTemp + "\n");
            break;
            
        case TokenType::MOD:
            // MOD is integer-only operation (64-bit)
            emit("    " + resultTemp + " =l rem " + leftTemp + ", " + rightTemp + "\n");
            break;
            
        // Comparison operators (return integer 0 or 1)
        case TokenType::EQUAL:
            resultTemp = allocTemp("w");  // Comparisons return integer
            emit("    " + resultTemp + " =w ceq" + typeSuffix + " " + leftTemp + ", " + rightTemp + "\n");
            break;
            
        case TokenType::NOT_EQUAL:
            resultTemp = allocTemp("w");
            emit("    " + resultTemp + " =w cne" + typeSuffix + " " + leftTemp + ", " + rightTemp + "\n");
            break;
            
        case TokenType::LESS_THAN:
            resultTemp = allocTemp("w");
            // For integers use 'cslt', for floats use 'clt'
            // INT is 64-bit long ('l'), so use csltl for integers
            emit("    " + resultTemp + " =w c" + (typeSuffix == "l" ? "s" : "") + "lt" + typeSuffix + " " + leftTemp + ", " + rightTemp + "\n");
            break;
            
        case TokenType::LESS_EQUAL:
            resultTemp = allocTemp("w");
            // For integers use 'csle', for floats use 'cle'
            emit("    " + resultTemp + " =w c" + (typeSuffix == "l" ? "s" : "") + "le" + typeSuffix + " " + leftTemp + ", " + rightTemp + "\n");
            break;
            
        case TokenType::GREATER_THAN:
            resultTemp = allocTemp("w");
            // For integers use 'csgt', for floats use 'cgt'
            emit("    " + resultTemp + " =w c" + (typeSuffix == "l" ? "s" : "") + "gt" + typeSuffix + " " + leftTemp + ", " + rightTemp + "\n");
            break;
            
        case TokenType::GREATER_EQUAL:
            resultTemp = allocTemp("w");
            // For integers use 'csge', for floats use 'cge'
            emit("    " + resultTemp + " =w c" + (typeSuffix == "l" ? "s" : "") + "ge" + typeSuffix + " " + leftTemp + ", " + rightTemp + "\n");
            break;
            
        // Logical/Bitwise operators - work on both w (comparisons) and l (integers)
        case TokenType::AND:
        case TokenType::OR:
        case TokenType::XOR: {
            // Get actual QBE types of operands (w for comparisons, l for integers)
            std::string leftQBEType = getActualQBEType(expr->left.get());
            std::string rightQBEType = getActualQBEType(expr->right.get());
            
            // Determine operation type - promote w to l if mixed
            std::string opQBEType;
            if (leftQBEType == "l" || rightQBEType == "l") {
                opQBEType = "l";  // Use l if either operand is l
                // Extend w operands to l if needed
                if (leftQBEType == "w") {
                    std::string extendedLeft = allocTemp("l");
                    emit("    " + extendedLeft + " =l extsw " + leftTemp + "\n");
                    m_stats.instructionsGenerated++;
                    leftTemp = extendedLeft;
                }
                if (rightQBEType == "w") {
                    std::string extendedRight = allocTemp("l");
                    emit("    " + extendedRight + " =l extsw " + rightTemp + "\n");
                    m_stats.instructionsGenerated++;
                    rightTemp = extendedRight;
                }
            } else {
                opQBEType = "w";  // Both are w, use w
            }
            
            // Allocate result temp with correct type
            resultTemp = allocTemp(opQBEType);
            
            // Emit the operation with correct type
            const char* opName = (op == TokenType::AND) ? "and" : 
                                 (op == TokenType::OR) ? "or" : "xor";
            emit("    " + resultTemp + " =" + opQBEType + " " + opName + " " + leftTemp + ", " + rightTemp + "\n");
            break;
        }
            
        default:
            emitComment("Unknown operator: " + std::to_string(static_cast<int>(op)));
            emit("    " + resultTemp + " =" + typeSuffix + " copy " + (typeSuffix == "d" ? "d_0.0" : "0") + "\n");
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
        std::string temp = allocTemp("d");
        emit("    " + temp + " =d copy d_0.0\n");
        m_stats.instructionsGenerated++;
        return temp;
    }
    
    std::string operandTemp = emitExpression(expr->expr.get());
    VariableType operandType = inferExpressionType(expr->expr.get());
    
    TokenType op = expr->op;
    
    // Determine result type
    std::string qbeType = getQBEType(operandType);
    std::string typeSuffix = (operandType == VariableType::INT) ? "w" : "d";
    std::string resultTemp = allocTemp(qbeType);
    
    switch (op) {
        case TokenType::MINUS:
            // Negation: 0 - operand
            if (operandType == VariableType::INT) {
                emit("    " + resultTemp + " =w sub 0, " + operandTemp + "\n");
            } else {
                emit("    " + resultTemp + " =d sub d_0.0, " + operandTemp + "\n");
            }
            break;
            
        case TokenType::NOT:
            // Logical NOT: operand == 0 ? 1 : 0
            // Always returns integer
            resultTemp = allocTemp("w");
            if (operandType == VariableType::INT) {
                emit("    " + resultTemp + " =w ceqw " + operandTemp + ", 0\n");
            } else {
                emit("    " + resultTemp + " =w ceqd " + operandTemp + ", d_0.0\n");
            }
            break;
            
        case TokenType::PLUS:
            // Unary plus: just copy
            emit("    " + resultTemp + " =" + typeSuffix + " copy " + operandTemp + "\n");
            break;
            
        default:
            emitComment("Unknown unary operator: " + std::to_string(static_cast<int>(op)));
            emit("    " + resultTemp + " =" + typeSuffix + " copy " + operandTemp + "\n");
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
    
    std::string funcName = expr->name;
    std::string upper = funcName;
    for (char& c : upper) c = std::toupper(c);
    
    // Special case: LEN() - just load the length field from StringDescriptor (offset 8)
    if (upper == "LEN" && expr->arguments.size() == 1) {
        std::string strTemp = emitExpression(expr->arguments[0].get());
        std::string lenPtr = allocTemp("l");
        std::string result = allocTemp("l");
        emit("    " + lenPtr + " =l add " + strTemp + ", 8\n");  // offset to length field
        emit("    " + result + " =l loadl " + lenPtr + "\n");
        m_stats.instructionsGenerated += 2;
        return result;
    }
    
    // Special case: STRTYPE() - load encoding type from StringDescriptor (offset 28)
    // Returns: 0 = ASCII, 1 = UTF-32
    if (upper == "STRTYPE" && expr->arguments.size() == 1) {
        std::string strTemp = emitExpression(expr->arguments[0].get());
        std::string encPtr = allocTemp("l");
        std::string result = allocTemp("w");
        emit("    " + encPtr + " =l add " + strTemp + ", 28\n");  // offset to encoding field
        emit("    " + result + " =w loadub " + encPtr + "\n");     // load encoding byte (0 or 1)
        m_stats.instructionsGenerated += 2;
        return result;
    }
    
    // Special case: ASC() - load first code point from StringDescriptor
    // Need to check encoding type: ASCII (offset 28) = 0 uses loadub, UTF-32 = 1 uses loaduw
    if (upper == "ASC" && expr->arguments.size() == 1) {
        std::string strTemp = emitExpression(expr->arguments[0].get());
        
        // Check if string is NULL or empty - need to check length first
        std::string lenPtr = allocTemp("l");
        std::string len = allocTemp("l");
        emit("    " + lenPtr + " =l add " + strTemp + ", 8\n");  // offset to length field
        emit("    " + len + " =l loadl " + lenPtr + "\n");
        
        // Check if length > 0
        std::string hasChars = allocTemp("w");
        emit("    " + hasChars + " =w csgtl " + len + ", 0\n");  // signed greater than
        
        std::string validLabel = allocLabel();
        std::string emptyLabel = allocLabel();
        std::string endLabel = allocLabel();
        
        emit("    jnz " + hasChars + ", @" + validLabel + ", @" + emptyLabel + "\n");
        m_stats.instructionsGenerated += 4;
        
        // Valid string - check encoding type and load first character
        emit("@" + validLabel + "\n");
        
        // Load encoding byte at offset 28
        std::string encodingPtr = allocTemp("l");
        std::string encoding = allocTemp("w");
        emit("    " + encodingPtr + " =l add " + strTemp + ", 28\n");
        emit("    " + encoding + " =w loadub " + encodingPtr + "\n");  // load encoding byte
        
        // Load data pointer at offset 0
        std::string dataPtr = allocTemp("l");
        emit("    " + dataPtr + " =l loadl " + strTemp + "\n");
        
        // Check encoding: 0=ASCII, 1=UTF-32
        std::string isASCII = allocTemp("w");
        emit("    " + isASCII + " =w ceqw " + encoding + ", 0\n");
        
        std::string asciiLabel = allocLabel();
        std::string utf32Label = allocLabel();
        
        emit("    jnz " + isASCII + ", @" + asciiLabel + ", @" + utf32Label + "\n");
        m_stats.instructionsGenerated += 6;
        
        // ASCII path - load 1 byte
        emit("@" + asciiLabel + "\n");
        std::string asciiChar = allocTemp("w");
        emit("    " + asciiChar + " =w loadub " + dataPtr + "\n");
        std::string result = allocTemp("w");
        emit("    " + result + " =w copy " + asciiChar + "\n");
        emit("    jmp @" + endLabel + "\n");
        m_stats.instructionsGenerated += 3;
        
        // UTF-32 path - load 4 bytes
        emit("@" + utf32Label + "\n");
        std::string utf32Char = allocTemp("w");
        emit("    " + utf32Char + " =w loaduw " + dataPtr + "\n");
        emit("    " + result + " =w copy " + utf32Char + "\n");
        emit("    jmp @" + endLabel + "\n");
        m_stats.instructionsGenerated += 3;
        
        // Empty string - return 0
        emit("@" + emptyLabel + "\n");
        emit("    " + result + " =w copy 0\n");
        m_stats.instructionsGenerated++;
        
        emit("@" + endLabel + "\n");
        return result;
    }
    
    // Special case: INKEY$ - Non-blocking keyboard input
    if (upper == "INKEY" || upper == "INKEY$") {
        emitComment("INKEY$ - Non-blocking keyboard input");
        std::string result = allocTemp("l");
        emit("    " + result + " =l call $basic_inkey()\n");
        m_stats.instructionsGenerated++;
        return result;
    }
    
    // Special case: CSRLIN - Get current cursor row
    if (upper == "CSRLIN") {
        emitComment("CSRLIN - Get cursor row");
        std::string result = allocTemp("w");
        emit("    " + result + " =w call $basic_csrlin()\n");
        m_stats.instructionsGenerated++;
        return result;
    }
    
    // Special case: POS - Get current cursor column
    if (upper == "POS" && expr->arguments.size() == 1) {
        emitComment("POS - Get cursor column");
        // The argument is always 0 (dummy parameter), evaluate it but ignore result
        emitExpression(expr->arguments[0].get());
        std::string result = allocTemp("w");
        emit("    " + result + " =w call $basic_pos(w 0)\n");
        m_stats.instructionsGenerated++;
        return result;
    }
    
    // Special case: String slicing s$(start TO end) - converted to __string_slice
    if (upper == "__STRING_SLICE" && expr->arguments.size() == 3) {
        std::string strTemp = emitExpression(expr->arguments[0].get());
        std::string startTemp = emitExpression(expr->arguments[1].get());
        std::string endTemp = emitExpression(expr->arguments[2].get());
        
        // Ensure start and end are int64_t (l type)
        VariableType startType = inferExpressionType(expr->arguments[1].get());
        if (startType == VariableType::DOUBLE) {
            std::string startInt = allocTemp("w");
            emit("    " + startInt + " =w dtosi " + startTemp + "\n");
            std::string startLong = allocTemp("l");
            emit("    " + startLong + " =l extsw " + startInt + "\n");
            startTemp = startLong;
        } else if (startType == VariableType::INT) {
            std::string startLong = allocTemp("l");
            emit("    " + startLong + " =l extsw " + startTemp + "\n");
            startTemp = startLong;
        }
        
        VariableType endType = inferExpressionType(expr->arguments[2].get());
        if (endType == VariableType::DOUBLE) {
            std::string endInt = allocTemp("w");
            emit("    " + endInt + " =w dtosi " + endTemp + "\n");
            std::string endLong = allocTemp("l");
            emit("    " + endLong + " =l extsw " + endInt + "\n");
            endTemp = endLong;
        } else if (endType == VariableType::INT) {
            std::string endLong = allocTemp("l");
            emit("    " + endLong + " =l extsw " + endTemp + "\n");
            endTemp = endLong;
        }
        
        std::string result = allocTemp("l");
        emit("    " + result + " =l call $string_slice(l " + strTemp + ", l " + startTemp + ", l " + endTemp + ")\n");
        m_stats.instructionsGenerated++;
        return result;
    }
    
    // Special case: MID$(str, start, length) - substring extraction
    if ((upper == "MID" || upper == "MID$") && expr->arguments.size() == 3) {
        std::string strTemp = emitExpression(expr->arguments[0].get());
        std::string startTemp = emitExpression(expr->arguments[1].get());
        std::string lenTemp = emitExpression(expr->arguments[2].get());
        
        // Ensure start and length are int64_t (l type)
        VariableType startType = inferExpressionType(expr->arguments[1].get());
        if (startType == VariableType::DOUBLE) {
            // Convert double to int64_t
            std::string startInt = allocTemp("w");
            emit("    " + startInt + " =w dtosi " + startTemp + "\n");
            std::string startLong = allocTemp("l");
            emit("    " + startLong + " =l extsw " + startInt + "\n");
            startTemp = startLong;
        } else if (startType == VariableType::INT) {
            std::string startLong = allocTemp("l");
            emit("    " + startLong + " =l extsw " + startTemp + "\n");
            startTemp = startLong;
        }
        
        VariableType lenType = inferExpressionType(expr->arguments[2].get());
        if (lenType == VariableType::DOUBLE) {
            // Convert double to int64_t
            std::string lenInt = allocTemp("w");
            emit("    " + lenInt + " =w dtosi " + lenTemp + "\n");
            std::string lenLong = allocTemp("l");
            emit("    " + lenLong + " =l extsw " + lenInt + "\n");
            lenTemp = lenLong;
        } else if (lenType == VariableType::INT) {
            std::string lenLong = allocTemp("l");
            emit("    " + lenLong + " =l extsw " + lenTemp + "\n");
            lenTemp = lenLong;
        }
        
        std::string result = allocTemp("l");
        emit("    " + result + " =l call $string_mid(l " + strTemp + ", l " + startTemp + ", l " + lenTemp + ")\n");
        m_stats.instructionsGenerated++;
        return result;
    }
    
    // Special case: LEFT$(str, count) - left substring
    if ((upper == "LEFT" || upper == "LEFT$") && expr->arguments.size() == 2) {
        std::string strTemp = emitExpression(expr->arguments[0].get());
        std::string countTemp = emitExpression(expr->arguments[1].get());
        
        // Ensure count is int64_t
        VariableType countType = inferExpressionType(expr->arguments[1].get());
        if (countType == VariableType::DOUBLE) {
            // Convert double to int64_t
            std::string countInt = allocTemp("w");
            emit("    " + countInt + " =w dtosi " + countTemp + "\n");
            std::string countLong = allocTemp("l");
            emit("    " + countLong + " =l extsw " + countInt + "\n");
            countTemp = countLong;
        } else if (countType == VariableType::INT) {
            std::string countLong = allocTemp("l");
            emit("    " + countLong + " =l extsw " + countTemp + "\n");
            countTemp = countLong;
        }
        
        std::string result = allocTemp("l");
        emit("    " + result + " =l call $string_left(l " + strTemp + ", l " + countTemp + ")\n");
        m_stats.instructionsGenerated++;
        return result;
    }
    
    // Special case: RIGHT$(str, count) - right substring
    if ((upper == "RIGHT" || upper == "RIGHT$") && expr->arguments.size() == 2) {
        std::string strTemp = emitExpression(expr->arguments[0].get());
        std::string countTemp = emitExpression(expr->arguments[1].get());
        
        // Ensure count is int64_t
        VariableType countType = inferExpressionType(expr->arguments[1].get());
        if (countType == VariableType::DOUBLE) {
            // Convert double to int64_t
            std::string countInt = allocTemp("w");
            emit("    " + countInt + " =w dtosi " + countTemp + "\n");
            std::string countLong = allocTemp("l");
            emit("    " + countLong + " =l extsw " + countInt + "\n");
            countTemp = countLong;
        } else if (countType == VariableType::INT) {
            std::string countLong = allocTemp("l");
            emit("    " + countLong + " =l extsw " + countTemp + "\n");
            countTemp = countLong;
        }
        
        std::string result = allocTemp("l");
        emit("    " + result + " =l call $string_right(l " + strTemp + ", l " + countTemp + ")\n");
        m_stats.instructionsGenerated++;
        return result;
    }
    
    // Special case: RAND(n) - convert argument to int32_t
    if (upper == "RAND" && expr->arguments.size() == 1) {
        std::string argTemp = emitExpression(expr->arguments[0].get());
        VariableType argType = inferExpressionType(expr->arguments[0].get());
        
        // Convert argument to int if it's not already
        if (argType == VariableType::DOUBLE || argType == VariableType::FLOAT) {
            std::string intArg = allocTemp("w");
            emit("    " + intArg + " =w dtosi " + argTemp + "\n");
            argTemp = intArg;
            m_stats.instructionsGenerated++;
        } else if (argType == VariableType::INT) {
            // Already int, use as-is
        } else {
            // For other types, convert to int
            std::string intArg = allocTemp("w");
            emit("    " + intArg + " =w copy " + argTemp + "\n");
            argTemp = intArg;
            m_stats.instructionsGenerated++;
        }
        
        std::string result = allocTemp("w");
        emit("    " + result + " =w call $basic_rand(w " + argTemp + ")\n");
        m_stats.instructionsGenerated++;
        return result;
    }
    
    // =========================================================================
    // INTRINSIC FUNCTIONS - Generate inline code instead of function calls
    // =========================================================================
    
    // FIX(d) - Truncate toward zero using dtosi
    if (upper == "FIX" && expr->arguments.size() == 1) {
        // Check for constant folding
        double constValue;
        if (isNumberLiteral(expr->arguments[0].get(), constValue)) {
            // Constant fold: truncate toward zero
            int64_t result = static_cast<int64_t>(constValue);
            return emitIntConstant(result);
        }
        
        std::string argTemp = emitExpression(expr->arguments[0].get());
        VariableType argType = inferExpressionType(expr->arguments[0].get());
        
        // Convert to double if needed
        if (argType == VariableType::INT) {
            std::string doubleArg = allocTemp("d");
            emit("    " + doubleArg + " =d swtof " + argTemp + "\n");
            argTemp = doubleArg;
            m_stats.instructionsGenerated++;
        }
        
        // Truncate toward zero using dtosi
        std::string result = allocTemp("w");
        emit("    " + result + " =w dtosi " + argTemp + "\n");
        m_stats.instructionsGenerated++;
        return result;
    }
    
    // CINT(d) - Round to nearest integer
    if (upper == "CINT" && expr->arguments.size() == 1) {
        // Check for constant folding
        double constValue;
        if (isNumberLiteral(expr->arguments[0].get(), constValue)) {
            // Constant fold: add 0.5 and truncate toward zero (matches dtosi behavior)
            int64_t result = static_cast<int64_t>(std::trunc(constValue + 0.5));
            return emitIntConstant(result);
        }
        
        std::string argTemp = emitExpression(expr->arguments[0].get());
        VariableType argType = inferExpressionType(expr->arguments[0].get());
        
        // Convert to double if needed
        if (argType == VariableType::INT) {
            std::string doubleArg = allocTemp("d");
            emit("    " + doubleArg + " =d swtof " + argTemp + "\n");
            argTemp = doubleArg;
            m_stats.instructionsGenerated++;
        }
        
        // Simple rounding: add 0.5 and truncate
        // This approximates banker's rounding for most cases
        std::string adjusted = allocTemp("d");
        emit("    " + adjusted + " =d add " + argTemp + ", d_0.5\n");
        
        // Truncate to int
        std::string result = allocTemp("w");
        emit("    " + result + " =w dtosi " + adjusted + "\n");
        m_stats.instructionsGenerated += 2;
        return result;
    }
    
    // ABS(x) - Absolute value
    if (upper == "ABS" && expr->arguments.size() == 1) {
        // Check for constant folding
        double constValue;
        if (isNumberLiteral(expr->arguments[0].get(), constValue)) {
            // Constant fold: absolute value, but keep as double to match the
            // runtime signature (avoids emitting w-typed temps fed into
            // basic_print_double).
            double absVal = std::abs(constValue);
            std::string temp = allocTemp("d");
            std::ostringstream oss;
            oss << std::fixed << absVal;
            emit("    " + temp + " =d copy d_" + oss.str() + "\n");
            m_stats.instructionsGenerated++;
            return temp;
        }
        
        std::string argTemp = emitExpression(expr->arguments[0].get());
        VariableType argType = inferExpressionType(expr->arguments[0].get());
        
        if (argType == VariableType::INT) {
            // For integers: if negative, negate using conditional branches
            // Check actual QBE type (w or l)
            std::string argQBEType = getActualQBEType(expr->arguments[0].get());
            if (argQBEType != "w" && argQBEType != "l") {
                argQBEType = "l";  // Default to long for INT
            }
            
            std::string isNeg = allocTemp("w");
            if (argQBEType == "l") {
                emit("    " + isNeg + " =w csltl " + argTemp + ", 0\n");
            } else {
                emit("    " + isNeg + " =w csltw " + argTemp + ", 0\n");
            }
            
            std::string negVal = allocTemp(argQBEType);
            emit("    " + negVal + " =" + argQBEType + " neg " + argTemp + "\n");
            
            // Use conditional branch pattern
            std::string thenLabel = allocLabel();
            std::string elseLabel = allocLabel();
            std::string endLabel = allocLabel();
            std::string result = allocTemp(argQBEType);
            
            emit("    jnz " + isNeg + ", @" + thenLabel + ", @" + elseLabel + "\n");
            emit("@" + thenLabel + "\n");
            emit("    " + result + " =" + argQBEType + " copy " + negVal + "\n");
            emit("    jmp @" + endLabel + "\n");
            emit("@" + elseLabel + "\n");
            emit("    " + result + " =" + argQBEType + " copy " + argTemp + "\n");
            emit("@" + endLabel + "\n");
            
            m_stats.instructionsGenerated += 8;
            m_stats.labelsGenerated += 3;
            return result;
        } else {
            // For doubles: use runtime function (fabs)
            // Could be made intrinsic with conditional logic but keep simple for now
        }
    }
    
    // SGN(x) - Sign function (-1, 0, or 1)
    if (upper == "SGN" && expr->arguments.size() == 1) {
        // Check for constant folding
        double constValue;
        if (isNumberLiteral(expr->arguments[0].get(), constValue)) {
            // Constant fold: sign function
            int64_t result = (constValue > 0) ? 1 : ((constValue < 0) ? -1 : 0);
            return emitIntConstant(result);
        }
        
        std::string argTemp = emitExpression(expr->arguments[0].get());
        VariableType argType = inferExpressionType(expr->arguments[0].get());
        
        if (argType == VariableType::INT) {
            // For integers: use comparisons
            std::string zero = allocTemp("w");
            emit("    " + zero + " =w copy 0\n");
            
            std::string isNeg = allocTemp("w");
            emit("    " + isNeg + " =w csltw " + argTemp + ", " + zero + "\n");
            
            std::string isPos = allocTemp("w");
            emit("    " + isPos + " =w csgtw " + argTemp + ", " + zero + "\n");
            
            std::string negOne = allocTemp("w");
            emit("    " + negOne + " =w copy -1\n");
            
            std::string posOne = allocTemp("w");
            emit("    " + posOne + " =w copy 1\n");
            
            // Use conditional branches: if negative return -1, else if positive return 1, else return 0
            std::string negLabel = allocLabel();
            std::string posLabel = allocLabel();
            std::string zeroLabel = allocLabel();
            std::string endLabel = allocLabel();
            std::string result = allocTemp("w");
            
            emit("    jnz " + isNeg + ", @" + negLabel + ", @" + posLabel + "\n");
            emit("@" + negLabel + "\n");
            emit("    " + result + " =w copy " + negOne + "\n");
            emit("    jmp @" + endLabel + "\n");
            emit("@" + posLabel + "\n");
            std::string finalResult = allocTemp("w");
            emit("    jnz " + isPos + ", @" + zeroLabel + ", @" + endLabel + "\n");
            emit("@" + zeroLabel + "\n");
            emit("    " + result + " =w copy " + posOne + "\n");
            emit("    jmp @" + endLabel + "\n");
            emit("@" + endLabel + "\n");
            if (finalResult.empty()) {
                emit("    " + result + " =w copy " + zero + "\n");
            }
            
            m_stats.instructionsGenerated += 13;
            m_stats.labelsGenerated += 4;
            return result;
        } else {
            // For doubles: use runtime function
        }
    }
    
    // MIN(a, b) - Minimum of two values
    if (upper == "MIN" && expr->arguments.size() == 2) {
        // Check for constant folding
        double val1, val2;
        if (areNumberLiterals(expr->arguments[0].get(), expr->arguments[1].get(), val1, val2)) {
            // Constant fold: minimum
            int64_t result = static_cast<int64_t>(std::min(val1, val2));
            return emitIntConstant(result);
        }
        
        std::string leftTemp = emitExpression(expr->arguments[0].get());
        std::string rightTemp = emitExpression(expr->arguments[1].get());
        VariableType leftType = inferExpressionType(expr->arguments[0].get());
        VariableType rightType = inferExpressionType(expr->arguments[1].get());
        
        // Ensure both operands are the same type
        if (leftType != rightType) {
            // Promote to double if types differ
            if (leftType == VariableType::INT) {
                std::string promoted = allocTemp("d");
                emit("    " + promoted + " =d swtof " + leftTemp + "\n");
                leftTemp = promoted;
                m_stats.instructionsGenerated++;
            }
            if (rightType == VariableType::INT) {
                std::string promoted = allocTemp("d");
                emit("    " + promoted + " =d swtof " + rightTemp + "\n");
                rightTemp = promoted;
                m_stats.instructionsGenerated++;
            }
        }
        
        if (leftType == VariableType::INT || (leftType == rightType && leftType == VariableType::INT)) {
            // Integer minimum using conditional branches
            std::string isLess = allocTemp("w");
            emit("    " + isLess + " =w csltw " + leftTemp + ", " + rightTemp + "\n");
            
            std::string thenLabel = allocLabel();
            std::string elseLabel = allocLabel();
            std::string endLabel = allocLabel();
            std::string result = allocTemp("w");
            
            emit("    jnz " + isLess + ", @" + thenLabel + ", @" + elseLabel + "\n");
            emit("@" + thenLabel + "\n");
            emit("    " + result + " =w copy " + leftTemp + "\n");
            emit("    jmp @" + endLabel + "\n");
            emit("@" + elseLabel + "\n");
            emit("    " + result + " =w copy " + rightTemp + "\n");
            emit("@" + endLabel + "\n");
            
            m_stats.instructionsGenerated += 8;
            m_stats.labelsGenerated += 3;
            return result;
        } else {
            // Double minimum using conditional branches
            std::string isLess = allocTemp("w");
            emit("    " + isLess + " =w cltd " + leftTemp + ", " + rightTemp + "\n");
            
            std::string thenLabel = allocLabel();
            std::string elseLabel = allocLabel();
            std::string endLabel = allocLabel();
            std::string result = allocTemp("d");
            
            emit("    jnz " + isLess + ", @" + thenLabel + ", @" + elseLabel + "\n");
            emit("@" + thenLabel + "\n");
            emit("    " + result + " =d copy " + leftTemp + "\n");
            emit("    jmp @" + endLabel + "\n");
            emit("@" + elseLabel + "\n");
            emit("    " + result + " =d copy " + rightTemp + "\n");
            emit("@" + endLabel + "\n");
            
            m_stats.instructionsGenerated += 8;
            m_stats.labelsGenerated += 3;
            return result;
        }
    }
    
    // MAX(a, b) - Maximum of two values
    if (upper == "MAX" && expr->arguments.size() == 2) {
        // Check for constant folding
        double val1, val2;
        if (areNumberLiterals(expr->arguments[0].get(), expr->arguments[1].get(), val1, val2)) {
            // Constant fold: maximum
            int64_t result = static_cast<int64_t>(std::max(val1, val2));
            return emitIntConstant(result);
        }
        
        std::string leftTemp = emitExpression(expr->arguments[0].get());
        std::string rightTemp = emitExpression(expr->arguments[1].get());
        VariableType leftType = inferExpressionType(expr->arguments[0].get());
        VariableType rightType = inferExpressionType(expr->arguments[1].get());
        
        // Ensure both operands are the same type
        if (leftType != rightType) {
            // Promote to double if types differ
            if (leftType == VariableType::INT) {
                std::string promoted = allocTemp("d");
                emit("    " + promoted + " =d swtof " + leftTemp + "\n");
                leftTemp = promoted;
                m_stats.instructionsGenerated++;
            }
            if (rightType == VariableType::INT) {
                std::string promoted = allocTemp("d");
                emit("    " + promoted + " =d swtof " + rightTemp + "\n");
                rightTemp = promoted;
                m_stats.instructionsGenerated++;
            }
        }
        
        if (leftType == VariableType::INT || (leftType == rightType && leftType == VariableType::INT)) {
            // Integer maximum using conditional branches
            std::string isGreater = allocTemp("w");
            emit("    " + isGreater + " =w csgtw " + leftTemp + ", " + rightTemp + "\n");
            
            std::string thenLabel = allocLabel();
            std::string elseLabel = allocLabel();
            std::string endLabel = allocLabel();
            std::string result = allocTemp("w");
            
            emit("    jnz " + isGreater + ", @" + thenLabel + ", @" + elseLabel + "\n");
            emit("@" + thenLabel + "\n");
            emit("    " + result + " =w copy " + leftTemp + "\n");
            emit("    jmp @" + endLabel + "\n");
            emit("@" + elseLabel + "\n");
            emit("    " + result + " =w copy " + rightTemp + "\n");
            emit("@" + endLabel + "\n");
            
            m_stats.instructionsGenerated += 8;
            m_stats.labelsGenerated += 3;
            return result;
        } else {
            // Double maximum using conditional branches
            std::string isGreater = allocTemp("w");
            emit("    " + isGreater + " =w cgtd " + leftTemp + ", " + rightTemp + "\n");
            
            std::string thenLabel = allocLabel();
            std::string elseLabel = allocLabel();
            std::string endLabel = allocLabel();
            std::string result = allocTemp("d");
            
            emit("    jnz " + isGreater + ", @" + thenLabel + ", @" + elseLabel + "\n");
            emit("@" + thenLabel + "\n");
            emit("    " + result + " =d copy " + leftTemp + "\n");
            emit("    jmp @" + endLabel + "\n");
            emit("@" + elseLabel + "\n");
            emit("    " + result + " =d copy " + rightTemp + "\n");
            emit("@" + endLabel + "\n");
            
            m_stats.instructionsGenerated += 8;
            m_stats.labelsGenerated += 3;
            return result;
        }
    }
    
    // Evaluate arguments (get raw temporaries)
    std::vector<std::string> argTemps;
    std::vector<VariableType> argTypes;
    for (const auto& arg : expr->arguments) {
        argTemps.push_back(emitExpression(arg.get()));
        argTypes.push_back(inferExpressionType(arg.get()));
    }
    
    // Check if this is a user-defined function
    bool isUserFunction = false;
    const ControlFlowGraph* funcCFG = nullptr;
    if (m_programCFG) {
        funcCFG = m_programCFG->getFunctionCFG(funcName);
        isUserFunction = (funcCFG != nullptr);
    }
    
    // Determine return type
    std::string returnType = "l";  // Default to long (64-bit INT)
    if (isUserFunction && funcCFG) {
        if (funcCFG->returnType == VariableType::DOUBLE || funcCFG->returnType == VariableType::FLOAT) {
            returnType = "d";
        } else if (funcCFG->returnType == VariableType::STRING) {
            returnType = "l";
        }
    } else {
        // Builtins ending with $ return pointers
        if (!upper.empty() && upper.back() == '$') {
            returnType = "l";
        }
        // Check for builtin string functions
        if (upper == "CHR$" || upper == "LEFT$" || upper == "RIGHT$" || 
            upper == "MID$" || upper == "__STRING_SLICE" ||
            upper == "STR$" || upper == "STRING$" || upper == "SPACE$" ||
            upper == "LTRIM$" || upper == "RTRIM$" || upper == "TRIM$" ||
            upper == "REPLACE$" || upper == "REVERSE$" || upper == "INSERT$" ||
            upper == "DELETE$" || upper == "REMOVE$" || upper == "EXTRACT$" ||
            upper == "LPAD$" || upper == "RPAD$" || upper == "CENTER$" ||
            upper == "STRREV$" || upper == "HEX$" || upper == "BIN$" ||
            upper == "OCT$" || upper == "JOIN$" || upper == "SPLIT$") {
            returnType = "l";  // String functions return pointers
        }
    }
    
    // Pre-convert arguments to match parameter types BEFORE emitting the call
    std::vector<std::string> convertedArgTemps;
    std::vector<std::string> convertedArgTypes;
    
    if (isUserFunction) {
        for (size_t i = 0; i < argTemps.size(); ++i) {
            std::string argTemp = argTemps[i];
            VariableType argType = argTypes[i];
            VariableType paramType = VariableType::DOUBLE;  // Default
            
            // Look up actual parameter type if available
            if (funcCFG && i < funcCFG->parameterTypes.size()) {
                paramType = funcCFG->parameterTypes[i];
            } else if (m_symbols) {
                // Try to find in symbol table
                auto it = m_symbols->functions.find(funcName);
                if (it != m_symbols->functions.end() && i < it->second.parameterTypes.size()) {
                    paramType = it->second.parameterTypes[i];
                }
            }
            
            // Coerce argument to match parameter type (this emits conversion instructions)
            if (argType != paramType) {
                argTemp = promoteToType(argTemp, argType, paramType);
            }
            
            convertedArgTemps.push_back(argTemp);
            convertedArgTypes.push_back(getQBEType(paramType));
        }
    } else {
        // For runtime functions, just use the argument types as-is
        for (size_t i = 0; i < argTemps.size(); ++i) {
            convertedArgTemps.push_back(argTemps[i]);
            convertedArgTypes.push_back(getQBEType(argTypes[i]));
        }
    }
    
    std::string resultTemp = allocTemp(returnType);
    
    if (isUserFunction) {
        // Call user-defined function with pre-converted arguments
        emit("    " + resultTemp + " =" + returnType + " call $" + funcName + "(");
        
        for (size_t i = 0; i < convertedArgTemps.size(); ++i) {
            if (i > 0) emit(", ");
            emit(convertedArgTypes[i] + " " + convertedArgTemps[i]);
        }
        
        emit(")\n");
    } else {
        // Call runtime library function
        std::string runtimeFunc = mapToRuntimeFunction(funcName);
        
        // Determine correct return type for runtime function
        std::string upper = funcName;
        for (char& c : upper) c = std::toupper(c);
        
        static const std::unordered_set<std::string> longReturn = {
            "LEN", "ASC", "INSTR", "INSTRREV", "TALLY"
        };

        static const std::unordered_set<std::string> stringReturn = {
            "CHR$", "LEFT$", "RIGHT$", "MID$", "STR$", "SPACE$", "STRING$",
            "UCASE$", "LCASE$", "TRIM$", "LTRIM$", "RTRIM$", "REPLACE$",
            "REVERSE$", "INSERT$", "DELETE$", "REMOVE$", "EXTRACT$", "LPAD$",
            "RPAD$", "CENTER$", "STRREV$", "HEX$", "BIN$", "OCT$", "JOIN$",
            "SPLIT$"
        };

        static const std::unordered_set<std::string> doubleReturn = {
            "VAL", "RND", "SIN", "COS", "TAN", "ATAN", "ATAN2", "LOG", "EXP", "SQRT",
            "ABS", "POW", "TIMER", "SQR", "LN", "ATN", "ASIN", "ACOS",
            "SINH", "COSH", "TANH", "ASINH", "ACOSH", "ATANH",
            "LOG10", "LOG1P", "EXP2", "EXPM1", "CBRT", "HYPOT", "FMOD", "REMAINDER",
            "FLOOR", "CEIL", "TRUNC", "ROUND", "COPYSIGN",
            "ERF", "ERFC", "TGAMMA", "LGAMMA", "NEXTAFTER", "FMAX", "FMIN", "FMA",
            "DEG", "RAD", "SIGMOID", "LOGIT", "NORMPDF", "NORMCDF", "FACT", "FACTORIAL",
            "COMB", "PERM", "CLAMP", "LERP", "PMT", "PV", "FV"
        };

        std::string callReturnType = "w";  // Default to word
        if (!upper.empty() && upper.back() == '$') {
            callReturnType = "l";
        }
        if (longReturn.count(upper)) {
            callReturnType = "l";  // int64_t or uint32_t -> long
        } else if (stringReturn.count(upper)) {
            callReturnType = "l";  // StringDescriptor* -> long pointer
        } else if (doubleReturn.count(upper)) {
            callReturnType = "d";  // double
        }
        
        resultTemp = allocTemp(callReturnType);
        emit("    " + resultTemp + " =" + callReturnType + " call $" + runtimeFunc + "(");
        
        for (size_t i = 0; i < convertedArgTemps.size(); ++i) {
            if (i > 0) emit(", ");
            emit(convertedArgTypes[i] + " " + convertedArgTemps[i]);
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
    
    // Check if this is actually a function call (not an array access)
    // Function names are mangled with type suffixes (e.g., Factorial% -> Factorial_INT)
    std::string mangledName = expr->name;
    if (expr->typeSuffix != TokenType::UNKNOWN) {
        // Mangle the name with its type suffix
        switch (expr->typeSuffix) {
            case TokenType::TYPE_STRING:
                mangledName += "_STRING";
                break;
            case TokenType::TYPE_INT:
                mangledName += "_INT";
                break;
            case TokenType::TYPE_DOUBLE:
                mangledName += "_DOUBLE";
                break;
            case TokenType::TYPE_FLOAT:
                mangledName += "_FLOAT";
                break;
            default:
                break;
        }
    }
    
    // Check if this is a declared function
    if (m_symbols && m_symbols->functions.find(mangledName) != m_symbols->functions.end()) {
        // This is a function call, not array access - emit as function call
        const auto& funcSym = m_symbols->functions.at(mangledName);
        
        // Emit function call
        emitComment("Function call: " + mangledName);
        
        // Evaluate arguments
        std::vector<std::string> argTemps;
        std::vector<std::string> argTypes; // "w", "d", "l" for QBE
        
        for (const auto& argExpr : expr->indices) {
            std::string argTemp = emitExpression(argExpr.get());
            VariableType argType = inferExpressionType(argExpr.get());
            
            // Determine QBE type suffix
            std::string qbeType;
            if (argType == VariableType::STRING || argType == VariableType::UNICODE) {
                qbeType = "l";  // strings are pointers
            } else if (argType == VariableType::DOUBLE || argType == VariableType::FLOAT) {
                qbeType = "d";  // floats are doubles in QBE
            } else {
                qbeType = "w";  // integers
            }
            
            argTemps.push_back(argTemp);
            argTypes.push_back(qbeType);
        }
        
        // Build argument list
        std::string argList;
        for (size_t i = 0; i < argTemps.size(); ++i) {
            if (i > 0) argList += ", ";
            argList += argTypes[i] + " " + argTemps[i];
        }
        
        // Determine return type
        std::string retType;
        if (funcSym.returnType == VariableType::STRING || funcSym.returnType == VariableType::UNICODE) {
            retType = "l";
        } else if (funcSym.returnType == VariableType::DOUBLE || funcSym.returnType == VariableType::FLOAT) {
            retType = "d";
        } else {
            retType = "w";
        }
        
        // Emit the call
        std::string resultTemp = allocTemp(retType);
        emit("    " + resultTemp + " =" + retType + " call $" + mangledName + "(" + argList + ")\n");
        m_stats.instructionsGenerated++;
        
        return resultTemp;
    }
    
    // Not a function - proceed with array access
    // Evaluate indices - array indices MUST be integers
    std::vector<std::string> indexTemps;
    for (const auto& indexExpr : expr->indices) {
        std::string indexTemp = emitExpression(indexExpr.get());
        VariableType indexType = inferExpressionType(indexExpr.get());
        
        // Convert to integer if needed (array indices are always integers)
        if (indexType != VariableType::INT) {
            indexTemp = promoteToType(indexTemp, indexType, VariableType::INT);
        }
        
        indexTemps.push_back(indexTemp);
    }
    
    // For now, only support 1D arrays
    if (indexTemps.size() != 1) {
        emitComment("ERROR: Multi-dimensional arrays not yet supported");
        std::string temp = allocTemp("l");
        emit("    " + temp + " =l copy 0\n");
        m_stats.instructionsGenerated++;
        return temp;
    }
    
    std::string indexTemp = indexTemps[0];
    
    // Get array descriptor pointer
    std::string descPtr = getArrayRef(expr->name);
    
    emitComment("Array access " + expr->name + " with bounds check");
    
    // Load lowerBound from descriptor (offset 8)
    std::string lowerBoundAddr = allocTemp("l");
    emit("    " + lowerBoundAddr + " =l add " + descPtr + ", 8\n");
    std::string lowerBound = allocTemp("l");
    emit("    " + lowerBound + " =l loadl " + lowerBoundAddr + "\n");
    m_stats.instructionsGenerated += 2;
    
    // Load upperBound from descriptor (offset 16)
    std::string upperBoundAddr = allocTemp("l");
    emit("    " + upperBoundAddr + " =l add " + descPtr + ", 16\n");
    std::string upperBound = allocTemp("l");
    emit("    " + upperBound + " =l loadl " + upperBoundAddr + "\n");
    m_stats.instructionsGenerated += 2;
    
    // Convert index to long for comparison
    std::string indexLong = allocTemp("l");
    emit("    " + indexLong + " =l extsw " + indexTemp + "\n");
    m_stats.instructionsGenerated++;
    
    // Bounds check: index >= lowerBound
    std::string checkLower = allocTemp("w");
    emit("    " + checkLower + " =w csge" + "l " + indexLong + ", " + lowerBound + "\n");
    m_stats.instructionsGenerated++;
    
    // Bounds check: index <= upperBound
    std::string checkUpper = allocTemp("w");
    emit("    " + checkUpper + " =w csle" + "l " + indexLong + ", " + upperBound + "\n");
    m_stats.instructionsGenerated++;
    
    // Combined check: both must be true
    std::string checkBoth = allocTemp("w");
    emit("    " + checkBoth + " =w and " + checkLower + ", " + checkUpper + "\n");
    m_stats.instructionsGenerated++;
    
    // Branch: if out of bounds, call error handler
    std::string boundsOkLabel = allocLabel();
    std::string boundsErrLabel = allocLabel();
    
    emit("    jnz " + checkBoth + ", @" + boundsOkLabel + ", @" + boundsErrLabel + "\n");
    m_stats.instructionsGenerated++;
    
    // Bounds error block
    emit("@" + boundsErrLabel + "\n");
    emit("    call $basic_array_bounds_error(l " + indexLong + ", l " + lowerBound + ", l " + upperBound + ")\n");
    m_stats.instructionsGenerated++;
    
    // Bounds OK block
    emit("@" + boundsOkLabel + "\n");
    
    // Calculate offset: (index - lowerBound) * elementSize
    std::string adjustedIndex = allocTemp("l");
    emit("    " + adjustedIndex + " =l sub " + indexLong + ", " + lowerBound + "\n");
    m_stats.instructionsGenerated++;
    
    // Load elementSize from descriptor (offset 24)
    std::string elemSizeAddr = allocTemp("l");
    emit("    " + elemSizeAddr + " =l add " + descPtr + ", 24\n");
    std::string elemSize = allocTemp("l");
    emit("    " + elemSize + " =l loadl " + elemSizeAddr + "\n");
    m_stats.instructionsGenerated += 2;
    
    // Calculate byte offset
    std::string byteOffset = allocTemp("l");
    emit("    " + byteOffset + " =l mul " + adjustedIndex + ", " + elemSize + "\n");
    m_stats.instructionsGenerated++;
    
    // Load data pointer from descriptor (offset 0)
    std::string dataPtr = allocTemp("l");
    emit("    " + dataPtr + " =l loadl " + descPtr + "\n");
    m_stats.instructionsGenerated++;
    
    // Calculate element address
    std::string elementPtr = allocTemp("l");
    emit("    " + elementPtr + " =l add " + dataPtr + ", " + byteOffset + "\n");
    m_stats.instructionsGenerated++;
    
    // Check if this is a UDT array - return pointer for member access
    auto elemTypeIt = m_arrayElementTypes.find(expr->name);
    if (elemTypeIt != m_arrayElementTypes.end()) {
        // UDT array - return pointer so member access can work
        return elementPtr;
    }
    
    // Regular scalar array - load and return the value
    // Determine array element type from symbol table using TypeDescriptor
    TypeDescriptor elementTypeDesc = TypeDescriptor(BaseType::INTEGER); // Default
    if (m_symbols && m_symbols->arrays.find(expr->name) != m_symbols->arrays.end()) {
        const auto& arraySym = m_symbols->arrays.at(expr->name);
        // Use TypeDescriptor if available, otherwise convert from legacy type
        if (arraySym.elementTypeDesc.baseType != BaseType::UNKNOWN) {
            elementTypeDesc = arraySym.elementTypeDesc;
        } else {
            elementTypeDesc = legacyTypeToDescriptor(arraySym.type);
        }
    }
    
    std::string valueTemp;
    std::string qbeType = getQBETypeD(elementTypeDesc);
    std::string loadOp = getQBELoadOpD(elementTypeDesc);
    
    // Allocate temporary with correct QBE type
    valueTemp = allocTemp(qbeType);
    
    // Load with correct operation (handles sign/zero extension for byte/short)
    emit("    " + valueTemp + " =" + qbeType + " load" + loadOp + " " + elementPtr + "\n");
    m_stats.instructionsGenerated++;
    
    return valueTemp;
}

// Helper to get array element pointer without loading (for lvalue assignment)
std::string QBECodeGenerator::emitArrayElementPtr(const std::string& arrayName, 
                                                    const std::vector<std::unique_ptr<Expression>>& indices) {
    if (indices.size() != 1) {
        emitComment("ERROR: Multi-dimensional arrays not yet supported");
        std::string temp = allocTemp("l");
        emit("    " + temp + " =l copy 0\n");
        m_stats.instructionsGenerated++;
        return temp;
    }
    
    // Evaluate index
    std::string indexTemp = emitExpression(indices[0].get());
    VariableType indexType = inferExpressionType(indices[0].get());
    
    // Convert to integer if needed
    if (indexType != VariableType::INT) {
        indexTemp = promoteToType(indexTemp, indexType, VariableType::INT);
    }
    
    // Get array descriptor pointer
    std::string descPtr = getArrayRef(arrayName);
    
    emitComment("Get array element pointer for " + arrayName);
    
    // Load lowerBound from descriptor (offset 8)
    std::string lowerBoundAddr = allocTemp("l");
    emit("    " + lowerBoundAddr + " =l add " + descPtr + ", 8\n");
    std::string lowerBound = allocTemp("l");
    emit("    " + lowerBound + " =l loadl " + lowerBoundAddr + "\n");
    m_stats.instructionsGenerated += 2;
    
    // Load upperBound from descriptor (offset 16)
    std::string upperBoundAddr = allocTemp("l");
    emit("    " + upperBoundAddr + " =l add " + descPtr + ", 16\n");
    std::string upperBound = allocTemp("l");
    emit("    " + upperBound + " =l loadl " + upperBoundAddr + "\n");
    m_stats.instructionsGenerated += 2;
    
    // Convert index to long for comparison
    std::string indexLong = allocTemp("l");
    emit("    " + indexLong + " =l extsw " + indexTemp + "\n");
    m_stats.instructionsGenerated++;
    
    // Bounds check: index >= lowerBound
    std::string checkLower = allocTemp("w");
    emit("    " + checkLower + " =w csge" + "l " + indexLong + ", " + lowerBound + "\n");
    m_stats.instructionsGenerated++;
    
    // Bounds check: index <= upperBound
    std::string checkUpper = allocTemp("w");
    emit("    " + checkUpper + " =w csle" + "l " + indexLong + ", " + upperBound + "\n");
    m_stats.instructionsGenerated++;
    
    // Combined check
    std::string checkBoth = allocTemp("w");
    emit("    " + checkBoth + " =w and " + checkLower + ", " + checkUpper + "\n");
    m_stats.instructionsGenerated++;
    
    // Branch for bounds check
    std::string boundsOkLabel = allocLabel();
    std::string boundsErrLabel = allocLabel();
    
    emit("    jnz " + checkBoth + ", @" + boundsOkLabel + ", @" + boundsErrLabel + "\n");
    m_stats.instructionsGenerated++;
    
    // Bounds error block
    emit("@" + boundsErrLabel + "\n");
    emit("    call $basic_array_bounds_error(l " + indexLong + ", l " + lowerBound + ", l " + upperBound + ")\n");
    m_stats.instructionsGenerated++;
    
    // Bounds OK block
    emit("@" + boundsOkLabel + "\n");
    
    // Calculate offset: (index - lowerBound) * elementSize
    std::string adjustedIndex = allocTemp("l");
    emit("    " + adjustedIndex + " =l sub " + indexLong + ", " + lowerBound + "\n");
    m_stats.instructionsGenerated++;
    
    // Load elementSize from descriptor (offset 24)
    std::string elemSizeAddr = allocTemp("l");
    emit("    " + elemSizeAddr + " =l add " + descPtr + ", 24\n");
    std::string elemSize = allocTemp("l");
    emit("    " + elemSize + " =l loadl " + elemSizeAddr + "\n");
    m_stats.instructionsGenerated += 2;
    
    // Calculate byte offset
    std::string byteOffset = allocTemp("l");
    emit("    " + byteOffset + " =l mul " + adjustedIndex + ", " + elemSize + "\n");
    m_stats.instructionsGenerated++;
    
    // Load data pointer from descriptor (offset 0)
    std::string dataPtr = allocTemp("l");
    emit("    " + dataPtr + " =l loadl " + descPtr + "\n");
    m_stats.instructionsGenerated++;
    
    // Calculate element address
    std::string elementPtr = allocTemp("l");
    emit("    " + elementPtr + " =l add " + dataPtr + ", " + byteOffset + "\n");
    m_stats.instructionsGenerated++;
    
    return elementPtr;
}

// =============================================================================
// Member Access Expression (User-Defined Types)
// =============================================================================

std::string QBECodeGenerator::emitMemberAccessExpr(const MemberAccessExpression* expr) {
    if (!expr) {
        std::string temp = allocTemp("d");
        emit("    " + temp + " =d copy d_0.0\n");
        m_stats.instructionsGenerated++;
        return temp;
    }
    
    // 1. Get base object (can be variable, array element, or another member access)
    std::string baseTemp = emitExpression(expr->object.get());
    
    // 2. Determine the type of the base object
    std::string baseTypeName = inferMemberAccessType(expr->object.get());
    
    if (baseTypeName.empty()) {
        emitComment("ERROR: Member access on non-UDT type");
        std::string temp = allocTemp("d");
        emit("    " + temp + " =d copy d_0.0\n");
        m_stats.instructionsGenerated++;
        return temp;
    }
    
    // 3. Look up field in type definition
    const TypeSymbol* typeSymbol = getTypeSymbol(baseTypeName);
    if (!typeSymbol) {
        emitComment("ERROR: Type not found: " + baseTypeName);
        std::string temp = allocTemp("d");
        emit("    " + temp + " =d copy d_0.0\n");
        m_stats.instructionsGenerated++;
        return temp;
    }
    
    const TypeSymbol::Field* field = typeSymbol->findField(expr->memberName);
    if (!field) {
        emitComment("ERROR: Field not found: " + expr->memberName + " in type " + baseTypeName);
        std::string temp = allocTemp("d");
        emit("    " + temp + " =d copy d_0.0\n");
        m_stats.instructionsGenerated++;
        return temp;
    }
    
    // 4. Calculate field offset
    size_t offset = calculateFieldOffset(baseTypeName, expr->memberName);
    
    // 5. Compute member address (base pointer + offset)
    std::string memberPtr = allocTemp("l");
    if (offset == 0) {
        // No offset, just use base pointer
        emit("    " + memberPtr + " =l copy " + baseTemp + "\n");
    } else {
        emit("    " + memberPtr + " =l add " + baseTemp + ", " + std::to_string(offset) + "\n");
    }
    m_stats.instructionsGenerated++;
    
    // 6. Load value based on field type
    std::string resultTemp;
    
    if (field->isBuiltIn) {
        // Built-in type - load the value
        std::string qbeType = getQBEType(field->builtInType);
        resultTemp = allocTemp(qbeType);
        
        if (qbeType == "w") {
            emit("    " + resultTemp + " =w loadw " + memberPtr + "\n");
        } else if (qbeType == "d") {
            emit("    " + resultTemp + " =d loadd " + memberPtr + "\n");
        } else if (qbeType == "l") {
            emit("    " + resultTemp + " =l loadl " + memberPtr + "\n");
        } else {
            // Default to word
            emit("    " + resultTemp + " =w loadw " + memberPtr + "\n");
        }
        m_stats.instructionsGenerated++;
    } else {
        // Nested UDT - return the pointer (no load)
        // This allows further member access: Player.Position.X
        resultTemp = memberPtr;
    }
    
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
    
    // Special cases that don't follow uppercase pattern
    if (upper == "TIMER") return "basic_timer";
    if (upper == "RND") return "basic_rnd";
    if (upper == "RAND") return "basic_rand";
    
    // Common math functions
    if (upper == "ABS") return "basic_abs_double";  // runtime provides typed variants
    if (upper == "SIN") return "basic_sin";
    if (upper == "COS") return "basic_cos";
    if (upper == "TAN") return "basic_tan";
    if (upper == "ATAN" || upper == "ATN") return "basic_atan";
    if (upper == "SQRT" || upper == "SQR") return "basic_sqrt";
    if (upper == "LOG" || upper == "LN") return "basic_log";
    if (upper == "LOG10") return "basic_log10";
    if (upper == "LOG1P") return "basic_log1p";
    if (upper == "EXP") return "basic_exp";
    if (upper == "EXP2") return "basic_exp2";
    if (upper == "EXPM1") return "basic_expm1";
    if (upper == "POW") return "basic_pow";
    if (upper == "ATAN2") return "basic_atan2";
    if (upper == "ASIN" || upper == "ASN") return "basic_asin";
    if (upper == "ACOS" || upper == "ACS") return "basic_acos";
    if (upper == "SINH") return "basic_sinh";
    if (upper == "COSH") return "basic_cosh";
    if (upper == "TANH") return "basic_tanh";
    if (upper == "ASINH") return "basic_asinh";
    if (upper == "ACOSH") return "basic_acosh";
    if (upper == "ATANH") return "basic_atanh";
    if (upper == "CBRT") return "basic_cbrt";
    if (upper == "HYPOT") return "basic_hypot";
    if (upper == "FMOD") return "basic_fmod";
    if (upper == "REMAINDER") return "basic_remainder";
    if (upper == "FLOOR") return "basic_floor";
    if (upper == "CEIL") return "basic_ceil";
    if (upper == "TRUNC") return "basic_trunc";
    if (upper == "ROUND") return "basic_round";
    if (upper == "COPYSIGN") return "basic_copysign";
    if (upper == "ERF") return "basic_erf";
    if (upper == "ERFC") return "basic_erfc";
    if (upper == "TGAMMA") return "basic_tgamma";
    if (upper == "LGAMMA") return "basic_lgamma";
    if (upper == "NEXTAFTER") return "basic_nextafter";
    if (upper == "FMAX") return "basic_fmax";
    if (upper == "FMIN") return "basic_fmin";
    if (upper == "FMA") return "basic_fma";
    if (upper == "DEG") return "basic_deg";
    if (upper == "RAD") return "basic_rad";
    if (upper == "SIGMOID") return "basic_sigmoid";
    if (upper == "LOGIT") return "basic_logit";
    if (upper == "NORMPDF") return "basic_normpdf";
    if (upper == "NORMCDF") return "basic_normcdf";
    if (upper == "FACT" || upper == "FACTORIAL") return "basic_fact";
    if (upper == "COMB") return "basic_comb";
    if (upper == "PERM") return "basic_perm";
    if (upper == "CLAMP") return "basic_clamp";
    if (upper == "LERP") return "basic_lerp";
    if (upper == "PMT") return "basic_pmt";
    if (upper == "PV") return "basic_pv";
    if (upper == "FV") return "basic_fv";
    if (upper == "INT") return "basic_int";
    if (upper == "FIX") return "basic_fix";
    if (upper == "CINT") return "math_cint";
    if (upper == "RND") return "basic_rnd";
    if (upper == "SGN") return "basic_sgn";
    
    // String functions - UTF-32 runtime
    if (upper == "LEN") return "basic_len";          // Returns int64_t
    if (upper == "LEFT$") return "string_left";      // Returns StringDescriptor*
    if (upper == "RIGHT$") return "string_right";    // Returns StringDescriptor*
    if (upper == "MID$") return "string_mid";        // Returns StringDescriptor*
    if (upper == "CHR$") return "basic_chr";         // Returns StringDescriptor*
    if (upper == "ASC") return "basic_asc";          // Returns uint32_t
    if (upper == "STR$") return "basic_str_double";  // Returns StringDescriptor*
    if (upper == "VAL") return "basic_val";          // Returns double
    if (upper == "SPACE$") return "basic_space";     // Returns StringDescriptor*
    if (upper == "STRING$") return "basic_string_repeat"; // Returns StringDescriptor*
    if (upper == "UCASE$") return "string_upper";    // Returns StringDescriptor*
    if (upper == "LCASE$") return "string_lower";    // Returns StringDescriptor*
    if (upper == "TRIM$") return "string_trim";      // Returns StringDescriptor*
    if (upper == "LTRIM$") return "string_ltrim";    // Returns StringDescriptor*
    if (upper == "RTRIM$") return "string_rtrim";    // Returns StringDescriptor*
    if (upper == "INSTR") return "string_instr";     // Returns int64_t
    if (upper == "INSTRREV") return "string_instrrev"; // Returns int64_t
    if (upper == "REPLACE$") return "string_replace";
    if (upper == "REVERSE$") return "string_reverse";
    if (upper == "TALLY") return "string_tally";      // Returns int64_t
    if (upper == "INSERT$") return "string_insert";
    if (upper == "DELETE$") return "string_delete";
    if (upper == "REMOVE$") return "string_remove";
    if (upper == "EXTRACT$") return "string_extract";
    if (upper == "LPAD$") return "string_lpad";
    if (upper == "RPAD$") return "string_rpad";
    if (upper == "CENTER$") return "string_center";
    if (upper == "STRREV$") return "string_reverse";
    if (upper == "HEX$") return "HEX_STRING";
    if (upper == "BIN$") return "BIN_STRING";
    if (upper == "OCT$") return "OCT_STRING";
    if (upper == "JOIN$") return "string_join";
    if (upper == "SPLIT$") return "string_split";
    
    // Default: prefix with basic_
    return "basic_" + upper;
}

} // namespace FasterBASIC