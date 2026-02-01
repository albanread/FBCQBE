#include "ast_emitter.h"
#include <sstream>
#include <cmath>
#include <iostream>

namespace fbc {

using namespace FasterBASIC;

ASTEmitter::ASTEmitter(QBEBuilder& builder, TypeManager& typeManager,
                       SymbolMapper& symbolMapper, RuntimeLibrary& runtime,
                       SemanticAnalyzer& semantic)
    : builder_(builder)
    , typeManager_(typeManager)
    , symbolMapper_(symbolMapper)
    , runtime_(runtime)
    , semantic_(semantic)
{
}

// === Expression Emission ===

std::string ASTEmitter::emitExpression(const Expression* expr) {
    if (!expr) {
        builder_.emitComment("ERROR: null expression");
        return "0";
    }
    
    switch (expr->getType()) {
        case ASTNodeType::EXPR_NUMBER:
            return emitNumberLiteral(static_cast<const NumberExpression*>(expr), BaseType::UNKNOWN);
            
        case ASTNodeType::EXPR_STRING:
            return emitStringLiteral(static_cast<const StringExpression*>(expr));
        
        case ASTNodeType::EXPR_VARIABLE:
            return emitVariableExpression(static_cast<const VariableExpression*>(expr));
        
        case ASTNodeType::EXPR_BINARY:
            return emitBinaryExpression(static_cast<const BinaryExpression*>(expr));
        
        case ASTNodeType::EXPR_UNARY:
            return emitUnaryExpression(static_cast<const UnaryExpression*>(expr));
        
        case ASTNodeType::EXPR_ARRAY_ACCESS:
            return emitArrayAccessExpression(static_cast<const ArrayAccessExpression*>(expr));
        
        case ASTNodeType::EXPR_FUNCTION_CALL:
            return emitFunctionCall(static_cast<const FunctionCallExpression*>(expr));
            
        case ASTNodeType::EXPR_IIF:
            return emitIIFExpression(static_cast<const IIFExpression*>(expr));
            
        default:
            builder_.emitComment("ERROR: unsupported expression type");
            return "0";
    }
}

std::string ASTEmitter::emitExpressionAs(const Expression* expr, BaseType expectedType) {
    if (!expr) {
        return "0";
    }
    
    // Special case: if it's a simple number literal, emit it with the expected type
    if (expr->getType() == ASTNodeType::EXPR_NUMBER) {
        return emitNumberLiteral(static_cast<const NumberExpression*>(expr), expectedType);
    }
    
    // For complex expressions, emit normally and convert if needed
    std::string value = emitExpression(expr);
    BaseType exprType = getExpressionType(expr);
    
    // Convert if necessary
    if (typeManager_.needsConversion(exprType, expectedType)) {
        return emitTypeConversion(value, exprType, expectedType);
    }
    
    return value;
}

// === Expression Emitters (by type) ===

std::string ASTEmitter::emitNumberLiteral(const NumberExpression* expr, BaseType expectedType) {
    double value = expr->value;
    
    // Check if it's an integer value
    if (value == std::floor(value) && value >= INT32_MIN && value <= INT32_MAX) {
        // Integer literal - but check if we need to emit as float/double for context
        if (expectedType == BaseType::SINGLE || expectedType == BaseType::DOUBLE) {
            std::ostringstream oss;
            oss.precision(17);
            oss << (expectedType == BaseType::SINGLE ? "s_" : "d_") << value;
            return oss.str();
        }
        return std::to_string(static_cast<int>(value));
    } else {
        // Float/double literal - use expectedType if provided, otherwise default to double
        std::ostringstream oss;
        oss.precision(17);  // Full double precision
        
        if (expectedType == BaseType::SINGLE) {
            oss << "s_" << value;
        } else {
            // Default to double for floating literals (matches getExpressionType)
            oss << "d_" << value;
        }
        return oss.str();
    }
}

std::string ASTEmitter::emitStringLiteral(const StringExpression* expr) {
    // Get the label from the string pool (should already be registered)
    std::string label = builder_.getStringLabel(expr->value);
    
    if (label.empty()) {
        // Fallback: register now if somehow missed during collection
        label = builder_.registerString(expr->value);
        builder_.emitComment("WARNING: String not pre-registered: " + expr->value);
    }
    
    // Convert C string to FasterBASIC string descriptor
    return runtime_.emitStringLiteral(label);
}

std::string ASTEmitter::emitVariableExpression(const VariableExpression* expr) {
    return loadVariable(expr->name);
}

std::string ASTEmitter::emitBinaryExpression(const BinaryExpression* expr) {
    TokenType op = expr->op;
    
    // Get expression types
    BaseType leftType = getExpressionType(expr->left.get());
    BaseType rightType = getExpressionType(expr->right.get());
    
    // Check if this is a string operation
    if (typeManager_.isString(leftType) || typeManager_.isString(rightType)) {
        std::string left = emitExpressionAs(expr->left.get(), BaseType::STRING);
        std::string right = emitExpressionAs(expr->right.get(), BaseType::STRING);
        return emitStringOp(left, right, op);
    }
    
    // Numeric operation - promote to common type
    BaseType commonType = typeManager_.getPromotedType(leftType, rightType);
    
    std::string left = emitExpressionAs(expr->left.get(), commonType);
    std::string right = emitExpressionAs(expr->right.get(), commonType);
    
    // Check operation type
    if (op >= TokenType::EQUAL && op <= TokenType::GREATER_EQUAL) {
        // Comparison operation
        return emitComparisonOp(left, right, op, commonType);
    } else if (op == TokenType::AND || op == TokenType::OR || op == TokenType::XOR) {
        // Bitwise/logical operation
        return emitLogicalOp(left, right, op);
    } else {
        // Arithmetic operation
        return emitArithmeticOp(left, right, op, commonType);
    }
}

std::string ASTEmitter::emitUnaryExpression(const UnaryExpression* expr) {
    std::string operand = emitExpression(expr->expr.get());
    BaseType operandType = getExpressionType(expr->expr.get());
    std::string qbeType = typeManager_.getQBEType(operandType);
    
    if (expr->op == TokenType::MINUS) {
        // Negation
        std::string result = builder_.newTemp();
        builder_.emitNeg(result, qbeType, operand);
        return result;
    } else if (expr->op == TokenType::NOT) {
        // Bitwise NOT - flip all bits
        std::string result = builder_.newTemp();
        
        // Coerce to 32-bit integer if needed
        std::string notOperand = operand;
        if (typeManager_.isFloatingPoint(operandType)) {
            notOperand = builder_.newTemp();
            builder_.emitRaw("    " + notOperand + " =w " + qbeType + "tosi " + operand);
        }
        
        // Perform bitwise NOT using XOR with -1
        builder_.emitBinary(result, "w", "xor", notOperand, "-1");
        return result;
    } else if (expr->op == TokenType::PLUS) {
        // Unary plus - no-op
        return operand;
    } else {
        builder_.emitComment("ERROR: unsupported unary operator");
        return operand;
    }
}

std::string ASTEmitter::emitArrayAccessExpression(const ArrayAccessExpression* expr) {
    return loadArrayElement(expr->name, expr->indices);
}

std::string ASTEmitter::emitIIFExpression(const IIFExpression* expr) {
    if (!expr || !expr->condition || !expr->trueValue || !expr->falseValue) {
        builder_.emitComment("ERROR: invalid IIF expression");
        return builder_.newTemp();
    }
    
    builder_.emitComment("IIF expression");
    
    // Determine result type from the branches
    BaseType trueType = getExpressionType(expr->trueValue.get());
    BaseType falseType = getExpressionType(expr->falseValue.get());
    
    // Use the promoted type
    BaseType resultType = typeManager_.getPromotedType(trueType, falseType);
    std::string qbeType = typeManager_.getQBEType(resultType);
    
    // Allocate result temporary
    std::string resultTemp = builder_.newTemp();
    
    // Create labels
    std::string trueLabel = symbolMapper_.getUniqueLabel("iif_true");
    std::string falseLabel = symbolMapper_.getUniqueLabel("iif_false");
    std::string endLabel = symbolMapper_.getUniqueLabel("iif_end");
    
    // Evaluate condition
    std::string condTemp = emitExpression(expr->condition.get());
    BaseType condType = getExpressionType(expr->condition.get());
    
    // Convert condition to word if needed
    std::string condWord = condTemp;
    std::string condQbeType = typeManager_.getQBEType(condType);
    if (condQbeType != "w") {
        condWord = builder_.newTemp();
        if (condQbeType == "d") {
            builder_.emitConvert(condWord, "w", "dtosi", condTemp);
        } else if (condQbeType == "s") {
            builder_.emitConvert(condWord, "w", "stosi", condTemp);
        } else if (condQbeType == "l") {
            builder_.emitTrunc(condWord, "w", condTemp);
        }
    }
    
    // Branch based on condition
    builder_.emitBranch(condWord, trueLabel, falseLabel);
    
    // True branch
    builder_.emitLabel(trueLabel);
    std::string trueTemp = emitExpression(expr->trueValue.get());
    
    // Convert true value to result type if needed
    if (trueType != resultType) {
        trueTemp = emitTypeConversion(trueTemp, trueType, resultType);
    }
    
    builder_.emitInstruction(resultTemp + " =" + qbeType + " copy " + trueTemp);
    builder_.emitJump(endLabel);
    
    // False branch
    builder_.emitLabel(falseLabel);
    std::string falseTemp = emitExpression(expr->falseValue.get());
    
    // Convert false value to result type if needed
    if (falseType != resultType) {
        falseTemp = emitTypeConversion(falseTemp, falseType, resultType);
    }
    
    builder_.emitInstruction(resultTemp + " =" + qbeType + " copy " + falseTemp);
    
    // End label
    builder_.emitLabel(endLabel);
    
    return resultTemp;
}

std::string ASTEmitter::emitFunctionCall(const FunctionCallExpression* expr) {
    std::string funcName = expr->name;
    
    // Convert to uppercase for case-insensitive matching
    std::string upperName = funcName;
    std::transform(upperName.begin(), upperName.end(), upperName.begin(), ::toupper);
    
    // Check for intrinsic/built-in functions
    
    // ABS(x) - Absolute value
    if (upperName == "ABS") {
        if (expr->arguments.size() != 1) {
            builder_.emitComment("ERROR: ABS requires exactly 1 argument");
            return "0";
        }
        std::string argTemp = emitExpression(expr->arguments[0].get());
        BaseType argType = getExpressionType(expr->arguments[0].get());
        
        if (typeManager_.isIntegral(argType)) {
            // For integers: use conditional to get absolute value
            std::string isNeg = builder_.newTemp();
            builder_.emitCompare(isNeg, "w", "slt", argTemp, "0");
            
            std::string negVal = builder_.newTemp();
            builder_.emitNeg(negVal, "w", argTemp);
            
            // Use phi-like pattern with conditional
            std::string thenLabel = "abs_neg_" + std::to_string(builder_.getTempCounter());
            std::string elseLabel = "abs_pos_" + std::to_string(builder_.getTempCounter());
            std::string endLabel = "abs_end_" + std::to_string(builder_.getTempCounter());
            std::string result = builder_.newTemp();
            
            builder_.emitRaw("    jnz " + isNeg + ", @" + thenLabel + ", @" + elseLabel);
            builder_.emitLabel(thenLabel);
            builder_.emitRaw("    " + result + " =w copy " + negVal);
            builder_.emitRaw("    jmp @" + endLabel);
            builder_.emitLabel(elseLabel);
            builder_.emitRaw("    " + result + " =w copy " + argTemp);
            builder_.emitLabel(endLabel);
            
            return result;
        } else {
            // For floats/doubles: use runtime function
            return runtime_.emitAbs(argTemp, argType);
        }
    }
    
    // SGN(x) - Sign function (-1, 0, or 1)
    if (upperName == "SGN") {
        if (expr->arguments.size() != 1) {
            builder_.emitComment("ERROR: SGN requires exactly 1 argument");
            return "0";
        }
        std::string argTemp = emitExpression(expr->arguments[0].get());
        BaseType argType = getExpressionType(expr->arguments[0].get());
        
        if (typeManager_.isIntegral(argType)) {
            // For integers: branchless using (x > 0) - (x < 0)
            std::string isNeg = builder_.newTemp();
            builder_.emitCompare(isNeg, "w", "slt", argTemp, "0");
            
            std::string isPos = builder_.newTemp();
            builder_.emitCompare(isPos, "w", "sgt", argTemp, "0");
            
            std::string result = builder_.newTemp();
            builder_.emitBinary(result, "w", "sub", isPos, isNeg);
            
            return result;
        } else {
            // For floats/doubles: use runtime function
            std::string qbeType = typeManager_.getQBEType(argType);
            std::string result = builder_.newTemp();
            builder_.emitCall(result, "w", "basic_sgn", qbeType + " " + argTemp);
            return result;
        }
    }
    
    if (upperName == "LEN") {
        // LEN(string$) - returns length of string
        if (expr->arguments.size() != 1) {
            builder_.emitComment("ERROR: LEN requires exactly 1 argument");
            return "0";
        }
        std::string strArg = emitExpression(expr->arguments[0].get());
        return runtime_.emitStringLen(strArg);
    }
    
    if (upperName == "MID" || upperName == "MID$") {
        // MID$(string$, start[, length]) - substring extraction
        if (expr->arguments.size() < 2 || expr->arguments.size() > 3) {
            builder_.emitComment("ERROR: MID$ requires 2 or 3 arguments");
            return "0";
        }
        std::string strArg = emitExpression(expr->arguments[0].get());
        std::string startArg = emitExpression(expr->arguments[1].get());
        std::string lenArg = expr->arguments.size() == 3 ? 
                             emitExpression(expr->arguments[2].get()) : "";
        return runtime_.emitMid(strArg, startArg, lenArg);
    }
    
    if (upperName == "LEFT" || upperName == "LEFT$") {
        // LEFT$(string$, n) - left n characters
        if (expr->arguments.size() != 2) {
            builder_.emitComment("ERROR: LEFT$ requires exactly 2 arguments");
            return "0";
        }
        std::string strArg = emitExpression(expr->arguments[0].get());
        std::string lenArg = emitExpression(expr->arguments[1].get());
        return runtime_.emitLeft(strArg, lenArg);
    }
    
    if (upperName == "RIGHT" || upperName == "RIGHT$") {
        // RIGHT$(string$, n) - right n characters
        if (expr->arguments.size() != 2) {
            builder_.emitComment("ERROR: RIGHT$ requires exactly 2 arguments");
            return "0";
        }
        std::string strArg = emitExpression(expr->arguments[0].get());
        std::string lenArg = emitExpression(expr->arguments[1].get());
        return runtime_.emitRight(strArg, lenArg);
    }
    
    if (upperName == "CHR" || upperName == "CHR$") {
        // CHR$(n) - character from ASCII code
        if (expr->arguments.size() != 1) {
            builder_.emitComment("ERROR: CHR$ requires exactly 1 argument");
            return "0";
        }
        std::string codeArg = emitExpression(expr->arguments[0].get());
        return runtime_.emitChr(codeArg);
    }
    
    if (upperName == "ASC") {
        // ASC(string$) - ASCII code of first character
        if (expr->arguments.size() != 1) {
            builder_.emitComment("ERROR: ASC requires exactly 1 argument");
            return "0";
        }
        std::string strArg = emitExpression(expr->arguments[0].get());
        return runtime_.emitAsc(strArg);
    }
    
    if (upperName == "STR" || upperName == "STR$") {
        // STR$(n) - convert number to string
        if (expr->arguments.size() != 1) {
            builder_.emitComment("ERROR: STR$ requires exactly 1 argument");
            return "0";
        }
        std::string numArg = emitExpression(expr->arguments[0].get());
        BaseType argType = getExpressionType(expr->arguments[0].get());
        return runtime_.emitStr(numArg, argType);
    }
    
    if (upperName == "VAL") {
        // VAL(string$) - convert string to number
        if (expr->arguments.size() != 1) {
            builder_.emitComment("ERROR: VAL requires exactly 1 argument");
            return "0";
        }
        std::string strArg = emitExpression(expr->arguments[0].get());
        return runtime_.emitVal(strArg);
    }
    
    // Note: INSTR not yet implemented in runtime library
    if (upperName == "INSTR") {
        builder_.emitComment("TODO: INSTR function not yet implemented");
        return "0";
    }
    
    // Math functions that map to runtime
    if (upperName == "SIN" || upperName == "COS" || upperName == "TAN" ||
        upperName == "ATAN" || upperName == "ASIN" || upperName == "ACOS" ||
        upperName == "LOG" || upperName == "EXP" || upperName == "SQRT" || upperName == "SQR") {
        if (expr->arguments.size() != 1) {
            builder_.emitComment("ERROR: " + upperName + " requires exactly 1 argument");
            return "0";
        }
        std::string argTemp = emitExpression(expr->arguments[0].get());
        BaseType argType = getExpressionType(expr->arguments[0].get());
        
        // Convert to double if needed
        if (!typeManager_.isFloatingPoint(argType)) {
            argTemp = emitTypeConversion(argTemp, argType, BaseType::DOUBLE);
        }
        
        std::string runtimeFunc = "basic_" + upperName;
        std::transform(runtimeFunc.begin(), runtimeFunc.end(), runtimeFunc.begin(), ::tolower);
        if (upperName == "SQR") runtimeFunc = "basic_sqrt";
        
        std::string result = builder_.newTemp();
        builder_.emitCall(result, "d", runtimeFunc, "d " + argTemp);
        return result;
    }
    
    if (upperName == "INT" || upperName == "FIX") {
        if (expr->arguments.size() != 1) {
            builder_.emitComment("ERROR: " + upperName + " requires exactly 1 argument");
            return "0";
        }
        std::string argTemp = emitExpression(expr->arguments[0].get());
        BaseType argType = getExpressionType(expr->arguments[0].get());
        
        if (typeManager_.isFloatingPoint(argType)) {
            std::string qbeType = typeManager_.getQBEType(argType);
            std::string result = builder_.newTemp();
            builder_.emitRaw("    " + result + " =w " + qbeType + "tosi " + argTemp);
            return result;
        }
        return argTemp;  // Already integer
    }
    
    if (upperName == "RND") {
        // RND() - random number 0.0 to 1.0
        std::string result = builder_.newTemp();
        builder_.emitCall(result, "d", "basic_rnd", "");
        return result;
    }
    
    // Check for user-defined functions (DEF FN)
    const auto& symbolTable = semantic_.getSymbolTable();
    auto funcIt = symbolTable.functions.find(funcName);
    if (funcIt != symbolTable.functions.end()) {
        // User-defined function call
        builder_.emitComment("TODO: user-defined function call " + funcName);
        return "0";
    }
    
    // Unknown function
    builder_.emitComment("ERROR: unknown function " + funcName);
    return "0";
}

// === Binary Operation Helpers ===

std::string ASTEmitter::emitArithmeticOp(const std::string& left, const std::string& right,
                                         TokenType op, BaseType type) {
    std::string qbeType = typeManager_.getQBEType(type);
    std::string qbeOp = getQBEArithmeticOp(op);
    
    std::string result = builder_.newTemp();
    builder_.emitBinary(result, qbeType, qbeOp, left, right);
    
    return result;
}

std::string ASTEmitter::emitComparisonOp(const std::string& left, const std::string& right,
                                         TokenType op, BaseType type) {
    std::string qbeType = typeManager_.getQBEType(type);
    std::string qbeOp = getQBEComparisonOp(op);
    
    std::string result = builder_.newTemp();
    builder_.emitCompare(result, qbeType, qbeOp, left, right);
    
    return result;
}

std::string ASTEmitter::emitLogicalOp(const std::string& left, const std::string& right,
                                      TokenType op) {
    std::string result = builder_.newTemp();
    
    if (op == TokenType::AND) {
        builder_.emitBinary(result, "w", "and", left, right);
    } else if (op == TokenType::OR) {
        builder_.emitBinary(result, "w", "or", left, right);
    } else if (op == TokenType::XOR) {
        builder_.emitBinary(result, "w", "xor", left, right);
    } else {
        builder_.emitComment("ERROR: unsupported logical operator");
        builder_.emitBinary(result, "w", "copy", left, "0");
    }
    
    return result;
}

std::string ASTEmitter::emitStringOp(const std::string& left, const std::string& right,
                                     TokenType op) {
    if (op == TokenType::PLUS) {
        // String concatenation
        return runtime_.emitStringConcat(left, right);
    } else if (op == TokenType::EQUAL) {
        // String equality
        std::string cmpResult = runtime_.emitStringCompare(left, right);
        std::string result = builder_.newTemp();
        builder_.emitCompare(result, "w", "eq", cmpResult, "0");
        return result;
    } else if (op == TokenType::NOT_EQUAL) {
        // String inequality
        std::string cmpResult = runtime_.emitStringCompare(left, right);
        std::string result = builder_.newTemp();
        builder_.emitCompare(result, "w", "ne", cmpResult, "0");
        return result;
    } else {
        builder_.emitComment("ERROR: unsupported string operator");
        return "0";
    }
}

// === Type Conversion Helpers ===

std::string ASTEmitter::emitTypeConversion(const std::string& value,
                                           BaseType fromType, BaseType toType) {
    if (fromType == toType) {
        return value;
    }
    
    std::string convOp = typeManager_.getConversionOp(fromType, toType);
    if (convOp.empty()) {
        return value;
    }
    
    // Handle special two-step conversions for integer to double
    if (convOp == "INT_TO_DOUBLE_W" || convOp == "INT_TO_DOUBLE_L") {
        // QBE doesn't have direct int→double, must go int→float→double
        std::string floatTemp = builder_.newTemp();
        std::string op1 = (convOp == "INT_TO_DOUBLE_W") ? "swtof" : "sltof";
        builder_.emitConvert(floatTemp, "s", op1, value);
        
        std::string result = builder_.newTemp();
        builder_.emitConvert(result, "d", "exts", floatTemp);
        return result;
    }
    
    std::string qbeToType = typeManager_.getQBEType(toType);
    std::string result = builder_.newTemp();
    
    builder_.emitConvert(result, qbeToType, convOp, value);
    
    return result;
}

// === Statement Emission ===

void ASTEmitter::emitStatement(const Statement* stmt) {
    if (!stmt) {
        builder_.emitComment("ERROR: null statement");
        return;
    }
    
    switch (stmt->getType()) {
        case ASTNodeType::STMT_LET:
            emitLetStatement(static_cast<const LetStatement*>(stmt));
            break;
            
        case ASTNodeType::STMT_PRINT:
            emitPrintStatement(static_cast<const PrintStatement*>(stmt));
            break;
            
        case ASTNodeType::STMT_INPUT:
            emitInputStatement(static_cast<const InputStatement*>(stmt));
            break;
            
        case ASTNodeType::STMT_END:
            emitEndStatement(static_cast<const EndStatement*>(stmt));
            break;
            
        case ASTNodeType::STMT_DIM:
            emitDimStatement(static_cast<const DimStatement*>(stmt));
            break;
            
        case ASTNodeType::STMT_FOR:
            emitForInit(static_cast<const ForStatement*>(stmt));
            break;
            
        case ASTNodeType::STMT_WHILE:
            // WHILE condition is handled by CFG edges
            builder_.emitComment("WHILE loop header");
            break;
            
        case ASTNodeType::STMT_IF:
            // IF condition is handled by CFG edges
            builder_.emitComment("IF statement");
            break;
            
        case ASTNodeType::STMT_GOSUB:
            // GOSUB is handled by CFG edges
            builder_.emitComment("GOSUB statement");
            break;
            
        case ASTNodeType::STMT_READ:
            emitReadStatement(static_cast<const ReadStatement*>(stmt));
            break;
            
        case ASTNodeType::STMT_RESTORE:
            emitRestoreStatement(static_cast<const RestoreStatement*>(stmt));
            break;
            
        case ASTNodeType::STMT_LOCAL:
            // LOCAL is like DIM but for function-local variables
            emitLocalStatement(static_cast<const LocalStatement*>(stmt));
            break;
            
        case ASTNodeType::STMT_SHARED:
            // SHARED is purely declarative - no code emission needed
            // Variables are already registered during function entry
            break;
            
        case ASTNodeType::STMT_GLOBAL:
            // GLOBAL is purely declarative - no code emission needed
            // Variables are declared at module level
            break;
            
        case ASTNodeType::STMT_CALL:
            emitCallStatement(static_cast<const CallStatement*>(stmt));
            break;
            
        default:
            builder_.emitComment("TODO: statement type " + std::to_string(static_cast<int>(stmt->getType())) + " not yet implemented");
            break;
    }
}

void ASTEmitter::emitLetStatement(const LetStatement* stmt) {
    // Get the target variable type first
    BaseType targetType = getVariableType(stmt->variable);
    
    // Emit the right-hand side expression with type context for smart literal generation
    std::string value = emitExpressionAs(stmt->value.get(), targetType);
    
    // Use variable name as-is - it's already mangled by the parser/semantic analyzer
    // (e.g., "Y#" becomes "Y_DOUBLE" in the symbol table)
    
    // Check if this is an array assignment
    if (!stmt->indices.empty()) {
        // Array assignment: arr(i,j) = value
        storeArrayElement(stmt->variable, stmt->indices, value);
    } else {
        // Regular variable assignment: x = value
        storeVariable(stmt->variable, value);
    }
}

void ASTEmitter::emitPrintStatement(const PrintStatement* stmt) {
    for (const auto& item : stmt->items) {
        if (item.expr) {
            BaseType exprType = getExpressionType(item.expr.get());
            std::string value = emitExpression(item.expr.get());
            
            if (typeManager_.isString(exprType)) {
                runtime_.emitPrintString(value);
            } else if (typeManager_.isFloatingPoint(exprType)) {
                if (exprType == BaseType::SINGLE) {
                    runtime_.emitPrintFloat(value);
                } else {
                    runtime_.emitPrintDouble(value);
                }
            } else {
                runtime_.emitPrintInt(value);
            }
        }
        
        // Handle separators
        if (item.comma) {
            runtime_.emitPrintTab();
        }
    }
    
    // Add final newline if not suppressed
    if (stmt->trailingNewline) {
        runtime_.emitPrintNewline();
    }
}

void ASTEmitter::emitInputStatement(const InputStatement* stmt) {
    // TODO: Handle prompt
    
    for (const auto& varName : stmt->variables) {
        BaseType varType = getVariableType(varName);
        std::string varAddr = getVariableAddress(varName);
        
        if (typeManager_.isString(varType)) {
            runtime_.emitInputString(varAddr);
        } else if (typeManager_.isFloatingPoint(varType)) {
            if (varType == BaseType::SINGLE) {
                runtime_.emitInputFloat(varAddr);
            } else {
                runtime_.emitInputDouble(varAddr);
            }
        } else {
            runtime_.emitInputInt(varAddr);
        }
    }
}

void ASTEmitter::emitEndStatement(const EndStatement* stmt) {
    // END statement - terminate execution
    builder_.emitComment("END statement - program exit");
    builder_.emitReturn("0");
}

void ASTEmitter::emitLocalStatement(const LocalStatement* stmt) {
    // LOCAL statement: allocate stack space for local variables in SUBs/FUNCTIONs
    // Similar to DIM but specifically for function-local scope
    
    for (const auto& varDecl : stmt->variables) {
        const std::string& varName = varDecl.name;
        
        builder_.emitComment("LOCAL variable: " + varName);
        
        // Look up variable in symbol table using scoped lookup
        std::string currentFunc = symbolMapper_.getCurrentFunction();
        const auto* varSymbol = semantic_.lookupVariableScoped(varName, currentFunc);
        if (!varSymbol) {
            builder_.emitComment("ERROR: LOCAL variable not found in symbol table: " + varName);
            continue;
        }
        
        // Allocate stack space for the local variable
        std::string mangledName = symbolMapper_.mangleVariableName(varName, false);
        BaseType varType = varSymbol->typeDesc.baseType;
        int64_t size = typeManager_.getTypeSize(varType);
        
        if (size == 4) {
            builder_.emitRaw("    " + mangledName + " =l alloc4 4");
        } else if (size == 8) {
            builder_.emitRaw("    " + mangledName + " =l alloc8 8");
        } else {
            builder_.emitRaw("    " + mangledName + " =l alloc8 " + std::to_string(size));
        }
        
        // Initialize to zero (BASIC variables are implicitly initialized)
        if (typeManager_.isString(varType)) {
            // Strings initialized to null pointer
            builder_.emitRaw("    storel 0, " + mangledName);
        } else if (size == 4) {
            builder_.emitRaw("    storew 0, " + mangledName);
        } else if (size == 8) {
            builder_.emitRaw("    storel 0, " + mangledName);
        }
    }
}

void ASTEmitter::emitDimStatement(const DimStatement* stmt) {
    // DIM statement: allocate array descriptors and initialize them
    // Note: DIM can also declare scalar variables, which we skip here
    
    for (const auto& arrayDecl : stmt->arrays) {
        const std::string& arrayName = arrayDecl.name;
        
        // Handle scalar variables (those without dimensions)
        if (arrayDecl.dimensions.empty()) {
            builder_.emitComment("DIM scalar variable: " + arrayName);
            
            // Check if it's a local variable - if so, allocate stack space
            const auto& symbolTable = semantic_.getSymbolTable();
            auto varIt = symbolTable.variables.find(arrayName);
            if (varIt != symbolTable.variables.end() && !varIt->second.isGlobal) {
                // Local variable - allocate on stack
                std::string mangledName = symbolMapper_.mangleVariableName(arrayName, false);
                BaseType varType = varIt->second.typeDesc.baseType;
                std::string qbeType = typeManager_.getQBEType(varType);
                int64_t size = typeManager_.getTypeSize(varType);
                
                if (size == 4) {
                    builder_.emitRaw("    " + mangledName + " =l alloc4 4");
                } else if (size == 8) {
                    builder_.emitRaw("    " + mangledName + " =l alloc8 8");
                } else {
                    builder_.emitRaw("    " + mangledName + " =l alloc8 " + std::to_string(size));
                }
            }
            continue;
        }
        
        // Look up array symbol in semantic analyzer
        const auto& symbolTable = semantic_.getSymbolTable();
        auto it = symbolTable.arrays.find(arrayName);
        if (it == symbolTable.arrays.end()) {
            builder_.emitComment("ERROR: array not found in symbol table: " + arrayName);
            continue;
        }
        
        const auto& arraySymbol = it->second;
        BaseType elemType = arraySymbol.elementTypeDesc.baseType;
        int64_t elementSize = typeManager_.getTypeSize(elemType);
        
        // Determine if array is global or local
        bool isGlobal = arraySymbol.functionScope.empty();
        
        // Get mangled array descriptor name
        std::string descName = symbolMapper_.getArrayDescriptorName(arrayName);
        if (isGlobal && descName[0] != '$') {
            descName = "$" + descName;
        } else if (!isGlobal && descName[0] != '%') {
            descName = "%" + descName;
        }
        
        builder_.emitComment("DIM " + arrayName + " - allocate descriptor and data");
        
        // Allocate array descriptor (64 bytes)
        std::string descPtr = descName;
        if (!isGlobal) {
            descPtr = builder_.newTemp();
            builder_.emitAlloc(descPtr, 64);
        }
        
        // Determine array dimensions (1D or 2D)
        int numDims = arraySymbol.dimensions.size();
        
        if (numDims == 1) {
            // 1D array
            // dimensions[0] contains the count (upper_bound + 1), so subtract 1 to get actual upper bound
            int64_t upperBound = arraySymbol.dimensions[0] - 1;
            int64_t lowerBound = 0; // OPTION BASE 0 for now
            int64_t count = upperBound - lowerBound + 1;
            
            // Calculate total size in bytes
            std::string countReg = builder_.newTemp();
            builder_.emitInstruction(countReg + " =l copy " + std::to_string(count));
            
            std::string totalBytes = builder_.newTemp();
            builder_.emitBinary(totalBytes, "l", "mul", countReg, std::to_string(elementSize));
            
            // Allocate data with malloc
            std::string dataPtr = builder_.newTemp();
            builder_.emitCall(dataPtr, "l", "malloc", "l " + totalBytes);
            
            // Zero-initialize data with memset
            builder_.emitCall("", "", "memset", "l " + dataPtr + ", w 0, l " + totalBytes);
            
            // Initialize descriptor fields
            // Offset 0: data pointer
            builder_.emitStore("l", dataPtr, descPtr);
            
            // Offset 8: lowerBound1
            std::string lowerAddr = builder_.newTemp();
            builder_.emitBinary(lowerAddr, "l", "add", descPtr, "8");
            builder_.emitStore("l", std::to_string(lowerBound), lowerAddr);
            
            // Offset 16: upperBound1
            std::string upperAddr = builder_.newTemp();
            builder_.emitBinary(upperAddr, "l", "add", descPtr, "16");
            builder_.emitStore("l", std::to_string(upperBound), upperAddr);
            
            // Offset 24: lowerBound2 (0 for 1D)
            std::string lower2Addr = builder_.newTemp();
            builder_.emitBinary(lower2Addr, "l", "add", descPtr, "24");
            builder_.emitStore("l", "0", lower2Addr);
            
            // Offset 32: upperBound2 (0 for 1D)
            std::string upper2Addr = builder_.newTemp();
            builder_.emitBinary(upper2Addr, "l", "add", descPtr, "32");
            builder_.emitStore("l", "0", upper2Addr);
            
            // Offset 40: elementSize
            std::string elemSizeAddr = builder_.newTemp();
            builder_.emitBinary(elemSizeAddr, "l", "add", descPtr, "40");
            builder_.emitStore("l", std::to_string(elementSize), elemSizeAddr);
            
            // Offset 48: dimensions (1)
            std::string dimsAddr = builder_.newTemp();
            builder_.emitBinary(dimsAddr, "l", "add", descPtr, "48");
            builder_.emitStore("w", "1", dimsAddr);
            
            // Offset 52: base (0)
            std::string baseAddr = builder_.newTemp();
            builder_.emitBinary(baseAddr, "l", "add", descPtr, "52");
            builder_.emitStore("w", "0", baseAddr);
            
        } else if (numDims == 2) {
            // 2D array
            // dimensions contain counts (upper_bound + 1), so subtract 1 to get actual upper bounds
            int64_t upperBound1 = arraySymbol.dimensions[0] - 1;
            int64_t upperBound2 = arraySymbol.dimensions[1] - 1;
            int64_t lowerBound1 = 0;
            int64_t lowerBound2 = 0;
            int64_t count1 = upperBound1 - lowerBound1 + 1;
            int64_t count2 = upperBound2 - lowerBound2 + 1;
            int64_t totalCount = count1 * count2;
            
            // Calculate total size in bytes
            std::string countReg = builder_.newTemp();
            builder_.emitInstruction(countReg + " =l copy " + std::to_string(totalCount));
            
            std::string totalBytes = builder_.newTemp();
            builder_.emitBinary(totalBytes, "l", "mul", countReg, std::to_string(elementSize));
            
            // Allocate data with malloc
            std::string dataPtr = builder_.newTemp();
            builder_.emitCall(dataPtr, "l", "malloc", "l " + totalBytes);
            
            // Zero-initialize data
            builder_.emitCall("", "", "memset", "l " + dataPtr + ", w 0, l " + totalBytes);
            
            // Initialize descriptor fields
            builder_.emitStore("l", dataPtr, descPtr);
            
            std::string lowerAddr1 = builder_.newTemp();
            builder_.emitBinary(lowerAddr1, "l", "add", descPtr, "8");
            builder_.emitStore("l", std::to_string(lowerBound1), lowerAddr1);
            
            std::string upperAddr1 = builder_.newTemp();
            builder_.emitBinary(upperAddr1, "l", "add", descPtr, "16");
            builder_.emitStore("l", std::to_string(upperBound1), upperAddr1);
            
            std::string lowerAddr2 = builder_.newTemp();
            builder_.emitBinary(lowerAddr2, "l", "add", descPtr, "24");
            builder_.emitStore("l", std::to_string(lowerBound2), lowerAddr2);
            
            std::string upperAddr2 = builder_.newTemp();
            builder_.emitBinary(upperAddr2, "l", "add", descPtr, "32");
            builder_.emitStore("l", std::to_string(upperBound2), upperAddr2);
            
            std::string elemSizeAddr = builder_.newTemp();
            builder_.emitBinary(elemSizeAddr, "l", "add", descPtr, "40");
            builder_.emitStore("l", std::to_string(elementSize), elemSizeAddr);
            
            std::string dimsAddr = builder_.newTemp();
            builder_.emitBinary(dimsAddr, "l", "add", descPtr, "48");
            builder_.emitStore("w", "2", dimsAddr);
            
            std::string baseAddr = builder_.newTemp();
            builder_.emitBinary(baseAddr, "l", "add", descPtr, "52");
            builder_.emitStore("w", "0", baseAddr);
            
        } else {
            builder_.emitComment("ERROR: Only 1D and 2D arrays supported, found " + std::to_string(numDims) + "D");
        }
        
        builder_.emitBlankLine();
    }
}

void ASTEmitter::emitCallStatement(const CallStatement* stmt) {
    // Get the mangled SUB name
    std::string mangledName = symbolMapper_.mangleSubName(stmt->subName);
    
    // Evaluate all arguments
    std::vector<std::string> argTemps;
    std::vector<BaseType> argTypes;
    
    for (const auto& arg : stmt->arguments) {
        BaseType argType = getExpressionType(arg.get());
        std::string argTemp = emitExpression(arg.get());
        argTemps.push_back(argTemp);
        argTypes.push_back(argType);
    }
    
    // Build argument list string for QBE call
    std::string args;
    for (size_t i = 0; i < argTemps.size(); ++i) {
        if (i > 0) args += ", ";
        std::string qbeType = typeManager_.getQBEType(argTypes[i]);
        args += qbeType + " " + argTemps[i];
    }
    
    // Strip leading $ from mangled name since emitCall adds it
    std::string callName = mangledName;
    if (!callName.empty() && callName[0] == '$') {
        callName = callName.substr(1);
    }
    
    // Emit the call (SUBs return void, so no destination)
    builder_.emitCall("", "", callName, args);
}

// === Variable Access ===

std::string ASTEmitter::getVariableAddress(const std::string& varName) {
    // Look up variable with scoped lookup
    std::string currentFunc = symbolMapper_.getCurrentFunction();
    const auto* varSymbolPtr = semantic_.lookupVariableScoped(varName, currentFunc);
    if (!varSymbolPtr) {
        builder_.emitComment("ERROR: variable not found: " + varName);
        return builder_.newTemp();
    }
    const auto& varSymbol = *varSymbolPtr;
    
    // Check if we're in a function and the variable is SHARED
    // SHARED variables in functions should use the global symbol
    bool treatAsGlobal = varSymbol.isGlobal || 
                         (symbolMapper_.inFunctionScope() && symbolMapper_.isSharedVariable(varName));
    
    // Mangle the variable name properly (strips type suffixes, sanitizes, etc.)
    std::string mangledName = symbolMapper_.mangleVariableName(varName, treatAsGlobal);
    
    if (treatAsGlobal) {
        // Cache the address
        if (globalVarAddresses_.find(mangledName) == globalVarAddresses_.end()) {
            globalVarAddresses_[mangledName] = mangledName;
        }
        
        return mangledName;
    } else {
        // Local variable
        return mangledName;
    }
}

std::string ASTEmitter::loadVariable(const std::string& varName) {
    // Check if this is a function parameter FIRST - parameters are passed as QBE temporaries
    // and don't need to be loaded from memory
    if (symbolMapper_.inFunctionScope() && symbolMapper_.isParameter(varName)) {
        // Parameter - return the parameter temporary directly (e.g., %a_INT)
        builder_.emitComment("Loading parameter: " + varName);
        return "%" + varName;
    }
    
    BaseType varType = getVariableType(varName);
    std::string qbeType = typeManager_.getQBEType(varType);
    
    // Look up variable with scoped lookup
    std::string currentFunc = symbolMapper_.getCurrentFunction();
    const auto* varSymbolPtr = semantic_.lookupVariableScoped(varName, currentFunc);
    if (!varSymbolPtr) {
        builder_.emitComment("ERROR: variable not found: " + varName);
        return builder_.newTemp();
    }
    const auto& varSymbol = *varSymbolPtr;
    
    // Check if we're in a function and the variable is SHARED
    bool treatAsGlobal = varSymbol.isGlobal || 
                         (symbolMapper_.inFunctionScope() && symbolMapper_.isSharedVariable(varName));
    
    // All variables (global and local) are stored in memory and must be loaded
    std::string addr = getVariableAddress(varName);
    std::string result = builder_.newTemp();
    
    if (treatAsGlobal) {
        // Global variable - load from global memory
        builder_.emitLoad(result, qbeType, addr);
    } else {
        // Local variable - load from stack allocation
        builder_.emitLoad(result, qbeType, addr);
    }
    
    return result;
}

void ASTEmitter::storeVariable(const std::string& varName, const std::string& value) {
    BaseType varType = getVariableType(varName);
    std::string qbeType = typeManager_.getQBEType(varType);
    
    // Check if this is a function parameter
    // In BASIC, parameters can be modified (pass-by-reference semantics)
    if (symbolMapper_.inFunctionScope() && symbolMapper_.isParameter(varName)) {
        // Parameter - need to allocate stack space and copy parameter value there
        // Then update all references to use the stack location
        // For now, we'll treat parameters as modifiable temporaries
        builder_.emitComment("WARNING: Modifying parameter " + varName + " (using copy assignment)");
        builder_.emitRaw("    %" + varName + " =" + qbeType + " copy " + value);
        return;
    }
    
    // Look up variable with scoped lookup
    std::string currentFunc = symbolMapper_.getCurrentFunction();
    const auto* varSymbolPtr = semantic_.lookupVariableScoped(varName, currentFunc);
    if (!varSymbolPtr) {
        builder_.emitComment("ERROR: variable not found: " + varName);
        return;
    }
    const auto& varSymbol = *varSymbolPtr;
    
    // Check if we're in a function and the variable is SHARED
    bool treatAsGlobal = varSymbol.isGlobal || 
                         (symbolMapper_.inFunctionScope() && symbolMapper_.isSharedVariable(varName));
    
    // All variables (global and local) are stored in memory
    std::string addr = getVariableAddress(varName);
    
    if (treatAsGlobal) {
        // Global variable - store to global memory
        builder_.emitStore(qbeType, value, addr);
    } else {
        // Local variable - store to stack allocation
        builder_.emitStore(qbeType, value, addr);
    }
}

// === Array Access ===

std::string ASTEmitter::emitArrayAccess(const std::string& arrayName,
                                        const std::vector<ExpressionPtr>& indices) {
    // Look up array symbol
    const auto& symbolTable = semantic_.getSymbolTable();
    auto it = symbolTable.arrays.find(arrayName);
    if (it == symbolTable.arrays.end()) {
        builder_.emitComment("ERROR: array not found: " + arrayName);
        return builder_.newTemp();
    }
    
    const auto& arraySymbol = it->second;
    BaseType elemType = arraySymbol.elementTypeDesc.baseType;
    int64_t elementSize = typeManager_.getTypeSize(elemType);
    
    // Get array descriptor pointer
    bool isGlobal = arraySymbol.functionScope.empty();
    std::string descName = symbolMapper_.getArrayDescriptorName(arrayName);
    if (isGlobal && descName[0] != '$') {
        descName = "$" + descName;
    } else if (!isGlobal && descName[0] != '%') {
        descName = "%" + descName;
    }
    
    int numIndices = indices.size();
    
    if (numIndices == 1) {
        // 1D array access
        builder_.emitComment("Array access: " + arrayName + "[i]");
        
        // Evaluate index expression
        std::string indexReg = emitExpression(indices[0].get());
        
        // Convert index to long if needed
        std::string indexLong = builder_.newTemp();
        std::string indexType = typeManager_.getQBEType(getExpressionType(indices[0].get()));
        if (indexType == "w") {
            builder_.emitExtend(indexLong, "l", "extsw", indexReg);
        } else {
            builder_.emitInstruction(indexLong + " =l copy " + indexReg);
        }
        
        // Load bounds from descriptor
        std::string lowerAddr = builder_.newTemp();
        builder_.emitBinary(lowerAddr, "l", "add", descName, "8");
        std::string lowerBound = builder_.newTemp();
        builder_.emitLoad(lowerBound, "l", lowerAddr);
        
        std::string upperAddr = builder_.newTemp();
        builder_.emitBinary(upperAddr, "l", "add", descName, "16");
        std::string upperBound = builder_.newTemp();
        builder_.emitLoad(upperBound, "l", upperAddr);
        
        // Bounds check: index >= lowerBound AND index <= upperBound
        std::string checkLower = builder_.newTemp();
        builder_.emitCompare(checkLower, "l", "sge", indexLong, lowerBound);
        
        std::string checkUpper = builder_.newTemp();
        builder_.emitCompare(checkUpper, "l", "sle", indexLong, upperBound);
        
        std::string checkBoth = builder_.newTemp();
        builder_.emitBinary(checkBoth, "w", "and", checkLower, checkUpper);
        
        // Generate bounds check labels
        std::string okLabel = symbolMapper_.getUniqueLabel("bounds_ok");
        std::string errLabel = symbolMapper_.getUniqueLabel("bounds_err");
        
        builder_.emitBranch(checkBoth, okLabel, errLabel);
        
        // Error path
        builder_.emitLabel(errLabel);
        builder_.emitCall("", "", "basic_array_bounds_error", 
                         "l " + indexLong + ", l " + lowerBound + ", l " + upperBound);
        
        // OK path: calculate element address
        builder_.emitLabel(okLabel);
        std::string adjustedIdx = builder_.newTemp();
        builder_.emitBinary(adjustedIdx, "l", "sub", indexLong, lowerBound);
        
        std::string byteOffset = builder_.newTemp();
        builder_.emitBinary(byteOffset, "l", "mul", adjustedIdx, std::to_string(elementSize));
        
        std::string dataPtr = builder_.newTemp();
        builder_.emitLoad(dataPtr, "l", descName);
        
        std::string elementPtr = builder_.newTemp();
        builder_.emitBinary(elementPtr, "l", "add", dataPtr, byteOffset);
        
        return elementPtr;
        
    } else if (numIndices == 2) {
        // 2D array access
        builder_.emitComment("Array access: " + arrayName + "[i,j]");
        
        // Evaluate index expressions
        std::string index1Reg = emitExpression(indices[0].get());
        std::string index2Reg = emitExpression(indices[1].get());
        
        // Convert indices to long
        std::string index1Long = builder_.newTemp();
        std::string indexType1 = typeManager_.getQBEType(getExpressionType(indices[0].get()));
        if (indexType1 == "w") {
            builder_.emitExtend(index1Long, "l", "extsw", index1Reg);
        } else {
            builder_.emitInstruction(index1Long + " =l copy " + index1Reg);
        }
        
        std::string index2Long = builder_.newTemp();
        std::string indexType2 = typeManager_.getQBEType(getExpressionType(indices[1].get()));
        if (indexType2 == "w") {
            builder_.emitExtend(index2Long, "l", "extsw", index2Reg);
        } else {
            builder_.emitInstruction(index2Long + " =l copy " + index2Reg);
        }
        
        // Load bounds for dimension 1
        std::string lowerAddr1 = builder_.newTemp();
        builder_.emitBinary(lowerAddr1, "l", "add", descName, "8");
        std::string lowerBound1 = builder_.newTemp();
        builder_.emitLoad(lowerBound1, "l", lowerAddr1);
        
        std::string upperAddr1 = builder_.newTemp();
        builder_.emitBinary(upperAddr1, "l", "add", descName, "16");
        std::string upperBound1 = builder_.newTemp();
        builder_.emitLoad(upperBound1, "l", upperAddr1);
        
        // Load bounds for dimension 2
        std::string lowerAddr2 = builder_.newTemp();
        builder_.emitBinary(lowerAddr2, "l", "add", descName, "24");
        std::string lowerBound2 = builder_.newTemp();
        builder_.emitLoad(lowerBound2, "l", lowerAddr2);
        
        std::string upperAddr2 = builder_.newTemp();
        builder_.emitBinary(upperAddr2, "l", "add", descName, "32");
        std::string upperBound2 = builder_.newTemp();
        builder_.emitLoad(upperBound2, "l", upperAddr2);
        
        // Bounds check dimension 1
        std::string checkLower1 = builder_.newTemp();
        builder_.emitCompare(checkLower1, "l", "sge", index1Long, lowerBound1);
        
        std::string checkUpper1 = builder_.newTemp();
        builder_.emitCompare(checkUpper1, "l", "sle", index1Long, upperBound1);
        
        std::string checkBoth1 = builder_.newTemp();
        builder_.emitBinary(checkBoth1, "w", "and", checkLower1, checkUpper1);
        
        // Bounds check dimension 2
        std::string checkLower2 = builder_.newTemp();
        builder_.emitCompare(checkLower2, "l", "sge", index2Long, lowerBound2);
        
        std::string checkUpper2 = builder_.newTemp();
        builder_.emitCompare(checkUpper2, "l", "sle", index2Long, upperBound2);
        
        std::string checkBoth2 = builder_.newTemp();
        builder_.emitBinary(checkBoth2, "w", "and", checkLower2, checkUpper2);
        
        // Combined check
        std::string checkAll = builder_.newTemp();
        builder_.emitBinary(checkAll, "w", "and", checkBoth1, checkBoth2);
        
        std::string okLabel = symbolMapper_.getUniqueLabel("bounds_ok");
        std::string errLabel = symbolMapper_.getUniqueLabel("bounds_err");
        
        builder_.emitBranch(checkAll, okLabel, errLabel);
        
        // Error path (simplified - just use first index for error message)
        builder_.emitLabel(errLabel);
        builder_.emitCall("", "", "basic_array_bounds_error", 
                         "l " + index1Long + ", l " + lowerBound1 + ", l " + upperBound1);
        
        // OK path: calculate element address
        // Row-major: offset = ((i - lower1) * dim2_size + (j - lower2)) * elemSize
        builder_.emitLabel(okLabel);
        
        std::string adjustedIdx1 = builder_.newTemp();
        builder_.emitBinary(adjustedIdx1, "l", "sub", index1Long, lowerBound1);
        
        std::string adjustedIdx2 = builder_.newTemp();
        builder_.emitBinary(adjustedIdx2, "l", "sub", index2Long, lowerBound2);
        
        // Calculate dim2_size = upperBound2 - lowerBound2 + 1
        std::string dim2Size = builder_.newTemp();
        builder_.emitBinary(dim2Size, "l", "sub", upperBound2, lowerBound2);
        std::string dim2SizePlus1 = builder_.newTemp();
        builder_.emitBinary(dim2SizePlus1, "l", "add", dim2Size, "1");
        
        // Calculate linear offset
        std::string rowOffset = builder_.newTemp();
        builder_.emitBinary(rowOffset, "l", "mul", adjustedIdx1, dim2SizePlus1);
        
        std::string linearIdx = builder_.newTemp();
        builder_.emitBinary(linearIdx, "l", "add", rowOffset, adjustedIdx2);
        
        std::string byteOffset = builder_.newTemp();
        builder_.emitBinary(byteOffset, "l", "mul", linearIdx, std::to_string(elementSize));
        
        std::string dataPtr = builder_.newTemp();
        builder_.emitLoad(dataPtr, "l", descName);
        
        std::string elementPtr = builder_.newTemp();
        builder_.emitBinary(elementPtr, "l", "add", dataPtr, byteOffset);
        
        return elementPtr;
        
    } else {
        builder_.emitComment("ERROR: Only 1D and 2D arrays supported");
        return builder_.newTemp();
    }
}

std::string ASTEmitter::loadArrayElement(const std::string& arrayName,
                                         const std::vector<ExpressionPtr>& indices) {
    std::string elemAddr = emitArrayAccess(arrayName, indices);
    
    // Get array element type
    const auto& symbolTable = semantic_.getSymbolTable();
    auto it = symbolTable.arrays.find(arrayName);
    if (it == symbolTable.arrays.end()) {
        builder_.emitComment("ERROR: array not found: " + arrayName);
        return builder_.newTemp();
    }
    const auto& arraySymbol = it->second;
    
    BaseType elemType = arraySymbol.elementTypeDesc.baseType;
    std::string qbeType = typeManager_.getQBEType(elemType);
    
    std::string result = builder_.newTemp();
    builder_.emitLoad(result, qbeType, elemAddr);
    
    return result;
}

void ASTEmitter::storeArrayElement(const std::string& arrayName,
                                   const std::vector<ExpressionPtr>& indices,
                                   const std::string& value) {
    std::string elemAddr = emitArrayAccess(arrayName, indices);
    
    // Get array element type
    const auto& symbolTable = semantic_.getSymbolTable();
    auto it = symbolTable.arrays.find(arrayName);
    if (it == symbolTable.arrays.end()) {
        builder_.emitComment("ERROR: array not found: " + arrayName);
        return;
    }
    const auto& arraySymbol = it->second;
    
    BaseType elemType = arraySymbol.elementTypeDesc.baseType;
    std::string qbeType = typeManager_.getQBEType(elemType);
    
    builder_.emitStore(qbeType, value, elemAddr);
}

// === Type Inference ===

BaseType ASTEmitter::getExpressionType(const Expression* expr) {
    if (!expr) {
        return BaseType::UNKNOWN;
    }
    
    switch (expr->getType()) {
        case ASTNodeType::EXPR_NUMBER: {
            const auto* numExpr = static_cast<const NumberExpression*>(expr);
            // Check if it's an integer
            if (numExpr->value == std::floor(numExpr->value) && 
                numExpr->value >= INT32_MIN && numExpr->value <= INT32_MAX) {
                return BaseType::INTEGER;
            } else {
                return BaseType::DOUBLE;
            }
        }
        
        case ASTNodeType::EXPR_STRING:
            return BaseType::STRING;
            
        case ASTNodeType::EXPR_VARIABLE: {
            const auto* varExpr = static_cast<const VariableExpression*>(expr);
            return getVariableType(varExpr->name);
        }
        
        case ASTNodeType::EXPR_BINARY: {
            const auto* binExpr = static_cast<const BinaryExpression*>(expr);
            BaseType leftType = getExpressionType(binExpr->left.get());
            BaseType rightType = getExpressionType(binExpr->right.get());
            
            // String operations always return string
            if (typeManager_.isString(leftType) || typeManager_.isString(rightType)) {
                return BaseType::STRING;
            }
            
            // Comparison operations return INTEGER (boolean)
            if (binExpr->op >= TokenType::EQUAL && binExpr->op <= TokenType::GREATER_EQUAL) {
                return BaseType::INTEGER;
            }
            
            // Arithmetic operations promote to common type
            return typeManager_.getPromotedType(leftType, rightType);
        }
        
        case ASTNodeType::EXPR_UNARY: {
            const auto* unaryExpr = static_cast<const UnaryExpression*>(expr);
            if (unaryExpr->op == TokenType::NOT) {
                return BaseType::INTEGER;  // Logical NOT returns boolean
            }
            return getExpressionType(unaryExpr->expr.get());
        }
        
        case ASTNodeType::EXPR_ARRAY_ACCESS: {
            const auto* arrExpr = static_cast<const ArrayAccessExpression*>(expr);
            const auto& symbolTable = semantic_.getSymbolTable();
            auto it = symbolTable.arrays.find(arrExpr->name);
            if (it == symbolTable.arrays.end()) {
                return BaseType::UNKNOWN;
            }
            const auto& arraySymbol = it->second;
            return arraySymbol.elementTypeDesc.baseType;
            return BaseType::UNKNOWN;
        }
        
        case ASTNodeType::EXPR_IIF: {
            const auto* iifExpr = static_cast<const IIFExpression*>(expr);
            // IIF result type is the promoted type of true/false branches
            BaseType trueType = getExpressionType(iifExpr->trueValue.get());
            BaseType falseType = getExpressionType(iifExpr->falseValue.get());
            return typeManager_.getPromotedType(trueType, falseType);
        }
        
        default:
            return BaseType::UNKNOWN;
    }
}

BaseType ASTEmitter::getVariableType(const std::string& varName) {
    // Use scoped lookup for variable type
    std::string currentFunc = symbolMapper_.getCurrentFunction();
    const auto* varSymbol = semantic_.lookupVariableScoped(varName, currentFunc);
    if (!varSymbol) {
        return BaseType::UNKNOWN;
    }
    
    return varSymbol->typeDesc.baseType;
}



// === Helper: get QBE operator names ===

std::string ASTEmitter::getQBEArithmeticOp(TokenType op) {
    switch (op) {
        case TokenType::PLUS:       return "add";
        case TokenType::MINUS:      return "sub";
        case TokenType::MULTIPLY:   return "mul";
        case TokenType::DIVIDE:     return "div";
        case TokenType::INT_DIVIDE: return "div";  // Integer division (same as regular div for now)
        case TokenType::MOD:        return "rem";
        case TokenType::POWER:      return "pow";  // TODO: implement power as runtime call
        default:                    return "add";
    }
}

std::string ASTEmitter::getQBEComparisonOp(TokenType op) {
    switch (op) {
        case TokenType::EQUAL:         return "eq";
        case TokenType::NOT_EQUAL:     return "ne";
        case TokenType::LESS_THAN:     return "slt";
        case TokenType::LESS_EQUAL:    return "sle";
        case TokenType::GREATER_THAN:  return "sgt";
        case TokenType::GREATER_EQUAL: return "sge";
        default:                       return "eq";
    }
}

// === FOR Loop Helpers ===

void ASTEmitter::emitForInit(const ForStatement* stmt) {
    // FOR loop initialization: evaluate start, limit, and step ONCE
    // All values must be treated as integers (BASIC requirement)
    
    // 1. Evaluate and store start value to loop variable
    std::string startValue = emitExpressionAs(stmt->start.get(), BaseType::INTEGER);
    storeVariable(stmt->variable, startValue);
    
    // 2. Allocate and initialize limit variable (constant during loop)
    std::string limitVar = "__for_limit_" + stmt->variable;
    std::string limitAddr = builder_.newTemp();
    builder_.emitRaw("    " + limitAddr + " =l alloc4 4");
    std::string limitValue = emitExpressionAs(stmt->end.get(), BaseType::INTEGER);
    builder_.emitRaw("    storew " + limitValue + ", " + limitAddr);
    
    // Store the address for later use (we'll use a map)
    forLoopTempAddresses_[limitVar] = limitAddr;
    
    // 3. Allocate and initialize step variable (constant during loop, default to 1)
    std::string stepVar = "__for_step_" + stmt->variable;
    std::string stepAddr = builder_.newTemp();
    builder_.emitRaw("    " + stepAddr + " =l alloc4 4");
    std::string stepValue;
    if (stmt->step) {
        stepValue = emitExpressionAs(stmt->step.get(), BaseType::INTEGER);
    } else {
        // Default step is 1
        stepValue = builder_.newTemp();
        builder_.emitRaw("    " + stepValue + " =w copy 1");
    }
    builder_.emitRaw("    storew " + stepValue + ", " + stepAddr);
    forLoopTempAddresses_[stepVar] = stepAddr;
}

std::string ASTEmitter::emitForCondition(const ForStatement* stmt) {
    // FOR loop condition: check if loop should continue
    // Load loop variable and limit, load step, and do simple comparison
    
    // Load loop variable (may have been modified in loop body)
    std::string loopVar = loadVariable(stmt->variable);
    
    // Load limit (constant, evaluated once at init)
    std::string limitVar = "__for_limit_" + stmt->variable;
    std::string limitAddr = forLoopTempAddresses_[limitVar];
    std::string limitValue = builder_.newTemp();
    builder_.emitRaw("    " + limitValue + " =w loadw " + limitAddr);
    
    // Load step value to check sign
    std::string stepVar = "__for_step_" + stmt->variable;
    std::string stepAddr = forLoopTempAddresses_[stepVar];
    std::string stepValue = builder_.newTemp();
    builder_.emitRaw("    " + stepValue + " =w loadw " + stepAddr);
    
    // Check if step is negative
    std::string stepIsNeg = builder_.newTemp();
    builder_.emitRaw("    " + stepIsNeg + " =w csltw " + stepValue + ", 0");
    
    // For positive step: continue while loopVar <= limit
    // For negative step: continue while loopVar >= limit
    // We compute both and select based on sign
    
    // Positive case: loopVar <= limit is !(loopVar > limit)
    std::string loopGtLimit = builder_.newTemp();
    builder_.emitRaw("    " + loopGtLimit + " =w csgtw " + loopVar + ", " + limitValue);
    std::string posCondition = builder_.newTemp();
    builder_.emitRaw("    " + posCondition + " =w xor " + loopGtLimit + ", 1");
    
    // Negative case: loopVar >= limit is !(loopVar < limit)
    std::string loopLtLimit = builder_.newTemp();
    builder_.emitRaw("    " + loopLtLimit + " =w csltw " + loopVar + ", " + limitValue);
    std::string negCondition = builder_.newTemp();
    builder_.emitRaw("    " + negCondition + " =w xor " + loopLtLimit + ", 1");
    
    // Select: if stepIsNeg then negCondition else posCondition
    // Use arithmetic: result = stepIsNeg * negCondition + (1 - stepIsNeg) * posCondition
    std::string negPart = builder_.newTemp();
    builder_.emitRaw("    " + negPart + " =w and " + stepIsNeg + ", " + negCondition);
    std::string notStepIsNeg = builder_.newTemp();
    builder_.emitRaw("    " + notStepIsNeg + " =w xor " + stepIsNeg + ", 1");
    std::string posPart = builder_.newTemp();
    builder_.emitRaw("    " + posPart + " =w and " + notStepIsNeg + ", " + posCondition);
    std::string result = builder_.newTemp();
    builder_.emitRaw("    " + result + " =w or " + negPart + ", " + posPart);
    
    return result;
}

void ASTEmitter::emitForIncrement(const ForStatement* stmt) {
    // FOR loop increment: add step to loop variable
    // Step was evaluated once at init and is constant
    
    // Load current loop variable value (may have been modified in body)
    std::string loopVar = loadVariable(stmt->variable);
    
    // Load step value (constant, evaluated once at init)
    std::string stepVar = "__for_step_" + stmt->variable;
    std::string stepAddr = forLoopTempAddresses_[stepVar];
    std::string stepValue = builder_.newTemp();
    builder_.emitRaw("    " + stepValue + " =w loadw " + stepAddr);
    
    // Increment: var = var + step
    std::string newValue = builder_.newTemp();
    builder_.emitBinary(newValue, "w", "add", loopVar, stepValue);
    
    // Store back to variable
    storeVariable(stmt->variable, newValue);
}

std::string ASTEmitter::emitIfCondition(const IfStatement* stmt) {
    return emitExpression(stmt->condition.get());
}

std::string ASTEmitter::emitWhileCondition(const WhileStatement* stmt) {
    return emitExpression(stmt->condition.get());
}

void ASTEmitter::emitReadStatement(const ReadStatement* stmt) {
    builder_.emitComment("READ statement");
    
    // For each variable in the READ list
    for (const auto& varName : stmt->variables) {
        // Determine variable type
        BaseType varType = getVariableType(varName);
        std::string qbeType = typeManager_.getQBEType(varType);
        
        // Generate inline READ with type checking
        // 1. Load current data pointer
        std::string ptrReg = builder_.getNextTemp();
        builder_.emitLoad(ptrReg, "l", "$__data_pointer");
        
        // 2. Check if exhausted
        std::string endReg = builder_.getNextTemp();
        builder_.emitLoad(endReg, "l", "$__data_end_const");
        std::string exhaustedReg = builder_.getNextTemp();
        builder_.emitCompare(exhaustedReg, "l", "eq", ptrReg, endReg);
        
        std::string errorLabel = "data_exhausted_" + std::to_string(builder_.getNextLabelId());
        std::string okLabel = "read_ok_" + std::to_string(builder_.getNextLabelId());
        builder_.emitBranch(exhaustedReg, errorLabel, okLabel);
        
        // Error block
        builder_.emitLabel(errorLabel);
        builder_.emitCall("", "", "fb_error_out_of_data", "");
        builder_.emitCall("", "", "exit", "w 1");
        
        // OK block
        builder_.emitLabel(okLabel);
        
        // 3. Calculate data index: (ptr - start) / 8
        std::string startReg = builder_.getNextTemp();
        builder_.emitLoad(startReg, "l", "$__data_start");
        std::string offsetReg = builder_.getNextTemp();
        builder_.emitBinary(offsetReg, "l", "sub", ptrReg, startReg);
        std::string indexReg = builder_.getNextTemp();
        builder_.emitBinary(indexReg, "l", "div", offsetReg, "8");
        
        // 4. Load type tag: __data_types[index]
        // Calculate address: $data_type_0 + index*4 (type tags are words)
        std::string typeBaseReg = builder_.getNextTemp();
        builder_.emitInstruction(typeBaseReg + " =l copy $data_type_0");
        std::string typeOffsetReg = builder_.getNextTemp();
        builder_.emitBinary(typeOffsetReg, "l", "mul", indexReg, "4");
        std::string typeAddrReg = builder_.getNextTemp();
        builder_.emitBinary(typeAddrReg, "l", "add", typeBaseReg, typeOffsetReg);
        std::string typeTagReg = builder_.getNextTemp();
        builder_.emitLoad(typeTagReg, "w", typeAddrReg);
        
        // 5. Load the data value (always as long first)
        std::string dataValueReg = builder_.getNextTemp();
        builder_.emitLoad(dataValueReg, "l", ptrReg);
        
        // 6. Generate type switch based on target variable type
        std::string finalValueReg = builder_.getNextTemp();
        
        if (qbeType == "w") {
            // Target is int (w) - check source type and convert
            // If type == 0 (int): truncate long to int
            // If type == 1 (double): reinterpret bits as double, convert to int
            // If type == 2 (string): error
            builder_.emitComment("Convert DATA to int");
            builder_.emitInstruction(finalValueReg + " =w copy " + dataValueReg);
            
        } else if (qbeType == "d") {
            // Target is double (d) - check source type and convert
            // If type == 0 (int): convert long to double
            // If type == 1 (double): reinterpret bits as double
            // If type == 2 (string): error
            builder_.emitComment("Convert DATA to double");
            builder_.emitInstruction(finalValueReg + " =d cast " + dataValueReg);
            
        } else if (qbeType == "s") {
            // Target is single (s) - similar to double
            builder_.emitComment("Convert DATA to single");
            builder_.emitInstruction(finalValueReg + " =s cast " + dataValueReg);
            
        } else if (qbeType == "l" && typeManager_.isString(varType)) {
            // Target is string (l) - just copy pointer
            builder_.emitComment("Copy DATA string pointer");
            finalValueReg = dataValueReg;  // Already correct type
            
        } else if (qbeType == "l") {
            // Target is long (l) - just copy
            builder_.emitComment("Copy DATA as long");
            finalValueReg = dataValueReg;  // Already correct type
            
        } else {
            builder_.emitComment("ERROR: unsupported QBE type for READ: " + qbeType);
            continue;
        }
        
        // Store to variable
        storeVariable(varName, finalValueReg);
        
        // 7. Advance pointer by 8 bytes
        std::string newPtrReg = builder_.getNextTemp();
        builder_.emitBinary(newPtrReg, "l", "add", ptrReg, "8");
        builder_.emitStore("l", newPtrReg, "$__data_pointer");
    }
}

void ASTEmitter::emitRestoreStatement(const RestoreStatement* stmt) {
    if (stmt->isLabel) {
        // RESTORE label_name
        builder_.emitComment("RESTORE " + stmt->label);
        std::string labelPos = "$data_label_" + stmt->label;
        std::string posReg = builder_.getNextTemp();
        builder_.emitLoad(posReg, "l", labelPos);
        builder_.emitCall("", "", "fb_restore_to_label", "l " + posReg);
        
    } else if (stmt->lineNumber > 0) {
        // RESTORE line_number
        builder_.emitComment("RESTORE " + std::to_string(stmt->lineNumber));
        std::string linePos = "$data_line_" + std::to_string(stmt->lineNumber);
        std::string posReg = builder_.getNextTemp();
        builder_.emitLoad(posReg, "l", linePos);
        builder_.emitCall("", "", "fb_restore_to_line", "l " + posReg);
        
    } else {
        // RESTORE with no argument - reset to start
        builder_.emitComment("RESTORE to start");
        builder_.emitCall("", "", "fb_restore", "");
    }
}

} // namespace fbc