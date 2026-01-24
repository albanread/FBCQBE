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
    
    // Not a constant - return the variable reference (QBE uses SSA)
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
    
    // Bitwise and MOD operations REQUIRE integers
    bool requiresInteger = (op == TokenType::AND || op == TokenType::OR || 
                           op == TokenType::XOR || op == TokenType::MOD);
    
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
    std::string typeSuffix = (opType == VariableType::INT) ? "w" : "d";
    
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
            // MOD is integer-only operation
            emit("    " + resultTemp + " =w rem " + leftTemp + ", " + rightTemp + "\n");
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
            emit("    " + resultTemp + " =w cslt" + typeSuffix + " " + leftTemp + ", " + rightTemp + "\n");
            break;
            
        case TokenType::LESS_EQUAL:
            resultTemp = allocTemp("w");
            emit("    " + resultTemp + " =w csle" + typeSuffix + " " + leftTemp + ", " + rightTemp + "\n");
            break;
            
        case TokenType::GREATER_THAN:
            resultTemp = allocTemp("w");
            emit("    " + resultTemp + " =w csgt" + typeSuffix + " " + leftTemp + ", " + rightTemp + "\n");
            break;
            
        case TokenType::GREATER_EQUAL:
            resultTemp = allocTemp("w");
            emit("    " + resultTemp + " =w csge" + typeSuffix + " " + leftTemp + ", " + rightTemp + "\n");
            break;
            
        // Logical/Bitwise operators (integer-only)
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
    
    // Evaluate arguments (get raw temporaries)
    std::vector<std::string> argTemps;
    std::vector<VariableType> argTypes;
    for (const auto& arg : expr->arguments) {
        argTemps.push_back(emitExpression(arg.get()));
        argTypes.push_back(inferExpressionType(arg.get()));
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
        
        emit("    " + resultTemp + " =w call $" + runtimeFunc + "(");
        
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
    
    // Get array pointer
    std::string arrayRef = getArrayRef(expr->name);
    
    // Check if this is a UDT array - use pointer arithmetic instead of runtime call
    auto elemTypeIt = m_arrayElementTypes.find(expr->name);
    if (elemTypeIt != m_arrayElementTypes.end()) {
        // UDT array - compute element address using pointer arithmetic
        std::string typeName = elemTypeIt->second;
        size_t elementSize = calculateTypeSize(typeName);
        
        // For now, only support 1D arrays (multi-dimensional coming later)
        if (indexTemps.size() != 1) {
            emitComment("ERROR: Multi-dimensional UDT arrays not yet supported");
            std::string temp = allocTemp("l");
            emit("    " + temp + " =l copy 0\n");
            m_stats.instructionsGenerated++;
            return temp;
        }
        
        // Compute offset: index * elementSize
        std::string indexTemp = indexTemps[0];
        
        // Convert index to long for pointer arithmetic
        std::string indexLong = allocTemp("l");
        emit("    " + indexLong + " =l extsw " + indexTemp + "\n");
        m_stats.instructionsGenerated++;
        
        // Multiply by element size
        std::string offsetTemp = allocTemp("l");
        emit("    " + offsetTemp + " =l mul " + indexLong + ", " + std::to_string(elementSize) + "\n");
        m_stats.instructionsGenerated++;
        
        // Add to base pointer to get element address
        std::string elementPtr = allocTemp("l");
        emit("    " + elementPtr + " =l add " + arrayRef + ", " + offsetTemp + "\n");
        m_stats.instructionsGenerated++;
        
        // Return pointer to element (for member access or whole-element operations)
        return elementPtr;
    }
    
    // Regular array - call runtime array access function
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