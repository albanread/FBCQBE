#include "ast_emitter.h"
#include <sstream>
#include <cmath>
#include <iostream>
#include <fstream>

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
    
    if (upperName == "UCASE" || upperName == "UCASE$") {
        // UCASE$(string$) - convert to uppercase
        if (expr->arguments.size() != 1) {
            builder_.emitComment("ERROR: UCASE$ requires exactly 1 argument");
            return "0";
        }
        std::string strArg = emitExpression(expr->arguments[0].get());
        return runtime_.emitUCase(strArg);
    }
    
    if (upperName == "LCASE" || upperName == "LCASE$") {
        // LCASE$(string$) - convert to lowercase
        if (expr->arguments.size() != 1) {
            builder_.emitComment("ERROR: LCASE$ requires exactly 1 argument");
            return "0";
        }
        std::string strArg = emitExpression(expr->arguments[0].get());
        return runtime_.emitLCase(strArg);
    }
    
    if (upperName == "__STRING_SLICE") {
        // __STRING_SLICE(string$, start, end) - internal slice operation
        // Used by parser for slice syntax: text$(start TO end)
        if (expr->arguments.size() != 3) {
            builder_.emitComment("ERROR: __STRING_SLICE requires exactly 3 arguments");
            return "0";
        }
        
        std::string strArg = emitExpression(expr->arguments[0].get());
        std::string startArg = emitExpression(expr->arguments[1].get());
        std::string endArg = emitExpression(expr->arguments[2].get());
        
        // Convert start and end to long if needed
        BaseType startType = getExpressionType(expr->arguments[1].get());
        BaseType endType = getExpressionType(expr->arguments[2].get());
        
        if (typeManager_.isIntegral(startType) && typeManager_.getQBEType(startType) == "w") {
            std::string startLong = builder_.newTemp();
            builder_.emitExtend(startLong, "l", "extsw", startArg);
            startArg = startLong;
        } else if (typeManager_.isFloatingPoint(startType)) {
            startArg = emitTypeConversion(startArg, startType, BaseType::LONG);
        }
        
        if (typeManager_.isIntegral(endType) && typeManager_.getQBEType(endType) == "w") {
            std::string endLong = builder_.newTemp();
            builder_.emitExtend(endLong, "l", "extsw", endArg);
            endArg = endLong;
        } else if (typeManager_.isFloatingPoint(endType)) {
            endArg = emitTypeConversion(endArg, endType, BaseType::LONG);
        }
        
        // Call string_slice runtime function
        std::string result = builder_.newTemp();
        builder_.emitCall(result, "l", "string_slice", "l " + strArg + ", l " + startArg + ", l " + endArg);
        return result;
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
    // Special case: POWER operator needs to call pow() runtime function
    if (op == TokenType::POWER) {
        std::string result = builder_.newTemp();
        
        // Convert operands to double for pow() call
        std::string leftDouble = left;
        std::string rightDouble = right;
        
        if (type != BaseType::DOUBLE) {
            // Convert to double
            if (typeManager_.isIntegral(type)) {
                leftDouble = builder_.newTemp();
                rightDouble = builder_.newTemp();
                builder_.emitInstruction(leftDouble + " =d swtof " + left);
                builder_.emitInstruction(rightDouble + " =d swtof " + right);
            } else if (type == BaseType::SINGLE) {
                leftDouble = builder_.newTemp();
                rightDouble = builder_.newTemp();
                builder_.emitInstruction(leftDouble + " =d exts " + left);
                builder_.emitInstruction(rightDouble + " =d exts " + right);
            }
        }
        
        // Call pow(double, double) -> double
        std::string powResult = builder_.newTemp();
        builder_.emitCall(powResult, "d", "pow", "d " + leftDouble + ", d " + rightDouble);
        
        // Convert result back to original type if needed
        if (type == BaseType::INTEGER || type == BaseType::UINTEGER) {
            builder_.emitInstruction(result + " =w dtosi " + powResult);
        } else if (type == BaseType::LONG || type == BaseType::ULONG) {
            builder_.emitInstruction(result + " =l dtosi " + powResult);
        } else if (type == BaseType::SINGLE) {
            builder_.emitInstruction(result + " =s truncd " + powResult);
        } else {
            result = powResult;  // Already double
        }
        
        return result;
    }
    
    // Regular arithmetic operations
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
    
    // Handle special two-step conversions for double/float to long
    if (convOp == "DOUBLE_TO_LONG") {
        // QBE doesn't have direct double→long, must go double→int→long
        std::string intTemp = builder_.newTemp();
        builder_.emitConvert(intTemp, "w", "dtosi", value);
        
        std::string result = builder_.newTemp();
        builder_.emitConvert(result, "l", "extsw", intTemp);
        return result;
    }
    
    if (convOp == "FLOAT_TO_LONG") {
        // QBE doesn't have direct float→long, must go float→int→long
        std::string intTemp = builder_.newTemp();
        builder_.emitConvert(intTemp, "w", "stosi", value);
        
        std::string result = builder_.newTemp();
        builder_.emitConvert(result, "l", "extsw", intTemp);
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
            
        case ASTNodeType::STMT_REDIM:
            emitRedimStatement(static_cast<const RedimStatement*>(stmt));
            break;
            
        case ASTNodeType::STMT_ERASE:
            emitEraseStatement(static_cast<const EraseStatement*>(stmt));
            break;
            
        case ASTNodeType::STMT_FOR:
            emitForInit(static_cast<const ForStatement*>(stmt));
            break;
            
        case ASTNodeType::STMT_WHILE:
            // WHILE condition is handled by CFG edges
            builder_.emitComment("WHILE loop header");
            break;
            
        case ASTNodeType::STMT_DO:
            // DO condition is handled by CFG edges
            builder_.emitComment("DO loop header");
            break;
            
        case ASTNodeType::STMT_LOOP:
            // LOOP condition is handled by CFG edges
            builder_.emitComment("LOOP statement");
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
            
        case ASTNodeType::STMT_SLICE_ASSIGN:
            emitSliceAssignStatement(static_cast<const SliceAssignStatement*>(stmt));
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
    // Determine target type based on whether it's an array or scalar
    BaseType targetType;
    
    if (!stmt->indices.empty()) {
        // Array assignment: get element type from array descriptor
        const auto& symbolTable = semantic_.getSymbolTable();
        auto it = symbolTable.arrays.find(stmt->variable);
        if (it != symbolTable.arrays.end()) {
            targetType = it->second.elementTypeDesc.baseType;
        } else {
            targetType = BaseType::UNKNOWN;
        }
    } else {
        // Scalar assignment: get variable type
        targetType = getVariableType(stmt->variable);
    }
    
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
    // DIM statement: allocate arrays using runtime array_new() function
    // Note: DIM can also declare scalar variables, which we skip here
    
    for (const auto& arrayDecl : stmt->arrays) {
        const std::string& arrayName = arrayDecl.name;
        
        // Handle scalar variables (those without dimensions)
        if (arrayDecl.dimensions.empty()) {
            builder_.emitComment("DIM scalar variable: " + arrayName);
            
            // NOTE: Local scalar variables are already allocated at function entry
            // in CFGEmitter::emitBlock for block 0. We don't need to allocate them again.
            // DIM for scalars is essentially a no-op in terms of codegen (declaration only).
            
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
        
        // Determine if array is global or local
        bool isGlobal = arraySymbol.functionScope.empty();
        
        // Get mangled array descriptor name
        std::string descName = symbolMapper_.getArrayDescriptorName(arrayName);
        if (isGlobal && descName[0] != '$') {
            descName = "$" + descName;
        } else if (!isGlobal && descName[0] != '%') {
            descName = "%" + descName;
        }
        
        builder_.emitComment("DIM " + arrayName + " - call array_new()");
        
        // Get type suffix character for runtime
        char typeSuffix = getTypeSuffixChar(elemType);
        
        // Determine number of dimensions
        int numDims = arraySymbol.dimensions.size();
        
        if (numDims < 1 || numDims > 8) {
            builder_.emitComment("ERROR: Invalid array dimensions: " + std::to_string(numDims));
            continue;
        }
        
        // Allocate bounds array on stack: [lower1, upper1, lower2, upper2, ...]
        std::string boundsArrayPtr = builder_.newTemp();
        int boundsSize = numDims * 2 * 4;  // 2 bounds per dimension, 4 bytes each (int32_t)
        builder_.emitAlloc(boundsArrayPtr, boundsSize);
        
        // Fill in bounds array
        for (int i = 0; i < numDims; i++) {
            // Lower bound (always 0 for OPTION BASE 0)
            int64_t lowerBound = 0;
            std::string lowerAddr = builder_.newTemp();
            int lowerOffset = i * 2 * 4;
            builder_.emitBinary(lowerAddr, "l", "add", boundsArrayPtr, std::to_string(lowerOffset));
            builder_.emitStore("w", std::to_string(lowerBound), lowerAddr);
            
            // Upper bound (dimensions[i] - 1)
            int64_t upperBound = arraySymbol.dimensions[i] - 1;
            std::string upperAddr = builder_.newTemp();
            int upperOffset = (i * 2 + 1) * 4;
            builder_.emitBinary(upperAddr, "l", "add", boundsArrayPtr, std::to_string(upperOffset));
            builder_.emitStore("w", std::to_string(upperBound), upperAddr);
        }
        
        // Call array_new(char type_suffix, int32_t dimensions, int32_t* bounds, int32_t base)
        std::string typeSuffixReg = builder_.newTemp();
        builder_.emitInstruction(typeSuffixReg + " =w copy " + std::to_string((int)typeSuffix));
        
        std::string dimsReg = builder_.newTemp();
        builder_.emitInstruction(dimsReg + " =w copy " + std::to_string(numDims));
        
        std::string baseReg = builder_.newTemp();
        builder_.emitInstruction(baseReg + " =w copy 0");  // OPTION BASE 0
        
        std::string arrayPtr = builder_.newTemp();
        builder_.emitCall(arrayPtr, "l", "array_new", 
                         "w " + typeSuffixReg + 
                         ", w " + dimsReg + 
                         ", l " + boundsArrayPtr + 
                         ", w " + baseReg);
        
        // Store the BasicArray* pointer in the array variable
        builder_.emitStore("l", arrayPtr, descName);
    }
}

void ASTEmitter::emitRedimStatement(const RedimStatement* stmt) {
    // REDIM statement: resize existing array (with or without PRESERVE)
    
    for (const auto& arrayDecl : stmt->arrays) {
        const std::string& arrayName = arrayDecl.name;
        
        builder_.emitComment("REDIM" + std::string(stmt->preserve ? " PRESERVE " : " ") + arrayName);
        
        // Look up array symbol in semantic analyzer
        const auto& symbolTable = semantic_.getSymbolTable();
        auto it = symbolTable.arrays.find(arrayName);
        if (it == symbolTable.arrays.end()) {
            builder_.emitComment("ERROR: array not found in symbol table: " + arrayName);
            continue;
        }
        
        const auto& arraySymbol = it->second;
        
        // Get the array descriptor pointer (the array variable itself)
        std::string descName = symbolMapper_.getArrayDescriptorName(arrayName);
        bool isGlobal = arraySymbol.functionScope.empty();
        if (isGlobal && descName[0] != '$') {
            descName = "$" + descName;
        } else if (!isGlobal && descName[0] != '%') {
            descName = "%" + descName;
        }
        
        // Evaluate dimension expressions to get new bounds
        std::vector<std::string> newBounds;
        for (const auto& dimExpr : arrayDecl.dimensions) {
            std::string upperBound = emitExpressionAs(dimExpr.get(), BaseType::LONG);
            newBounds.push_back(upperBound);
        }
        
        // Allocate bounds array: [lower1, upper1, lower2, upper2, ...]
        int numDims = newBounds.size();
        std::string boundsArraySize = std::to_string(numDims * 2 * 4); // 2 int32_t per dimension
        std::string boundsPtr = builder_.newTemp();
        builder_.emitCall(boundsPtr, "l", "malloc", "l " + boundsArraySize);
        
        // Fill in bounds array
        int32_t lowerBound = 0; // OPTION BASE 0 for now
        for (int i = 0; i < numDims; i++) {
            // Convert upper bound from long to word if needed
            std::string upperBoundWord = builder_.newTemp();
            builder_.emitInstruction(upperBoundWord + " =w copy " + newBounds[i]);
            
            // Store lower bound
            std::string lowerAddr = builder_.newTemp();
            builder_.emitBinary(lowerAddr, "l", "add", boundsPtr, std::to_string(i * 2 * 4));
            builder_.emitStore("w", std::to_string(lowerBound), lowerAddr);
            
            // Store upper bound
            std::string upperAddr = builder_.newTemp();
            builder_.emitBinary(upperAddr, "l", "add", boundsPtr, std::to_string((i * 2 + 1) * 4));
            builder_.emitStore("w", upperBoundWord, upperAddr);
        }
        
        // Load the BasicArray* pointer from the descriptor variable
        std::string arrayPtr = builder_.newTemp();
        builder_.emitLoad(arrayPtr, "l", descName);
        
        // Call array_redim(array, new_bounds, preserve)
        std::string preserveFlag = stmt->preserve ? "1" : "0";
        builder_.emitCall("", "", "array_redim", "l " + arrayPtr + ", l " + boundsPtr + ", w " + preserveFlag);
        
        // Free the temporary bounds array
        builder_.emitCall("", "", "free", "l " + boundsPtr);
        
        builder_.emitBlankLine();
    }
}

void ASTEmitter::emitEraseStatement(const EraseStatement* stmt) {
    // ERASE statement: deallocate array memory
    
    for (const std::string& arrayName : stmt->arrayNames) {
        builder_.emitComment("ERASE " + arrayName);
        
        // Look up array symbol in semantic analyzer
        const auto& symbolTable = semantic_.getSymbolTable();
        auto it = symbolTable.arrays.find(arrayName);
        if (it == symbolTable.arrays.end()) {
            builder_.emitComment("ERROR: array not found in symbol table: " + arrayName);
            continue;
        }
        
        const auto& arraySymbol = it->second;
        
        // Get the array descriptor pointer
        std::string descName = symbolMapper_.getArrayDescriptorName(arrayName);
        bool isGlobal = arraySymbol.functionScope.empty();
        if (isGlobal && descName[0] != '$') {
            descName = "$" + descName;
        } else if (!isGlobal && descName[0] != '%') {
            descName = "%" + descName;
        }
        
        // Load the BasicArray* pointer from the descriptor variable
        std::string arrayPtr = builder_.newTemp();
        builder_.emitLoad(arrayPtr, "l", descName);
        
        // Call array_erase(array)
        builder_.emitCall("", "", "array_erase", "l " + arrayPtr);
        
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

// Helper to normalize FOR loop variable names
// If varName references a FOR loop variable (by base name), returns the normalized name
// with the correct integer suffix. Otherwise returns varName unchanged.
std::string ASTEmitter::normalizeForLoopVarName(const std::string& varName) const {
    if (varName.empty()) return varName;
    
    // Debug
    std::ofstream debugFile("/tmp/for_debug.txt", std::ios::app);
    debugFile << "[DEBUG normalize] Input varName: '" << varName << "'" << std::endl;
    
    // Strip any existing suffix to get base name (handle both character and text suffixes)
    std::string baseName = varName;
    
    // Check for text suffixes first (from parser mangling)
    if (baseName.length() > 4 && baseName.substr(baseName.length() - 4) == "_INT") {
        baseName = baseName.substr(0, baseName.length() - 4);
    } else if (baseName.length() > 5 && baseName.substr(baseName.length() - 5) == "_LONG") {
        baseName = baseName.substr(0, baseName.length() - 5);
    } else if (baseName.length() > 7 && baseName.substr(baseName.length() - 7) == "_STRING") {
        baseName = baseName.substr(0, baseName.length() - 7);
    } else if (baseName.length() > 7 && baseName.substr(baseName.length() - 7) == "_DOUBLE") {
        baseName = baseName.substr(0, baseName.length() - 7);
    } else if (baseName.length() > 6 && baseName.substr(baseName.length() - 6) == "_FLOAT") {
        baseName = baseName.substr(0, baseName.length() - 6);
    } else if (baseName.length() > 5 && baseName.substr(baseName.length() - 5) == "_BYTE") {
        baseName = baseName.substr(0, baseName.length() - 5);
    } else if (baseName.length() > 6 && baseName.substr(baseName.length() - 6) == "_SHORT") {
        baseName = baseName.substr(0, baseName.length() - 6);
    } else {
        // Check for character suffixes (if not already converted by parser)
        char lastChar = baseName.back();
        if (lastChar == '%' || lastChar == '&' || lastChar == '!' || 
            lastChar == '#' || lastChar == '$' || lastChar == '@' || lastChar == '^') {
            baseName = baseName.substr(0, baseName.length() - 1);
        }
    }
    
    // Check if this base name is a FOR loop variable
    debugFile << "[DEBUG normalize] Stripped baseName: '" << baseName << "'" << std::endl;
    debugFile << "[DEBUG normalize] isForLoopVariable(baseName): " << semantic_.isForLoopVariable(baseName) << std::endl;
    
    if (semantic_.isForLoopVariable(baseName)) {
        // This is a FOR loop variable - return base name with correct integer suffix
        // The suffix is determined by OPTION FOR setting
        // Use text suffix format (_INT or _LONG) to match parser mangling
        std::string intSuffix = semantic_.getForLoopIntegerSuffix();
        std::string result = baseName + intSuffix;
        debugFile << "[DEBUG normalize] Normalized to: '" << result << "'" << std::endl;
        debugFile.close();
        return result;
    }
    
    // Not a FOR loop variable - return original name unchanged
    debugFile << "[DEBUG normalize] NOT a FOR var, returning unchanged: '" << varName << "'" << std::endl;
    debugFile.close();
    return varName;
}

std::string ASTEmitter::getVariableAddress(const std::string& varName) {
    // For FOR loop variables, normalize to correct integer suffix
    // The semantic analyzer has already declared the variable with the normalized name
    std::string lookupName = normalizeForLoopVarName(varName);
    
    // Debug
    std::ofstream debugFile("/tmp/for_debug.txt", std::ios::app);
    debugFile << "[DEBUG getVariableAddress] varName='" << varName << "' lookupName='" << lookupName << "'" << std::endl;
    
    // Look up variable with scoped lookup
    std::string currentFunc = symbolMapper_.getCurrentFunction();
    debugFile << "[DEBUG getVariableAddress] currentFunc='" << currentFunc << "'" << std::endl;
    
    const auto* varSymbolPtr = semantic_.lookupVariableScoped(lookupName, currentFunc);
    if (!varSymbolPtr) {
        debugFile << "[DEBUG getVariableAddress] LOOKUP FAILED for '" << lookupName << "'" << std::endl;
        debugFile.close();
        builder_.emitComment("ERROR: variable not found: " + lookupName);
        return builder_.newTemp();
    }
    debugFile << "[DEBUG getVariableAddress] LOOKUP SUCCESS" << std::endl;
    debugFile.close();
    const auto& varSymbol = *varSymbolPtr;
    
    // Check if we're in a function and the variable is SHARED
    bool isShared = symbolMapper_.isSharedVariable(lookupName);
    bool isParameter = symbolMapper_.isParameter(lookupName);
    bool treatAsGlobal = (varSymbol.isGlobal || isShared || isParameter);
    
    // Mangle the variable name properly (strips type suffixes, sanitizes, etc.)
    // Use lookupName (with suffix stripped for FOR loop vars)
    std::string mangledName = symbolMapper_.mangleVariableName(lookupName, treatAsGlobal);
    
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
    // For FOR loop variables, normalize to correct integer suffix
    std::string lookupName = normalizeForLoopVarName(varName);
    
    // Check if this is a function parameter FIRST - parameters are passed as QBE temporaries
    // and don't need to be loaded from memory
    if (symbolMapper_.inFunctionScope() && symbolMapper_.isParameter(lookupName)) {
        // Parameter - return the parameter temporary directly (e.g., %a_INT)
        builder_.emitComment("Loading parameter: " + lookupName);
        return "%" + lookupName;
    }
    
    BaseType varType = getVariableType(lookupName);
    std::string qbeType = typeManager_.getQBEType(varType);
    
    // Look up variable with scoped lookup
    std::string currentFunc = symbolMapper_.getCurrentFunction();
    const auto* varSymbolPtr = semantic_.lookupVariableScoped(lookupName, currentFunc);
    if (!varSymbolPtr) {
        builder_.emitComment("ERROR: variable not found: " + lookupName);
        return builder_.newTemp();
    }
    const auto& varSymbol = *varSymbolPtr;
    
    // Check if we're in a function and the variable is SHARED
    bool treatAsGlobal = varSymbol.isGlobal || 
                         (symbolMapper_.inFunctionScope() && symbolMapper_.isSharedVariable(lookupName));
    
    // All variables (global and local) are stored in memory and must be loaded
    std::string addr = getVariableAddress(lookupName);
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
    // For FOR loop variables, normalize to correct integer suffix
    std::string lookupName = normalizeForLoopVarName(varName);
    
    BaseType varType = getVariableType(lookupName);
    std::string qbeType = typeManager_.getQBEType(varType);
    
    // Check if this is a function parameter
    // In BASIC, parameters can be modified (pass-by-reference semantics)
    if (symbolMapper_.inFunctionScope() && symbolMapper_.isParameter(lookupName)) {
        // Parameter - need to allocate stack space and copy parameter value there
        // Then update all references to use the stack location
        // For now, we'll treat parameters as modifiable temporaries
        builder_.emitComment("WARNING: Modifying parameter " + lookupName + " (using copy assignment)");
        builder_.emitRaw("    %" + varName + " =" + qbeType + " copy " + value);
        return;
    }
    
    // Look up variable with scoped lookup
    std::string currentFunc = symbolMapper_.getCurrentFunction();
    const auto* varSymbolPtr = semantic_.lookupVariableScoped(lookupName, currentFunc);
    if (!varSymbolPtr) {
        builder_.emitComment("ERROR: variable not found: " + lookupName);
        return;
    }
    const auto& varSymbol = *varSymbolPtr;
    
    // Check if we're in a function and the variable is SHARED
    bool treatAsGlobal = varSymbol.isGlobal || 
                         (symbolMapper_.inFunctionScope() && symbolMapper_.isSharedVariable(lookupName));
    
    // All variables (global and local) are stored in memory
    std::string addr = getVariableAddress(lookupName);
    
    // *** STRING ASSIGNMENT WITH REFERENCE COUNTING ***
    // Strings require special handling to prevent memory leaks and ensure
    // proper reference counting semantics
    if (typeManager_.isString(varType)) {
        builder_.emitComment("String assignment: " + varName + " = <value>");
        
        // 1. Load old string pointer from variable
        std::string oldPtr = builder_.newTemp();
        builder_.emitLoad(oldPtr, "l", addr);
        
        // 2. Retain new string (increments refcount)
        //    This shares ownership - the variable now references the same descriptor
        std::string retainedPtr = builder_.newTemp();
        builder_.emitCall(retainedPtr, "l", "string_retain", "l " + value);
        
        // 3. Store new pointer to variable
        builder_.emitStore("l", retainedPtr, addr);
        
        // 4. Release old string (decrements refcount, frees if 0)
        //    Done AFTER storing new value to handle self-assignment correctly
        //    string_release handles NULL pointers gracefully
        builder_.emitCall("", "", "string_release", "l " + oldPtr);
        
        builder_.emitComment("End string assignment");
    } else {
        // Non-string types: regular store (no reference counting needed)
        if (treatAsGlobal) {
            // Global variable - store to global memory
            builder_.emitStore(qbeType, value, addr);
        } else {
            // Local variable - store to stack allocation
            builder_.emitStore(qbeType, value, addr);
        }
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
    
    // Get array descriptor pointer (now a BasicArray*)
    bool isGlobal = arraySymbol.functionScope.empty();
    std::string descName = symbolMapper_.getArrayDescriptorName(arrayName);
    if (isGlobal && descName[0] != '$') {
        descName = "$" + descName;
    } else if (!isGlobal && descName[0] != '%') {
        descName = "%" + descName;
    }
    
    int numIndices = indices.size();
    builder_.emitComment("Array access: " + arrayName + " (using array_get_address)");
    
    // Load the BasicArray* pointer
    std::string arrayPtr = builder_.newTemp();
    builder_.emitLoad(arrayPtr, "l", descName);
    
    // Allocate space for indices array on stack (int32_t per index)
    std::string indicesArrayPtr = builder_.newTemp();
    int indicesSize = numIndices * 4;  // 4 bytes per int32_t
    builder_.emitAlloc(indicesArrayPtr, indicesSize);
    
    // Store each index into the indices array
    for (int i = 0; i < numIndices; i++) {
        // Evaluate index expression
        std::string indexReg = emitExpression(indices[i].get());
        
        // Convert index to int32_t (word) if needed
        std::string indexWord = builder_.newTemp();
        std::string indexType = typeManager_.getQBEType(getExpressionType(indices[i].get()));
        if (indexType == "l") {
            // Truncate long to int
            builder_.emitInstruction(indexWord + " =w copy " + indexReg);
        } else {
            builder_.emitInstruction(indexWord + " =w copy " + indexReg);
        }
        
        // Store into indices array at offset i*4
        std::string indexAddr = builder_.newTemp();
        int offset = i * 4;
        if (offset == 0) {
            builder_.emitInstruction(indexAddr + " =l copy " + indicesArrayPtr);
        } else {
            builder_.emitBinary(indexAddr, "l", "add", indicesArrayPtr, std::to_string(offset));
        }
        builder_.emitStore("w", indexWord, indexAddr);
    }
    
    // Call array_get_address(BasicArray* array, int32_t* indices)
    std::string elementPtr = builder_.newTemp();
    builder_.emitCall(elementPtr, "l", "array_get_address", 
                     "l " + arrayPtr + ", l " + indicesArrayPtr);
    
    return elementPtr;
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
        
        case ASTNodeType::EXPR_FUNCTION_CALL: {
            const auto* callExpr = static_cast<const FunctionCallExpression*>(expr);
            
            // Look up function in symbol table to get return type
            const auto& symbolTable = semantic_.getSymbolTable();
            auto it = symbolTable.functions.find(callExpr->name);
            if (it != symbolTable.functions.end()) {
                return it->second.returnTypeDesc.baseType;
            }
            
            // Check for intrinsic functions
            std::string upperName = callExpr->name;
            std::transform(upperName.begin(), upperName.end(), upperName.begin(), ::toupper);
            
            // String functions
            if (upperName.back() == '$' || upperName == "CHR" || upperName == "STR" || 
                upperName == "LEFT" || upperName == "RIGHT" || upperName == "MID" ||
                upperName == "SPACE" || upperName == "STRING" || upperName == "UCASE" || 
                upperName == "LCASE" || upperName == "TRIM" || upperName == "LTRIM" || 
                upperName == "RTRIM" || upperName == "__STRING_SLICE") {
                return BaseType::STRING;
            }
            
            // Integer functions
            if (upperName == "LEN" || upperName == "ASC" || upperName == "INSTR" ||
                upperName == "INT" || upperName == "FIX" || upperName == "SGN" ||
                upperName == "CINT" || upperName == "ERR" || upperName == "ERL") {
                return BaseType::INTEGER;
            }
            
            // ABS returns same type as argument
            if (upperName == "ABS" && callExpr->arguments.size() == 1) {
                return getExpressionType(callExpr->arguments[0].get());
            }
            
            // Floating point math functions
            if (upperName == "SIN" || upperName == "COS" || upperName == "TAN" ||
                upperName == "SQRT" || upperName == "SQR" || upperName == "LOG" || 
                upperName == "EXP" || upperName == "RND" || upperName == "VAL") {
                return BaseType::DOUBLE;
            }
            
            // Default to DOUBLE for unknown functions
            return BaseType::DOUBLE;
        }
        
        default:
            return BaseType::UNKNOWN;
    }
}

BaseType ASTEmitter::getVariableType(const std::string& varName) {
    // Check if this is a parameter first - get type from function symbol
    if (symbolMapper_.inFunctionScope() && symbolMapper_.isParameter(varName)) {
        std::string currentFunc = symbolMapper_.getCurrentFunction();
        const auto& symbolTable = semantic_.getSymbolTable();
        auto it = symbolTable.functions.find(currentFunc);
        if (it != symbolTable.functions.end()) {
            const auto& funcSymbol = it->second;
            // Find the parameter index
            for (size_t i = 0; i < funcSymbol.parameters.size(); ++i) {
                if (funcSymbol.parameters[i] == varName) {
                    BaseType paramType = funcSymbol.parameterTypeDescs[i].baseType;
                    return paramType;
                }
            }
        }
    }
    
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

std::string ASTEmitter::emitDoPreCondition(const DoStatement* stmt) {
    if (stmt->preConditionType == DoStatement::ConditionType::NONE) {
        return "";  // No pre-condition
    }
    
    if (!stmt->preCondition) {
        return "";  // Shouldn't happen, but handle gracefully
    }
    
    // Just emit the condition - CFG has already set up edges correctly
    // For DO WHILE: true → body, false → exit
    // For DO UNTIL: true → exit, false → body (CFG reverses edges)
    return emitExpression(stmt->preCondition.get());
}

std::string ASTEmitter::emitLoopPostCondition(const LoopStatement* stmt) {
    if (stmt->conditionType == LoopStatement::ConditionType::NONE) {
        return "";  // No post-condition
    }
    
    if (!stmt->condition) {
        return "";  // Shouldn't happen, but handle gracefully
    }
    
    // Just emit the condition - CFG has already set up edges correctly
    // For LOOP WHILE: true → body, false → exit
    // For LOOP UNTIL: true → exit, false → body (CFG reverses edges)
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
            // Target is string (l) - convert C string pointer to StringDescriptor
            builder_.emitComment("Convert DATA C string to StringDescriptor");
            std::string strDescReg = builder_.getNextTemp();
            builder_.emitCall(strDescReg, "l", "string_new_utf8", "l " + dataValueReg);
            finalValueReg = strDescReg;
            
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
        builder_.emitStore("l", posReg, "$__data_pointer");
        
    } else if (stmt->lineNumber > 0) {
        // RESTORE line_number
        builder_.emitComment("RESTORE " + std::to_string(stmt->lineNumber));
        std::string linePos = "$data_line_" + std::to_string(stmt->lineNumber);
        std::string posReg = builder_.getNextTemp();
        builder_.emitLoad(posReg, "l", linePos);
        builder_.emitStore("l", posReg, "$__data_pointer");
        
    } else {
        // RESTORE with no argument - reset to start
        builder_.emitComment("RESTORE to start");
        std::string startReg = builder_.getNextTemp();
        builder_.emitLoad(startReg, "l", "$__data_start");
        builder_.emitStore("l", startReg, "$__data_pointer");
    }
}

void ASTEmitter::emitSliceAssignStatement(const SliceAssignStatement* stmt) {
    if (!stmt || !stmt->start || !stmt->end || !stmt->replacement) {
        builder_.emitComment("ERROR: invalid slice assignment");
        return;
    }
    
    builder_.emitComment("String slice assignment: " + stmt->variable + "$(start TO end) = value");
    
    // Get the variable address
    std::string varAddr = getVariableAddress(stmt->variable);
    
    // Load current string pointer
    std::string currentPtr = builder_.newTemp();
    builder_.emitLoad(currentPtr, "l", varAddr);
    
    // Evaluate start, end, and replacement expressions
    std::string startReg = emitExpression(stmt->start.get());
    std::string endReg = emitExpression(stmt->end.get());
    std::string replReg = emitExpression(stmt->replacement.get());
    
    // Convert start and end to long if needed
    BaseType startType = getExpressionType(stmt->start.get());
    BaseType endType = getExpressionType(stmt->end.get());
    
    if (typeManager_.isIntegral(startType) && typeManager_.getQBEType(startType) == "w") {
        std::string startLong = builder_.newTemp();
        builder_.emitExtend(startLong, "l", "extsw", startReg);
        startReg = startLong;
    } else if (typeManager_.isFloatingPoint(startType)) {
        startReg = emitTypeConversion(startReg, startType, BaseType::LONG);
    }
    
    if (typeManager_.isIntegral(endType) && typeManager_.getQBEType(endType) == "w") {
        std::string endLong = builder_.newTemp();
        builder_.emitExtend(endLong, "l", "extsw", endReg);
        endReg = endLong;
    } else if (typeManager_.isFloatingPoint(endType)) {
        endReg = emitTypeConversion(endReg, endType, BaseType::LONG);
    }
    
    // Call string_slice_assign - it handles copy-on-write and returns modified/new descriptor
    // IMPORTANT: string_slice_assign manages its own memory:
    //   - If refcount > 1: clones, decrements original
    //   - If same length: modifies in place
    //   - If different length: creates new, frees old
    // So we don't release the old pointer - the function handles it
    std::string resultPtr = builder_.newTemp();
    builder_.emitCall(resultPtr, "l", "string_slice_assign", 
                     "l " + currentPtr + ", l " + startReg + ", l " + endReg + ", l " + replReg);
    
    // Store the result back to the variable
    builder_.emitStore("l", resultPtr, varAddr);
    
    builder_.emitComment("End slice assignment");
}

// Helper: Convert BaseType to runtime type suffix character
char ASTEmitter::getTypeSuffixChar(BaseType type) {
    switch (type) {
        case BaseType::INTEGER:
        case BaseType::UINTEGER:
            return '%';  // INTEGER
        case BaseType::LONG:
        case BaseType::ULONG:
            return '&';  // LONG
        case BaseType::SINGLE:
            return '!';  // SINGLE
        case BaseType::DOUBLE:
            return '#';  // DOUBLE
        case BaseType::STRING:
            return '$';  // STRING
        default:
            return '#';  // Default to DOUBLE for unknown types
    }
}

} // namespace fbc