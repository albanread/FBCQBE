//
// qbe_codegen_statements.cpp
// FasterBASIC QBE Code Generator - Statement Emission
//
// This file contains all statement emission code:
// - emitStatement() dispatcher
// - PRINT, INPUT, LET
// - IF/THEN/ELSE
// - FOR/NEXT, WHILE/WEND
// - GOTO, GOSUB, RETURN
// - DIM, END, REM, CALL, EXIT
//

#include "../fasterbasic_qbe_codegen.h"

namespace FasterBASIC {

// =============================================================================
// Helper Functions
// =============================================================================

// Sanitize BASIC variable names for QBE - replace type suffix characters with underscores
static std::string sanitizeForQBE(const std::string& name) {
    std::string result = name;
    for (size_t i = 0; i < result.length(); ++i) {
        char c = result[i];
        if (c == '%' || c == '$' || c == '#' || c == '!' || c == '&' || c == '@' || c == '^') {
            result[i] = '_';
        }
    }
    return result;
}

// =============================================================================
// Statement Dispatcher
// =============================================================================

void QBECodeGenerator::emitStatement(const Statement* stmt) {
    if (!stmt) return;
    
    // Dispatch based on AST node type
    ASTNodeType nodeType = stmt->getType();
    
    switch (nodeType) {
        case ASTNodeType::STMT_PRINT:
            emitPrint(static_cast<const PrintStatement*>(stmt));
            break;
            
        case ASTNodeType::STMT_INPUT:
            emitInput(static_cast<const InputStatement*>(stmt));
            break;
            
        case ASTNodeType::STMT_LET:
            emitLet(static_cast<const LetStatement*>(stmt));
            break;
            
        case ASTNodeType::STMT_MID_ASSIGN:
            emitMidAssign(static_cast<const MidAssignStatement*>(stmt));
            break;
            
        case ASTNodeType::STMT_SLICE_ASSIGN:
            emitSliceAssign(static_cast<const SliceAssignStatement*>(stmt));
            break;
            
        case ASTNodeType::STMT_IF:
            emitIf(static_cast<const IfStatement*>(stmt));
            break;
            
        case ASTNodeType::STMT_FOR:
            emitFor(static_cast<const ForStatement*>(stmt));
            break;
            
        case ASTNodeType::STMT_FOR_IN:
            emitForIn(static_cast<const ForInStatement*>(stmt));
            break;
            
        case ASTNodeType::STMT_NEXT:
            emitNext(static_cast<const NextStatement*>(stmt));
            break;
            
        case ASTNodeType::STMT_WHILE:
            emitWhile(static_cast<const WhileStatement*>(stmt));
            break;
            
        case ASTNodeType::STMT_WEND:
            emitWend(static_cast<const WendStatement*>(stmt));
            break;
            
        case ASTNodeType::STMT_GOTO:
            emitGoto(static_cast<const GotoStatement*>(stmt));
            break;
            
        case ASTNodeType::STMT_GOSUB:
            emitGosub(static_cast<const GosubStatement*>(stmt));
            break;
            
        case ASTNodeType::STMT_ON_GOTO:
            emitOnGoto(static_cast<const OnGotoStatement*>(stmt));
            break;
            
        case ASTNodeType::STMT_ON_GOSUB:
            emitOnGosub(static_cast<const OnGosubStatement*>(stmt));
            break;
            
        case ASTNodeType::STMT_LABEL:
            // Labels are just markers - no code generated
            break;
            
        case ASTNodeType::STMT_RETURN:
            emitReturn(static_cast<const ReturnStatement*>(stmt));
            break;
            
        case ASTNodeType::STMT_DIM:
            emitDim(static_cast<const DimStatement*>(stmt));
            break;
            
        case ASTNodeType::STMT_END:
            emitEnd(static_cast<const EndStatement*>(stmt));
            break;
            
        case ASTNodeType::STMT_REM:
            emitRem(static_cast<const RemStatement*>(stmt));
            break;
            
        case ASTNodeType::STMT_CALL:
            emitCall(static_cast<const CallStatement*>(stmt));
            break;
            
        case ASTNodeType::STMT_ERASE:
            emitErase(static_cast<const EraseStatement*>(stmt));
            break;
            
        case ASTNodeType::STMT_REDIM:
            emitRedim(static_cast<const RedimStatement*>(stmt));
            break;
            
        case ASTNodeType::STMT_EXIT:
            emitExit(static_cast<const ExitStatement*>(stmt));
            break;
            
        case ASTNodeType::STMT_REPEAT:
            emitRepeat(static_cast<const RepeatStatement*>(stmt));
            break;
            
        case ASTNodeType::STMT_UNTIL:
            emitUntil(static_cast<const UntilStatement*>(stmt));
            break;
            
        case ASTNodeType::STMT_DO:
            emitDo(static_cast<const DoStatement*>(stmt));
            break;
            
        case ASTNodeType::STMT_LOOP:
            emitLoop(static_cast<const LoopStatement*>(stmt));
            break;
            
        case ASTNodeType::STMT_CASE:
            emitCase(static_cast<const CaseStatement*>(stmt));
            break;
            
        case ASTNodeType::STMT_LOCAL:
            emitLocal(static_cast<const LocalStatement*>(stmt));
            break;
            
        case ASTNodeType::STMT_GLOBAL:
            // GLOBAL declarations are pure compile-time metadata
            // Variables are already registered by semantic analyzer and initialized in main
            // No runtime code needed
            break;
            
        case ASTNodeType::STMT_SHARED:
            emitShared(static_cast<const SharedStatement*>(stmt));
            break;
            
        case ASTNodeType::STMT_TYPE:
            // TYPE declarations are pure compile-time metadata
            // No runtime code needed - handled by semantic analyzer
            break;
            
        case ASTNodeType::STMT_CONSTANT:
            // CONSTANT declarations are pure compile-time metadata
            // No runtime code needed - values inlined at use sites
            break;
            
        case ASTNodeType::STMT_READ:
            emitRead(static_cast<const ReadStatement*>(stmt));
            break;
            
        case ASTNodeType::STMT_RESTORE:
            emitRestore(static_cast<const RestoreStatement*>(stmt));
            break;
            
        case ASTNodeType::STMT_DATA:
            // DATA statements are preprocessed - no runtime code needed
            break;
            
        case ASTNodeType::STMT_TRY_CATCH:
            emitTryCatch(static_cast<const TryCatchStatement*>(stmt));
            break;
            
        case ASTNodeType::STMT_THROW:
            emitThrow(static_cast<const ThrowStatement*>(stmt));
            break;
            
        case ASTNodeType::STMT_CLS:
            emitCls(static_cast<const SimpleStatement*>(stmt));
            break;
            
        case ASTNodeType::STMT_COLOR:
            emitColor(static_cast<const ExpressionStatement*>(stmt));
            break;
            
        case ASTNodeType::STMT_LOCATE:
            emitLocate(static_cast<const ExpressionStatement*>(stmt));
            break;
            
        case ASTNodeType::STMT_WIDTH:
            emitWidth(static_cast<const ExpressionStatement*>(stmt));
            break;
            
        case ASTNodeType::STMT_AT:
            emitLocate(static_cast<const ExpressionStatement*>(stmt));
            break;
            
        default:
            emitComment("TODO: Unhandled statement type " + std::to_string(static_cast<int>(nodeType)));
            break;
    }
}

// =============================================================================
// PRINT Statement
// =============================================================================

void QBECodeGenerator::emitPrint(const PrintStatement* stmt) {
    if (!stmt) return;
    
    if (stmt->hasUsing) {
        // PRINT USING: evaluate format string and arguments (convert non-strings to strings)
        std::string fmtTemp = emitExpression(stmt->formatExpr.get());

        std::vector<std::string> argTemps;
        std::vector<bool> createdTemp;
        for (const auto& valExpr : stmt->usingValues) {
            std::string tmp = emitExpression(valExpr.get());
            VariableType vt = inferExpressionType(valExpr.get());
            if (vt != VariableType::STRING && vt != VariableType::UNICODE) {
                // Convert to string descriptor
                if (vt == VariableType::INT) tmp = emitIntToString(tmp);
                else tmp = emitDoubleToString(tmp);
                createdTemp.push_back(true);
            } else {
                createdTemp.push_back(false);
            }
            argTemps.push_back(tmp);
        }

        // Build array of StringDescriptor* on stack
        // Changed from varargs to array-based approach to avoid ARM64 ABI issues
        if (argTemps.empty()) {
            // No arguments - pass NULL
            emit("    call $basic_print_using(l " + fmtTemp + ", l 0, l 0)\n");
            m_stats.instructionsGenerated++;
        } else {
            // Heap-allocate array of pointers (8 bytes per pointer)
            int64_t arraySize = argTemps.size() * 8;
            std::string arrayPtr = allocTemp("l");
            emit("    " + arrayPtr + " =l call $malloc(l " + std::to_string(arraySize) + ")\n");
            m_stats.instructionsGenerated++;
            
            // Store each descriptor pointer into the array
            for (size_t i = 0; i < argTemps.size(); ++i) {
                if (i == 0) {
                    // First element - store directly at base
                    emit("    storel " + argTemps[i] + ", " + arrayPtr + "\n");
                } else {
                    // Subsequent elements - calculate offset address
                    int64_t offset = i * 8;
                    std::string offsetPtr = allocTemp("l");
                    emit("    " + offsetPtr + " =l add " + arrayPtr + ", " + std::to_string(offset) + "\n");
                    emit("    storel " + argTemps[i] + ", " + offsetPtr + "\n");
                    m_stats.instructionsGenerated++;
                }
                m_stats.instructionsGenerated++;
            }
            
            // Call basic_print_using(format, count, args_array)
            emit("    call $basic_print_using(l " + fmtTemp + ", l " + std::to_string(argTemps.size()) + ", l " + arrayPtr + ")\n");
            m_stats.instructionsGenerated++;
            
            // Free the heap-allocated array
            emit("    call $free(l " + arrayPtr + ")\n");
            m_stats.instructionsGenerated++;
        }

        // NOW release any temporary string descriptors created by conversions
        // (after basic_print_using has extracted the UTF-8 strings)
        for (size_t i = 0; i < argTemps.size(); ++i) {
            if (createdTemp[i]) {
                emit("    call $string_release(l " + argTemps[i] + ")\n");
                m_stats.instructionsGenerated++;
            }
        }

        if (stmt->trailingNewline) emitPrintNewline();
    } else {
        for (const auto& item : stmt->items) {
            if (item.expr) {
                // Evaluate expression and print result
                std::string resultTemp = emitExpression(item.expr.get());
                
                // Infer type from expression
                VariableType exprType = inferExpressionType(item.expr.get());
                emitPrintValue(resultTemp, exprType);
            }
            
            // Handle separators (only between items, not after each item)
            if (item.comma) {
                // Tab separator
                emit("    call $basic_print_tab()\n");
                m_stats.instructionsGenerated++;
            }
            // Semicolon and no separator both mean no extra spacing
        }
        
        // Newline at end unless trailing semicolon
        if (stmt->trailingNewline) {
            emitPrintNewline();
        }
    }
}

// =============================================================================
// INPUT Statement
// =============================================================================

void QBECodeGenerator::emitInput(const InputStatement* stmt) {
    if (!stmt) return;
    
    // Print prompt if present
    if (!stmt->prompt.empty()) {
        std::string promptTemp = emitStringConstant(stmt->prompt);
        emit("    call $basic_print_string(l " + promptTemp + ")\n");
        m_stats.instructionsGenerated++;
    }
    
    // Read input for each variable
    for (const auto& varName : stmt->variables) {
        std::string varRef = getVariableRef(varName);
        VariableType varType = getVariableType(varName);
        
        if (varType == VariableType::STRING) {
            std::string inputTemp = emitInputString();
            emit("    " + varRef + " =l copy " + inputTemp + "\n");
        } else if (varType == VariableType::INT) {
            emit("    " + varRef + " =w call $basic_input_int()\n");
        } else if (varType == VariableType::DOUBLE) {
            emit("    " + varRef + " =d call $basic_input_double()\n");
        }
        m_stats.instructionsGenerated++;
    }
}

// =============================================================================
// LET Statement (Assignment)
// =============================================================================

void QBECodeGenerator::emitLet(const LetStatement* stmt) {
    if (!stmt || !stmt->value) return;
    
    // Evaluate right-hand side
    std::string valueTemp = emitExpression(stmt->value.get());
    
    // Check if this is member assignment (UDT)
    if (!stmt->memberChain.empty()) {
        // Member assignment: Player.Position.X = 100 or points(0).x = 5
        
        std::string baseRef;
        std::string currentTypeName;
        
        // 1. Check if this is an array element or a simple variable
        if (!stmt->indices.empty()) {
            // Array element with member access: points(0).x = 5
            
            // Get array element pointer using array access logic
            auto elemTypeIt = m_arrayElementTypes.find(stmt->variable);
            if (elemTypeIt == m_arrayElementTypes.end()) {
                emitComment("ERROR: Array element member access on non-UDT array: " + stmt->variable);
                return;
            }
            
            // Evaluate array indices
            std::vector<std::string> indexTemps;
            for (const auto& indexExpr : stmt->indices) {
                std::string indexTemp = emitExpression(indexExpr.get());
                VariableType indexType = inferExpressionType(indexExpr.get());
                if (indexType != VariableType::INT) {
                    indexTemp = promoteToType(indexTemp, indexType, VariableType::INT);
                }
                indexTemps.push_back(indexTemp);
            }
            
            // For now, only support 1D arrays
            if (indexTemps.size() != 1) {
                emitComment("ERROR: Multi-dimensional UDT arrays not yet supported");
                return;
            }
            
            // Compute element address using pointer arithmetic
            std::string arrayRef = getArrayRef(stmt->variable);
            currentTypeName = elemTypeIt->second;
            size_t elementSize = calculateTypeSize(currentTypeName);
            
            // Load the data pointer from the descriptor (offset 0)
            std::string dataPtr = allocTemp("l");
            emit("    " + dataPtr + " =l loadl " + arrayRef + "\n");
            m_stats.instructionsGenerated++;
            
            std::string indexTemp = indexTemps[0];
            std::string indexLong = allocTemp("l");
            emit("    " + indexLong + " =l extsw " + indexTemp + "\n");
            m_stats.instructionsGenerated++;
            
            std::string offsetTemp = allocTemp("l");
            emit("    " + offsetTemp + " =l mul " + indexLong + ", " + std::to_string(elementSize) + "\n");
            m_stats.instructionsGenerated++;
            
            baseRef = allocTemp("l");
            emit("    " + baseRef + " =l add " + dataPtr + ", " + offsetTemp + "\n");
            m_stats.instructionsGenerated++;
        } else {
            // Simple variable member access: Player.X = 100
            baseRef = getVariableRef(stmt->variable);
            VariableType baseType = getVariableType(stmt->variable);
            
            if (baseType != VariableType::USER_DEFINED) {
                emitComment("ERROR: Member access on non-UDT variable: " + stmt->variable);
                return;
            }
            
            currentTypeName = getVariableTypeName(stmt->variable);
            if (currentTypeName.empty()) {
                emitComment("ERROR: Could not determine type of variable: " + stmt->variable);
                return;
            }
        }
        
        // 3. Walk member chain to compute final address
        std::string currentPtr = baseRef;
        
        for (size_t i = 0; i < stmt->memberChain.size(); ++i) {
            const std::string& memberName = stmt->memberChain[i];
            size_t offset = calculateFieldOffset(currentTypeName, memberName);
            
            std::string nextPtr = allocTemp("l");
            if (offset == 0) {
                emit("    " + nextPtr + " =l copy " + currentPtr + "\n");
            } else {
                emit("    " + nextPtr + " =l add " + currentPtr + ", " + std::to_string(offset) + "\n");
            }
            m_stats.instructionsGenerated++;
            currentPtr = nextPtr;
            
            // Update type for next iteration (in case of nested UDTs)
            const TypeSymbol* typeSym = getTypeSymbol(currentTypeName);
            if (typeSym) {
                const TypeSymbol::Field* field = typeSym->findField(memberName);
                if (field && !field->isBuiltIn) {
                    currentTypeName = field->typeName;
                }
            }
        }
        
        // 4. Get final field type
        const TypeSymbol* finalTypeSym = getTypeSymbol(currentTypeName);
        if (!finalTypeSym) {
            emitComment("ERROR: Type not found: " + currentTypeName);
            return;
        }
        
        const TypeSymbol::Field* finalField = finalTypeSym->findField(stmt->memberChain.back());
        if (!finalField) {
            emitComment("ERROR: Field not found: " + stmt->memberChain.back());
            return;
        }
        
        // Get field type from TypeDescriptor (correct for INTEGER = 'w' not 'l')
        TypeDescriptor fieldTypeDesc = finalField->typeDesc;
        std::string qbeType = fieldTypeDesc.toQBEType();
        
        // 5. Type conversion if needed
        VariableType exprType = inferExpressionType(stmt->value.get());
        VariableType fieldTypeLegacy = finalField->builtInType;
        
        if (fieldTypeLegacy != exprType) {
            // Convert expression to field type
            // For INTEGER fields, this will convert to 'w' (32-bit)
            std::string convertedTemp = promoteToType(valueTemp, exprType, fieldTypeLegacy);
            
            // If promoteToType returned 'l' but field needs 'w', truncate
            if (qbeType == "w" && fieldTypeLegacy == VariableType::INT) {
                std::string truncTemp = allocTemp("w");
                emit("    " + truncTemp + " =w copy " + convertedTemp + "\n");
                m_stats.instructionsGenerated++;
                valueTemp = truncTemp;
            } else {
                valueTemp = convertedTemp;
            }
        }
        
        // 6. Store value using correct QBE type
        if (qbeType == "w") {
            emit("    storew " + valueTemp + ", " + currentPtr + "\n");
        } else if (qbeType == "d") {
            emit("    stored " + valueTemp + ", " + currentPtr + "\n");
        } else if (qbeType == "l") {
            emit("    storel " + valueTemp + ", " + currentPtr + "\n");
        } else if (qbeType == "s") {
            emit("    stores " + valueTemp + ", " + currentPtr + "\n");
        } else {
            // Default to word
            emit("    storew " + valueTemp + ", " + currentPtr + "\n");
        }
        
        m_stats.instructionsGenerated++;
    } else if (stmt->indices.empty()) {
        // Simple variable assignment
        
        // Check if this is a GLOBAL variable - handle specially with store operations
        bool isGlobalVar = false;
        int globalSlot = -1;
        VariableType varType = getVariableType(stmt->variable);
        
        if (m_symbols) {
            auto it = m_symbols->variables.find(stmt->variable);
            if (it != m_symbols->variables.end() && it->second.isGlobal) {
                isGlobalVar = true;
                globalSlot = it->second.globalOffset;
                varType = it->second.type;
            }
        }
        
        if (isGlobalVar) {
            // GLOBAL variable assignment using pointer arithmetic + store
            std::string qbeType = getQBEType(varType);
            VariableType exprType = inferExpressionType(stmt->value.get());
            
            // Type conversion if needed
            std::string finalValue = valueTemp;
            if (varType != exprType) {
                std::string actualQBEType = getActualQBEType(stmt->value.get());
                finalValue = promoteToType(valueTemp, exprType, varType, actualQBEType);
            }
            
            // Calculate address using global vector label directly (pre-calculate byte offset: slot * 8)
            int byteOffset = globalSlot * 8;
            std::string addr = allocTemp("l");
            emit("    " + addr + " =l add $__global_vector, " + std::to_string(byteOffset) + "\n");
            
            // Store value
            if (varType == VariableType::DOUBLE) {
                emit("    stored " + finalValue + ", " + addr + "\n");
            } else {
                emit("    storel " + finalValue + ", " + addr + "\n");
            }
            
            m_stats.instructionsGenerated += 2;
        } else {
            // Local or regular variable assignment
            std::string varRef = getVariableRef(stmt->variable);
            std::string qbeType = getQBEType(varType);
            
            // Infer the type of the expression value
            VariableType exprType = inferExpressionType(stmt->value.get());
            
            // For STRING types, call string_retain to increment refcount
            if (varType == VariableType::STRING && exprType == VariableType::STRING) {
                std::string retainedTemp = allocTemp("l");
                emit("    " + retainedTemp + " =l call $string_retain(l " + valueTemp + ")\n");
                emit("    " + varRef + " =l copy " + retainedTemp + "\n");
                m_stats.instructionsGenerated += 2;
            } else if (varType != exprType) {
                // Convert value to match variable type if needed
                // promoteToType emits the conversion and returns the result temp
                // Pass actual QBE type so it knows if INT is 'w' or 'l'
                std::string actualQBEType = getActualQBEType(stmt->value.get());
                std::string convertedValue = promoteToType(valueTemp, exprType, varType, actualQBEType);
                emit("    " + varRef + " =" + qbeType + " copy " + convertedValue + "\n");
                m_stats.instructionsGenerated++;
            } else {
                // Semantic types match, but check if QBE types differ (e.g., w vs l for INT)
                std::string exprQBEType = getActualQBEType(stmt->value.get());
                
                if (exprQBEType != qbeType) {
                    // Need to convert between QBE types
                    std::string convertedValue = valueTemp;
                    
                    if (exprQBEType == "w" && qbeType == "l") {
                        // Word to long (sign extend)
                        std::string extendedTemp = allocTemp("l");
                        emit("    " + extendedTemp + " =l extsw " + valueTemp + "\n");
                        convertedValue = extendedTemp;
                        m_stats.instructionsGenerated++;
                    } else if (exprQBEType == "l" && qbeType == "w") {
                        // Long to word (truncate)
                        std::string truncTemp = allocTemp("w");
                        emit("    " + truncTemp + " =w copy " + valueTemp + "\n");
                        convertedValue = truncTemp;
                        m_stats.instructionsGenerated++;
                    }
                    
                    emit("    " + varRef + " =" + qbeType + " copy " + convertedValue + "\n");
                } else {
                    // QBE types match, just copy
                    emit("    " + varRef + " =" + qbeType + " copy " + valueTemp + "\n");
                }
                m_stats.instructionsGenerated++;
            }
        }
    } else {
        // Array element assignment using descriptor-based approach
        emitComment("Array assignment: " + stmt->variable + "(...) = value");
        
        // Get element pointer (this includes bounds checking)
        std::string elementPtr = emitArrayElementPtr(stmt->variable, stmt->indices);
        
        // Determine array element type using TypeDescriptor
        TypeDescriptor elementTypeDesc = TypeDescriptor(BaseType::INTEGER); // Default
        if (m_symbols && m_symbols->arrays.find(stmt->variable) != m_symbols->arrays.end()) {
            const auto& arraySym = m_symbols->arrays.at(stmt->variable);
            // Use TypeDescriptor if available, otherwise convert from legacy type
            if (arraySym.elementTypeDesc.baseType != BaseType::UNKNOWN) {
                elementTypeDesc = arraySym.elementTypeDesc;
            } else {
                elementTypeDesc = legacyTypeToDescriptor(arraySym.type);
            }
        }
        
        // Convert value type to match array element type
        // Work with QBE types directly for better control over conversions
        std::string valueQBE = getQBEType(inferExpressionType(stmt->value.get()));
        std::string elemQBE = getQBETypeD(elementTypeDesc);
        
        // Convert if types don't match
        if (valueQBE != elemQBE) {
            if (valueQBE == "d" && elemQBE == "s") {
                // Double to single (float) conversion - truncate precision
                std::string convertedTemp = allocTemp("s");
                emit("    " + convertedTemp + " =s truncd " + valueTemp + "\n");
                m_stats.instructionsGenerated++;
                valueTemp = convertedTemp;
            } else if (valueQBE == "d" && elemQBE == "w") {
                // Double to integer word conversion
                std::string convertedTemp = allocTemp("w");
                emit("    " + convertedTemp + " =w dtosi " + valueTemp + "\n");
                m_stats.instructionsGenerated++;
                valueTemp = convertedTemp;
            } else if (valueQBE == "d" && elemQBE == "l") {
                // Double to long conversion
                std::string convertedTemp = allocTemp("l");
                emit("    " + convertedTemp + " =l dtosi " + valueTemp + "\n");
                m_stats.instructionsGenerated++;
                valueTemp = convertedTemp;
            } else if ((valueQBE == "w" || valueQBE == "l") && elemQBE == "d") {
                // Integer to double conversion
                std::string convertedTemp = allocTemp("d");
                if (valueQBE == "w") {
                    // Word to double: first extend to long, then convert
                    std::string longTemp = allocTemp("l");
                    emit("    " + longTemp + " =l extsw " + valueTemp + "\n");
                    emit("    " + convertedTemp + " =d sltof " + longTemp + "\n");
                    m_stats.instructionsGenerated += 2;
                } else {
                    // Long to double: direct conversion
                    emit("    " + convertedTemp + " =d sltof " + valueTemp + "\n");
                    m_stats.instructionsGenerated++;
                }
                valueTemp = convertedTemp;
            } else if ((valueQBE == "w" || valueQBE == "l") && elemQBE == "s") {
                // Integer to single (float) conversion
                std::string convertedTemp = allocTemp("s");
                if (valueQBE == "w") {
                    // Word to float: first extend to long, then convert
                    std::string longTemp = allocTemp("l");
                    emit("    " + longTemp + " =l extsw " + valueTemp + "\n");
                    emit("    " + convertedTemp + " =s sltof " + longTemp + "\n");
                    m_stats.instructionsGenerated += 2;
                } else {
                    // Long to float: direct conversion
                    emit("    " + convertedTemp + " =s sltof " + valueTemp + "\n");
                    m_stats.instructionsGenerated++;
                }
                valueTemp = convertedTemp;
            } else if (valueQBE == "s" && elemQBE == "d") {
                // Single to double conversion
                std::string convertedTemp = allocTemp("d");
                emit("    " + convertedTemp + " =d exts " + valueTemp + "\n");
                m_stats.instructionsGenerated++;
                valueTemp = convertedTemp;
            } else if (valueQBE == "w" && elemQBE == "l") {
                // Word to long extension
                std::string convertedTemp = allocTemp("l");
                emit("    " + convertedTemp + " =l extsw " + valueTemp + "\n");
                m_stats.instructionsGenerated++;
                valueTemp = convertedTemp;
            } else if (valueQBE == "l" && elemQBE == "w") {
                // Long to word truncation
                std::string convertedTemp = allocTemp("w");
                emit("    " + convertedTemp + " =w copy " + valueTemp + "\n");
                m_stats.instructionsGenerated++;
                valueTemp = convertedTemp;
            }
        }
        
        // Get the correct memory operation for the element type
        std::string memOp = getQBEMemOpD(elementTypeDesc);
        
        // Store value at element pointer with correct operation
        emit("    store" + memOp + " " + valueTemp + ", " + elementPtr + "\n");
        m_stats.instructionsGenerated++;
    }
}

// =============================================================================
// IF Statement (Conditional)
// =============================================================================

void QBECodeGenerator::emitIf(const IfStatement* stmt) {
    if (!stmt || !stmt->condition) return;
    
    // Evaluate condition
    std::string condTemp = emitExpression(stmt->condition.get());
    
    // Convert condition to boolean (0 or 1)
    std::string boolTemp = allocTemp("w");
    emit("    " + boolTemp + " =w cnew " + condTemp + ", 0\n");
    m_stats.instructionsGenerated++;
    
    // Check if this is an inline IF (single-line) or block-level IF (multi-line, CFG-driven)
    // Single-line IF: emit inline with labels
    // Multi-line IF: nested statements should be in separate CFG blocks
    bool isInlineIF = !stmt->isMultiLine && !stmt->thenStatements.empty();
    
    if (isInlineIF) {
        // Inline IF/THEN/ELSE - emit complete structure with local labels
        std::string thenLabel = makeLabel("then");
        std::string elseLabel = makeLabel("else");
        std::string endLabel = makeLabel("endif");
        
        emit("    jnz " + boolTemp + ", @" + thenLabel + ", @" + elseLabel + "\n");
        m_stats.instructionsGenerated++;
        
        // THEN block
        emit("@" + thenLabel + "\n");
        m_stats.labelsGenerated++;
        m_lastStatementWasTerminator = false;
        for (const auto& thenStmt : stmt->thenStatements) {
            emitStatement(thenStmt.get());
        }
        // Only emit jump if last statement wasn't a terminator (RETURN, EXIT, etc.)
        if (!m_lastStatementWasTerminator) {
            emit("    jmp @" + endLabel + "\n");
            m_stats.instructionsGenerated++;
        }
        
        // ELSE block
        emit("@" + elseLabel + "\n");
        m_stats.labelsGenerated++;
        m_lastStatementWasTerminator = false;
        for (const auto& elseStmt : stmt->elseStatements) {
            emitStatement(elseStmt.get());
        }
        // Only emit jump if last statement wasn't a terminator
        if (!m_lastStatementWasTerminator) {
            emit("    jmp @" + endLabel + "\n");
            m_stats.instructionsGenerated++;
        }
        
        // End of IF
        emit("@" + endLabel + "\n");
        m_stats.labelsGenerated++;
        m_lastStatementWasTerminator = false;
    } else {
        // Multi-line IF - store condition for CFG-driven branching
        // The nested statements should be in separate CFG blocks
        m_lastCondition = boolTemp;
        emitComment("Multi-line IF: condition stored for CFG branching");
    }
}

// =============================================================================
// FOR Statement (Loop Header)
// =============================================================================

void QBECodeGenerator::emitFor(const ForStatement* stmt) {
    if (!stmt || !stmt->start || !stmt->end) return;
    
    // FOR loop variable is a regular 64-bit INTEGER ('l')
    // Variable name has no suffix - it's just a plain name
    std::string varRef = getVariableRef(stmt->variable);

    // Find the user-visible variable (with type suffix) to mirror the counter into
    std::string userVarRef = varRef;
    if (m_symbols) {
        for (const auto& entry : m_symbols->variables) {
            if (stripTypeSuffix(entry.first) == stmt->variable) {
                userVarRef = "%var_" + sanitizeForQBE(entry.first);
                break;
            }
        }
    }
    
    // Set up loop context for EXIT FOR to work
    if (m_currentBlock && m_cfg) {
        int exitBlockId = -1;
        for (int i = m_currentBlock->id + 1; i < m_cfg->getBlockCount(); i++) {
            const BasicBlock* candidateBlock = m_cfg->getBlock(i);
            if (candidateBlock && candidateBlock->label.find("After FOR") != std::string::npos) {
                exitBlockId = i;
                break;
            }
        }
        if (exitBlockId >= 0) {
            std::string exitLabel = getBlockLabel(exitBlockId);
            pushLoop(exitLabel, "", "FOR", stmt->variable);
        }
    }
    
    // Initialize loop variable
    std::string startTemp = emitExpression(stmt->start.get());
    
    // Convert start value to 64-bit integer
    VariableType startType = inferExpressionType(stmt->start.get());
    if (startType == VariableType::DOUBLE || startType == VariableType::FLOAT) {
        startTemp = emitDoubleToInt(startTemp);
    }

    emit("    " + varRef + " =l copy " + startTemp + "\n");
    m_stats.instructionsGenerated++;

    // Keep user-declared variable in sync for BASIC visibility
    if (userVarRef != varRef) {
        emit("    " + userVarRef + " =l copy " + varRef + "\n");
        m_stats.instructionsGenerated++;
    }
    
    // Emit step value (default 1)
    std::string stepTemp;
    if (stmt->step) {
        stepTemp = emitExpression(stmt->step.get());
        
        // Convert step value to 64-bit integer
        VariableType stepType = inferExpressionType(stmt->step.get());
        if (stepType == VariableType::DOUBLE || stepType == VariableType::FLOAT) {
            stepTemp = emitDoubleToInt(stepTemp);
        }
    } else {
        stepTemp = allocTemp("l");
        emit("    " + stepTemp + " =l copy 1\n");
        m_stats.instructionsGenerated++;
    }
    
    // Store step for NEXT statement
    std::string stepVar = "%step_" + sanitizeForQBE(stmt->variable);
    emit("    " + stepVar + " =l copy " + stepTemp + "\n");
    m_stats.instructionsGenerated++;
    
    // Emit end value (evaluate once and store)
    std::string endTemp = emitExpression(stmt->end.get());
    
    // Convert end value to 64-bit integer
    VariableType endType = inferExpressionType(stmt->end.get());
    if (endType == VariableType::DOUBLE || endType == VariableType::FLOAT) {
        endTemp = emitDoubleToInt(endTemp);
    }
    
    std::string endVar = "%end_" + sanitizeForQBE(stmt->variable);
    emit("    " + endVar + " =l copy " + endTemp + "\n");
    m_stats.instructionsGenerated++;
    
    // Note: Loop condition check happens in the FOR check block (separate block)
    // This init block just sets up the loop variables and falls through to check block
}

// =============================================================================
// FOR EACH...IN Statement (Array Iteration Loop Header)
// =============================================================================

void QBECodeGenerator::emitForIn(const ForInStatement* stmt) {
    if (!stmt || !stmt->array) return;
    
    emitComment("FOR EACH...IN loop initialization");
    
    // Evaluate the array expression to get array descriptor pointer
    std::string arrayTemp = emitExpression(stmt->array.get());
    
    // Use the element type that was inferred during semantic analysis (cast from int)
    VariableType elemType = static_cast<VariableType>(stmt->inferredType);
    std::string qbeType = getQBEType(elemType);
    
    // Declare the loop variable with the inferred type (ADAPTIVE type resolved here)
    std::string loopVarName = stmt->variable;
    // Note: We do NOT declare the FOR EACH variable as a real variable
    // It's just an accessor for the current array element
    
    // Create loop index variable (hidden from user)
    std::string indexVar = "%foreach_idx_" + sanitizeForQBE(stmt->variable);
    emit("    " + indexVar + " =l copy 0\n");
    m_stats.instructionsGenerated++;
    
    // Get array size from descriptor (element_count at offset 8)
    std::string sizePtr = allocTemp("l");
    std::string arraySize = allocTemp("l");
    emit("    " + sizePtr + " =l add " + arrayTemp + ", 8\n");
    emit("    " + arraySize + " =l loadl " + sizePtr + "\n");
    m_stats.instructionsGenerated += 2;
    
    // Store array descriptor and size for NEXT to use
    std::string arrayVar = "%foreach_arr_" + sanitizeForQBE(stmt->variable);
    std::string sizeVar = "%foreach_size_" + sanitizeForQBE(stmt->variable);
    emit("    " + arrayVar + " =l copy " + arrayTemp + "\n");
    emit("    " + sizeVar + " =l copy " + arraySize + "\n");
    m_stats.instructionsGenerated += 2;
    
    // Set up loop context for EXIT FOR
    if (m_currentBlock && m_cfg) {
        int exitBlockId = -1;
        for (int i = m_currentBlock->id + 1; i < m_cfg->getBlockCount(); i++) {
            const BasicBlock* candidateBlock = m_cfg->getBlock(i);
            if (candidateBlock && candidateBlock->label.find("After FOR") != std::string::npos) {
                exitBlockId = i;
                break;
            }
        }
        if (exitBlockId >= 0) {
            std::string exitLabel = getBlockLabel(exitBlockId);
            auto* loop = pushLoop(exitLabel, "", "FOR", stmt->variable, true);
            // Store FOR EACH context
            if (loop) {
                loop->forEachArrayDesc = arrayVar;
                loop->forEachIndex = indexVar;
                loop->forEachElemType = elemType;
            }
        }
    }
    
    emitComment("FOR EACH...IN loop body starts - variable is virtual, accessed via array");
}

// =============================================================================
// NEXT Statement (Loop Footer)
// =============================================================================

void QBECodeGenerator::emitNext(const NextStatement* stmt) {
    if (!stmt) return;
    
    // Get variable name: use from statement, or from current loop context, or default to "I"
    std::string varName;
    if (!stmt->variable.empty()) {
        varName = stmt->variable;
    } else {
        // NEXT without variable - use the current FOR loop's variable
        auto* loop = getCurrentLoop();
        if (loop && loop->type == "FOR" && !loop->forVariable.empty()) {
            varName = loop->forVariable;
        } else {
            varName = "I";  // Fallback to default
        }
    }
    
    // For FOR EACH loops, find the variable with correct type suffix
    auto* loop = getCurrentLoop();
    bool isForEachLoop = (loop && loop->isForEach);
    
    if (isForEachLoop && m_symbols) {
        // Look for variable with matching base name
        for (const auto& entry : m_symbols->variables) {
            if (stripTypeSuffix(entry.first) == varName) {
                varName = entry.first;
                break;
            }
        }
    }

    
    if (isForEachLoop) {
        // Handle FOR EACH...IN loop iteration - just increment index
        std::string foreachIndexVar = "%foreach_idx_" + sanitizeForQBE(varName);
        
        emitComment("NEXT: increment FOR EACH index");
        
        // Increment index (the variable is accessed on-demand via getVariableRef)
        std::string indexTemp = allocTemp("l");
        emit("    " + indexTemp + " =l add " + foreachIndexVar + ", 1\n");
        emit("    " + foreachIndexVar + " =l copy " + indexTemp + "\n");
        m_stats.instructionsGenerated += 2;
        
        emitComment("NEXT: jump back to FOR EACH header");
    } else {
        // Handle traditional FOR loop
        std::string varRef = getVariableRef(varName);
        std::string stepVar = "%step_" + sanitizeForQBE(varName);

        // Find the user-visible variable (with type suffix) to mirror the counter into
        std::string userVarRef = varRef;
        if (m_symbols) {
            for (const auto& entry : m_symbols->variables) {
                if (stripTypeSuffix(entry.first) == varName) {
                    userVarRef = "%var_" + sanitizeForQBE(entry.first);
                    break;
                }
            }
        }
        
        // Increment loop variable: var = var + step (64-bit integer arithmetic)
        std::string newValueTemp = allocTemp("l");
        emit("    " + newValueTemp + " =l add " + varRef + ", " + stepVar + "\n");
        emit("    " + varRef + " =l copy " + newValueTemp + "\n");
        m_stats.instructionsGenerated += 2;

        // Keep user-declared variable in sync for BASIC visibility
        if (userVarRef != varRef) {
            emit("    " + userVarRef + " =l copy " + varRef + "\n");
            m_stats.instructionsGenerated++;
        }
        
        emitComment("NEXT: increment and jump back to FOR header");
    }
    
    // Pop loop context when we're done with the loop body
    popLoop();
}

// =============================================================================
// WHILE Statement (Loop Header)
// =============================================================================

void QBECodeGenerator::emitWhile(const WhileStatement* stmt) {
    if (!stmt || !stmt->condition) return;
    
    // Evaluate condition
    std::string condTemp = emitExpression(stmt->condition.get());
    
    // Convert to boolean
    std::string boolTemp = allocTemp("w");
    emit("    " + boolTemp + " =w cnew " + condTemp + ", 0\n");
    m_stats.instructionsGenerated++;
    
    // Store condition for CFG-driven branching
    m_lastCondition = boolTemp;
    
    // CFG handles branching to body or exit
    emitComment("WHILE condition - CFG handles branch to body/exit");
}

// =============================================================================
// WEND Statement (Loop Footer)
// =============================================================================

void QBECodeGenerator::emitWend(const WendStatement* stmt) {
    // WEND just marks end of WHILE loop
    // CFG handles jump back to WHILE header
    emitComment("WEND - CFG handles jump to WHILE header");
}

// =============================================================================
// GOTO Statement
// =============================================================================

void QBECodeGenerator::emitGoto(const GotoStatement* stmt) {
    if (!stmt) return;
    
    // Get target block from CFG
    int targetBlock = -1;
    
    if (stmt->isLabel) {
        // Symbolic label - need to find the line it's on, then map to block
        if (m_symbols) {
            auto it = m_symbols->labels.find(stmt->label);
            if (it != m_symbols->labels.end()) {
                // Find which line this label is on and map to block
                int labelLine = it->second.programLineIndex;
                if (m_cfg && labelLine >= 0) {
                    targetBlock = m_cfg->getBlockForLine(labelLine);
                }
            }
        }
    } else {
        // Line number
        targetBlock = m_cfg->getBlockForLine(stmt->lineNumber);
    }
    
    if (targetBlock >= 0) {
        std::string targetLabel = getBlockLabel(targetBlock);
        emit("    jmp @" + targetLabel + "\n");
        m_stats.instructionsGenerated++;
        m_lastStatementWasTerminator = true;  // GOTO is a terminator
    } else {
        emitComment("ERROR: GOTO target not found");
    }
}

// =============================================================================
// GOSUB Statement
// =============================================================================

void QBECodeGenerator::emitGosub(const GosubStatement* stmt) {
    if (!stmt) return;
    
    // Get target block
    int targetBlock = -1;
    if (stmt->isLabel) {
        if (m_symbols) {
            auto it = m_symbols->labels.find(stmt->label);
            if (it != m_symbols->labels.end()) {
                int labelLine = it->second.programLineIndex;
                if (m_cfg && labelLine >= 0) {
                    targetBlock = m_cfg->getBlockForLine(labelLine);
                }
            }
        }
    } else {
        targetBlock = m_cfg->getBlockForLine(stmt->lineNumber);
    }
    
    if (targetBlock >= 0) {
        // Get return block (next statement after GOSUB)
        int returnBlock = getFallthroughBlock(stmt);
        
        if (returnBlock >= 0) {
            // Push return block to stack
            std::string spTemp = allocTemp("w");
            emit("    " + spTemp + " =w loadw $return_sp\n");
            std::string spLong = allocTemp("l");
            emit("    " + spLong + " =l extsw " + spTemp + "\n");
            std::string byteOffset = allocTemp("l");
            emit("    " + byteOffset + " =l mul " + spLong + ", 4\n");
            std::string stackAddr = allocTemp("l");
            emit("    " + stackAddr + " =l add $return_stack, " + byteOffset + "\n");
            emit("    storew " + std::to_string(returnBlock) + ", " + stackAddr + "\n");
            std::string newSp = allocTemp("w");
            emit("    " + newSp + " =w add " + spTemp + ", 1\n");
            emit("    storew " + newSp + ", $return_sp\n");
            m_stats.instructionsGenerated += 7;
        }
        
        // Jump to target
        std::string targetLabel = getBlockLabel(targetBlock);
        emitComment("GOSUB to " + targetLabel);
        emit("    jmp @" + targetLabel + "\n");
        m_stats.instructionsGenerated++;
        m_lastStatementWasTerminator = true;
    } else {
        emitComment("ERROR: GOSUB target not found");
    }
}

// =============================================================================
// ON GOTO Statement
// =============================================================================

void QBECodeGenerator::emitOnGoto(const OnGotoStatement* stmt) {
    if (!stmt || stmt->isLabelList.empty()) return;
    
    // Evaluate the selector expression
    std::string selectorTemp = emitExpression(stmt->selector.get());
    
    // Convert to integer if needed (should already be int, but be safe)
    VariableType selectorType = inferExpressionType(stmt->selector.get());
    if (selectorType != VariableType::INT) {
        // Convert to int
        std::string intTemp = allocTemp("w");
        if (selectorType == VariableType::DOUBLE) {
            emit("    " + intTemp + " =w dtosi " + selectorTemp + "\n");
        } else {
            // Default conversion
            emit("    " + intTemp + " =w copy " + selectorTemp + "\n");
        }
        selectorTemp = intTemp;
        m_stats.instructionsGenerated++;
    }
    
    // Determine fallthrough block (next statement after ON GOTO)
    int fallthroughBlock = getFallthroughBlock(stmt);

    // Generate dispatch thunk with conditional jumps
    // ON x GOTO target1, target2, target3, ... means:
    // if x == 1 goto target1
    // if x == 2 goto target2
    // etc.
    // If out of range, fall through
    
    std::vector<std::pair<int, int>> dispatchTargets;
    dispatchTargets.reserve(stmt->isLabelList.size());

    for (size_t i = 0; i < stmt->isLabelList.size(); ++i) {
        int targetIndex = static_cast<int>(i) + 1;  // 1-based indexing

        int targetBlock = -1;
        if (stmt->isLabelList[i]) {
            if (m_symbols) {
                auto it = m_symbols->labels.find(stmt->labels[i]);
                if (it != m_symbols->labels.end()) {
                    int labelLine = it->second.programLineIndex;
                    if (m_cfg && labelLine >= 0) {
                        targetBlock = m_cfg->getBlockForLine(labelLine);
                    }
                }
            }
        } else {
            targetBlock = m_cfg->getBlockForLine(stmt->lineNumbers[i]);
        }

        if (targetBlock >= 0) {
            dispatchTargets.emplace_back(targetIndex, targetBlock);
        }
    }

    bool hasFallthroughBlock = fallthroughBlock >= 0;
    std::string fallthroughLabel = hasFallthroughBlock ? getBlockLabel(fallthroughBlock) : allocLabel();

    if (dispatchTargets.empty()) {
        if (hasFallthroughBlock) {
            emit("    jmp @" + fallthroughLabel + "\n");
            m_stats.instructionsGenerated++;
            m_lastStatementWasTerminator = true;
        } else {
            emitComment("WARNING: ON GOTO without valid targets or fallthrough block");
            emit("    jmp @" + getFunctionExitLabel() + "\n");
            m_stats.instructionsGenerated++;
            m_lastStatementWasTerminator = true;
        }
        return;
    }

    for (size_t i = 0; i < dispatchTargets.size(); ++i) {
        int targetIndex = dispatchTargets[i].first;
        int targetBlock = dispatchTargets[i].second;
        std::string targetLabel = getBlockLabel(targetBlock);

        std::string constTemp = allocTemp("w");
        emit("    " + constTemp + " =w copy " + std::to_string(targetIndex) + "\n");
        m_stats.instructionsGenerated++;

        std::string isEqualTemp = allocTemp("w");
        emit("    " + isEqualTemp + " =w ceqw " + selectorTemp + ", " + constTemp + "\n");
        m_stats.instructionsGenerated++;

        bool isLast = (i + 1 == dispatchTargets.size());
        std::string nextLabel = isLast ? fallthroughLabel : allocLabel();
        emit("    jnz " + isEqualTemp + ", @" + targetLabel + ", @" + nextLabel + "\n");
        m_stats.instructionsGenerated++;

        if (!isLast) {
            emitLabel(nextLabel);
        }
    }

    if (hasFallthroughBlock) {
        m_lastStatementWasTerminator = true;
    } else {
        emitLabel(fallthroughLabel);
        emitComment("WARNING: ON GOTO without valid fallthrough block");
        emit("    jmp @" + getFunctionExitLabel() + "\n");
        m_stats.instructionsGenerated++;
        m_lastStatementWasTerminator = true;
    }
}

// =============================================================================
// ON GOSUB Statement
// =============================================================================

void QBECodeGenerator::emitOnGosub(const OnGosubStatement* stmt) {
    if (!stmt || stmt->isLabelList.empty()) return;
    
    // Evaluate the selector expression
    std::string selectorTemp = emitExpression(stmt->selector.get());
    
    // Convert to integer if needed (should already be int, but be safe)
    VariableType selectorType = inferExpressionType(stmt->selector.get());
    if (selectorType != VariableType::INT) {
        // Convert to int
        std::string intTemp = allocTemp("w");
        if (selectorType == VariableType::DOUBLE) {
            emit("    " + intTemp + " =w dtosi " + selectorTemp + "\n");
        } else {
            // Default conversion
            emit("    " + intTemp + " =w copy " + selectorTemp + "\n");
        }
        selectorTemp = intTemp;
        m_stats.instructionsGenerated++;
    }
    
    // Get return block (next statement after ON GOSUB)
    int returnBlock = getFallthroughBlock(stmt);
    
    // Push return block to stack so RETURN can resume execution
    if (returnBlock >= 0) {
        std::string spTemp = allocTemp("w");
        emit("    " + spTemp + " =w loadw $return_sp\n");
        std::string spLong = allocTemp("l");
        emit("    " + spLong + " =l extsw " + spTemp + "\n");
        std::string byteOffset = allocTemp("l");
        emit("    " + byteOffset + " =l mul " + spLong + ", 4\n");
        std::string stackAddr = allocTemp("l");
        emit("    " + stackAddr + " =l add $return_stack, " + byteOffset + "\n");
        emit("    storew " + std::to_string(returnBlock) + ", " + stackAddr + "\n");
        std::string newSp = allocTemp("w");
        emit("    " + newSp + " =w add " + spTemp + ", 1\n");
        emit("    storew " + newSp + ", $return_sp\n");
        m_stats.instructionsGenerated += 7;
    } else {
        emitComment("WARNING: ON GOSUB without valid fallthrough block");
    }
    
    // Generate dispatch thunk with conditional jumps
    // ON x GOSUB target1, target2, target3, ... means:
    // if x == 1 GOSUB target1
    // if x == 2 GOSUB target2
    // etc.
    std::vector<std::pair<int, int>> dispatchTargets;
    dispatchTargets.reserve(stmt->isLabelList.size());

    for (size_t i = 0; i < stmt->isLabelList.size(); ++i) {
        int targetIndex = static_cast<int>(i) + 1;

        int targetBlock = -1;
        if (stmt->isLabelList[i]) {
            if (m_symbols) {
                auto it = m_symbols->labels.find(stmt->labels[i]);
                if (it != m_symbols->labels.end()) {
                    int labelLine = it->second.programLineIndex;
                    if (m_cfg && labelLine >= 0) {
                        targetBlock = m_cfg->getBlockForLine(labelLine);
                    }
                }
            }
        } else {
            targetBlock = m_cfg->getBlockForLine(stmt->lineNumbers[i]);
        }

        if (targetBlock >= 0) {
            dispatchTargets.emplace_back(targetIndex, targetBlock);
        }
    }

    std::string fallbackLabel = allocLabel();

    for (size_t i = 0; i < dispatchTargets.size(); ++i) {
        int targetIndex = dispatchTargets[i].first;
        int targetBlock = dispatchTargets[i].second;
        std::string targetLabel = getBlockLabel(targetBlock);

        std::string constTemp = allocTemp("w");
        emit("    " + constTemp + " =w copy " + std::to_string(targetIndex) + "\n");
        m_stats.instructionsGenerated++;

        std::string isEqualTemp = allocTemp("w");
        emit("    " + isEqualTemp + " =w ceqw " + selectorTemp + ", " + constTemp + "\n");
        m_stats.instructionsGenerated++;

        bool isLast = (i + 1 == dispatchTargets.size());
        std::string falseLabel = isLast ? fallbackLabel : allocLabel();
        emit("    jnz " + isEqualTemp + ", @" + targetLabel + ", @" + falseLabel + "\n");
        m_stats.instructionsGenerated++;

        if (!isLast) {
            emitLabel(falseLabel);
        }
    }

    emitLabel(fallbackLabel);

    if (returnBlock >= 0) {
        std::string spTemp = allocTemp("w");
        emit("    " + spTemp + " =w loadw $return_sp\n");
        std::string newSp = allocTemp("w");
        emit("    " + newSp + " =w sub " + spTemp + ", 1\n");
        emit("    storew " + newSp + ", $return_sp\n");
        std::string nextBlockLabel = getBlockLabel(returnBlock);
        emit("    jmp @" + nextBlockLabel + "\n");
        m_stats.instructionsGenerated += 4;
        m_lastStatementWasTerminator = true;
    } else {
        emitComment("ERROR: ON GOSUB without valid return block");
        emit("    jmp @" + getFunctionExitLabel() + "\n");
        m_stats.instructionsGenerated++;
        m_lastStatementWasTerminator = true;
    }
}

// =============================================================================
// RETURN Statement
// =============================================================================

void QBECodeGenerator::emitReturn(const ReturnStatement* stmt) {
    if (!stmt) return;
    
    // Check if we're in a function
    if (m_inFunction) {
        // RETURN expr - set function return value and exit
        if (stmt->returnValue) {
            // Evaluate the return expression
            std::string valueTemp = emitExpression(stmt->returnValue.get());
            
            // Get the function name for the return variable
            std::string returnVar = "%var_" + m_currentFunction;
            
            // Get the return type
            std::string qbeType = "w";
            if (m_cfg && m_cfg->returnType == VariableType::INT) {
                qbeType = "l";
            } else if (m_cfg && m_cfg->returnType == VariableType::DOUBLE) {
                qbeType = "d";
            } else if (m_cfg && m_cfg->returnType == VariableType::FLOAT) {
                qbeType = "d";
            } else if (m_cfg && m_cfg->returnType == VariableType::STRING) {
                qbeType = "l";
            }
            
            // Get the actual type of the expression
            std::string exprType = getActualQBEType(stmt->returnValue.get());
            
            // Convert if types don't match
            std::string finalValue = valueTemp;
            if (exprType != qbeType) {
                std::string convertedTemp = allocTemp(qbeType);
                
                // Convert between types
                if (qbeType == "l" && exprType == "d") {
                    // Double to long (integer)
                    emit("    " + convertedTemp + " =l dtosi " + valueTemp + "\n");
                } else if (qbeType == "l" && exprType == "w") {
                    // Word to long (sign extend)
                    emit("    " + convertedTemp + " =l extsw " + valueTemp + "\n");
                } else if (qbeType == "d" && exprType == "l") {
                    // Long to double
                    emit("    " + convertedTemp + " =d sltof " + valueTemp + "\n");
                } else if (qbeType == "d" && exprType == "w") {
                    // Word to double (sign extend to long first, then convert)
                    std::string extendedTemp = allocTemp("l");
                    emit("    " + extendedTemp + " =l extsw " + valueTemp + "\n");
                    emit("    " + convertedTemp + " =d sltof " + extendedTemp + "\n");
                } else if (qbeType == "w" && exprType == "l") {
                    // Long to word (truncate)
                    emit("    " + convertedTemp + " =w copy " + valueTemp + "\n");
                } else if (qbeType == "w" && exprType == "d") {
                    // Double to word (convert to long first, then truncate)
                    std::string longTemp = allocTemp("l");
                    emit("    " + longTemp + " =l dtosi " + valueTemp + "\n");
                    emit("    " + convertedTemp + " =w copy " + longTemp + "\n");
                } else {
                    // Same type or unsupported conversion, just copy
                    emit("    " + convertedTemp + " =" + qbeType + " copy " + valueTemp + "\n");
                }
                
                m_stats.instructionsGenerated++;
                finalValue = convertedTemp;
            }
            
            // Assign to return variable
            emit("    " + returnVar + " =" + qbeType + " copy " + finalValue + "\n");
            m_stats.instructionsGenerated++;
        }
        
        // Jump to function exit (tidy_exit for cleanup)
        emit("    jmp @" + getFunctionExitLabel() + "\n");
        m_stats.instructionsGenerated++;
        m_lastStatementWasTerminator = true;
    } else {
        // RETURN from GOSUB
        // Pop return block index from stack and dispatch
        std::string spTemp = allocTemp("w");
        emit("    " + spTemp + " =w loadw $return_sp\n");
        std::string newSp = allocTemp("w");
        emit("    " + newSp + " =w sub " + spTemp + ", 1\n");
        emit("    storew " + newSp + ", $return_sp\n");
        std::string newSpLong = allocTemp("l");
        emit("    " + newSpLong + " =l extsw " + newSp + "\n");
        std::string byteOffset = allocTemp("l");
        emit("    " + byteOffset + " =l mul " + newSpLong + ", 4\n");
        std::string stackAddr = allocTemp("l");
        emit("    " + stackAddr + " =l add $return_stack, " + byteOffset + "\n");
        std::string returnBlockIdTemp = allocTemp("w");
        emit("    " + returnBlockIdTemp + " =w loadw " + stackAddr + "\n");
        m_stats.instructionsGenerated += 7;

        // Optimization: Only check blocks that are GOSUB return points (sparse jump table)
        // This dramatically reduces the number of comparisons from O(total blocks) to O(return blocks)
        if (m_cfg && !m_cfg->gosubReturnBlocks.empty()) {
            emitComment("Sparse RETURN dispatch - only checking " + 
                       std::to_string(m_cfg->gosubReturnBlocks.size()) + 
                       " return blocks (out of " + 
                       std::to_string(m_cfg->blocks.size()) + " total)");
            
            std::string fallbackLabel = allocLabel();
            std::vector<int> returnBlocks(m_cfg->gosubReturnBlocks.begin(), 
                                         m_cfg->gosubReturnBlocks.end());
            std::sort(returnBlocks.begin(), returnBlocks.end());
            
            for (size_t i = 0; i < returnBlocks.size(); ++i) {
                int blockId = returnBlocks[i];
                std::string caseValue = allocTemp("w");
                emit("    " + caseValue + " =w copy " + std::to_string(blockId) + "\n");
                std::string isMatch = allocTemp("w");
                emit("    " + isMatch + " =w ceqw " + returnBlockIdTemp + ", " + caseValue + "\n");
                std::string targetLabel = getBlockLabel(blockId);
                bool isLast = (i + 1 == returnBlocks.size());
                std::string falseLabel = isLast ? fallbackLabel : allocLabel();
                emit("    jnz " + isMatch + ", @" + targetLabel + ", @" + falseLabel + "\n");
                m_stats.instructionsGenerated += 3;
                if (!isLast) {
                    emitLabel(falseLabel);
                }
            }
            emitLabel(fallbackLabel);
        } else {
            emitComment("WARNING: No GOSUB return blocks found, using fallback");
        }

        emitComment("RETURN stack underflow or invalid target - falling back to program exit");
        emit("    jmp @" + getFunctionExitLabel() + "\n");
        m_stats.instructionsGenerated++;
        m_lastStatementWasTerminator = true;
    }
}

// =============================================================================
// DIM Statement (Array Declaration)
// =============================================================================

void QBECodeGenerator::emitDim(const DimStatement* stmt) {
    if (!stmt) return;
    
    for (const auto& arrayDecl : stmt->arrays) {
        std::string arrayName = arrayDecl.name;
        
        // Skip scalar variables (DIM x AS INTEGER, DIM p AS Point with no dimensions)
        // These are variables, not arrays
        if (arrayDecl.dimensions.empty()) {
            emitComment("Scalar variable " + arrayName + " (no array allocation needed)");
            continue;
        }
        
        // Check if this is a UDT array that was already allocated on the heap (global scope only)
        // Local arrays in functions need to be allocated here
        bool isLocalArray = !m_functionStack.empty();
        auto elemTypeIt = m_arrayElementTypes.find(arrayName);
        if (!isLocalArray && elemTypeIt != m_arrayElementTypes.end()) {
            // Global UDT array - already allocated in emitMainFunction, skip runtime creation
            emitComment("Array " + arrayName + " already allocated on heap");
            continue;
        }
        
        // Check if this is a UDT array (needs malloc instead of runtime array_create)
        // Check the DimStatement itself for the type (works for both global and local arrays)
        bool isUDTArray = arrayDecl.hasAsType && !arrayDecl.asTypeName.empty();
        std::string udtTypeName = arrayDecl.asTypeName;
        
        std::string arrayRef = getArrayRef(arrayName);
        
        // Use array descriptor approach for all arrays
        emitComment("DIM " + arrayName + " with descriptor (dope vector)");
        
        // Evaluate dimension expressions
        std::vector<std::string> dimTemps;
        for (const auto& dimExpr : arrayDecl.dimensions) {
            dimTemps.push_back(emitExpression(dimExpr.get()));
        }
        
        // Support 1D and 2D arrays
        if (dimTemps.size() > 2) {
            emitComment("ERROR: Arrays with more than 2 dimensions not yet supported");
            continue;
        }
        
        bool is2D = (dimTemps.size() == 2);
        
        // Determine element size and type suffix using TypeDescriptor
        size_t elementSize;
        char typeSuffixChar = 0; // 0 for UDT/opaque
        TypeDescriptor elementTypeDesc;
        
        if (isUDTArray) {
            elementSize = calculateTypeSize(udtTypeName);
            m_arrayElementTypes[arrayName] = udtTypeName;
        } else {
            // Use TypeDescriptor to get correct element size
            // Prefer asTypeKeyword (preserves LONG, unsigned info) over typeSuffix
            if (arrayDecl.hasAsType && arrayDecl.asTypeKeyword != TokenType::UNKNOWN) {
                elementTypeDesc = keywordToDescriptor(arrayDecl.asTypeKeyword);
            } else {
                elementTypeDesc = tokenSuffixToDescriptor(arrayDecl.typeSuffix);
            }
            
            // Calculate element size based on BaseType
            switch (elementTypeDesc.baseType) {
                case BaseType::BYTE:
                case BaseType::UBYTE:
                    elementSize = 1;
                    break;
                case BaseType::SHORT:
                case BaseType::USHORT:
                    elementSize = 2;
                    break;
                case BaseType::INTEGER:
                case BaseType::UINTEGER:
                case BaseType::SINGLE:
                    elementSize = 4;
                    break;
                case BaseType::LONG:
                case BaseType::ULONG:
                case BaseType::DOUBLE:
                case BaseType::STRING:
                case BaseType::UNICODE:
                case BaseType::POINTER:
                    elementSize = 8;
                    break;
                default:
                    elementSize = 8; // Default
                    break;
            }
            
            // Set type suffix character for descriptor
            switch (arrayDecl.typeSuffix) {
                case TokenType::TYPE_INT:    typeSuffixChar = '%'; break;
                case TokenType::TYPE_FLOAT:  typeSuffixChar = '!'; break;
                case TokenType::TYPE_DOUBLE: typeSuffixChar = '#'; break;
                case TokenType::TYPE_STRING: typeSuffixChar = '$'; break;
                case TokenType::TYPE_BYTE:   typeSuffixChar = '@'; break;
                case TokenType::TYPE_SHORT:  typeSuffixChar = '^'; break;
                default: typeSuffixChar = 0; break;
            }
        }
        
        std::string dimTemp1 = dimTemps[0];
        
        // Convert dimension 1 to INT if needed
        VariableType dimType1 = inferExpressionType(arrayDecl.dimensions[0].get());
        if (dimType1 != VariableType::INT) {
            dimTemp1 = promoteToType(dimTemp1, dimType1, VariableType::INT);
        }
        
        std::string dimTemp2;
        if (is2D) {
            dimTemp2 = dimTemps[1];
            // Convert dimension 2 to INT if needed
            VariableType dimType2 = inferExpressionType(arrayDecl.dimensions[1].get());
            if (dimType2 != VariableType::INT) {
                dimTemp2 = promoteToType(dimTemp2, dimType2, VariableType::INT);
            }
        }
        
        // Get array descriptor address (assuming descriptor is stored with array reference)
        // For now, arrayRef points to the descriptor structure
        std::string descPtr = arrayRef;
        
        // Calculate bounds (BASIC: DIM A(N) creates 0 to N, or 1 to N with OPTION BASE 1)
        // We'll use 0-based for simplicity (lowerBound=0, upperBound=N)
        int64_t lowerBound = 0; // TODO: respect OPTION BASE
        
        // upperBound1 = dimTemp1 (N)
        std::string upperBoundTemp1 = dimTemp1;
        
        // count1 = upperBound1 - lowerBound + 1 = N + 1
        std::string countTemp1 = allocTemp("w");
        emit("    " + countTemp1 + " =w add " + upperBoundTemp1 + ", 1\n");
        m_stats.instructionsGenerated++;
        
        // Convert to long for size calculations
        std::string countLong1 = allocTemp("l");
        emit("    " + countLong1 + " =l extsw " + countTemp1 + "\n");
        m_stats.instructionsGenerated++;
        
        std::string totalCount;
        std::string upperBoundTemp2;
        std::string countLong2;
        
        if (is2D) {
            // upperBound2 = dimTemp2 (M)
            upperBoundTemp2 = dimTemp2;
            
            // count2 = upperBound2 - lowerBound + 1 = M + 1
            std::string countTemp2 = allocTemp("w");
            emit("    " + countTemp2 + " =w add " + upperBoundTemp2 + ", 1\n");
            m_stats.instructionsGenerated++;
            
            // Convert to long for size calculations
            countLong2 = allocTemp("l");
            emit("    " + countLong2 + " =l extsw " + countTemp2 + "\n");
            m_stats.instructionsGenerated++;
            
            // totalCount = count1 * count2
            totalCount = allocTemp("l");
            emit("    " + totalCount + " =l mul " + countLong1 + ", " + countLong2 + "\n");
            m_stats.instructionsGenerated++;
        } else {
            totalCount = countLong1;
        }
        
        // totalBytes = totalCount * elementSize
        std::string totalBytes = allocTemp("l");
        emit("    " + totalBytes + " =l mul " + totalCount + ", " + std::to_string(elementSize) + "\n");
        m_stats.instructionsGenerated++;
        
        // Allocate array data
        std::string dataPtr = allocTemp("l");
        emit("    " + dataPtr + " =l call $malloc(l " + totalBytes + ")\n");
        m_stats.instructionsGenerated++;
        
        // Zero-initialize the array
        emit("    call $memset(l " + dataPtr + ", w 0, l " + totalBytes + ")\n");
        m_stats.instructionsGenerated++;
        
        // Initialize descriptor fields
        // ArrayDescriptor layout (updated for 2D support - 64 bytes total):
        //   offset 0:  data pointer (8 bytes)
        //   offset 8:  lowerBound1 (8 bytes)
        //   offset 16: upperBound1 (8 bytes)
        //   offset 24: lowerBound2 (8 bytes) - 0 for 1D arrays
        //   offset 32: upperBound2 (8 bytes) - 0 for 1D arrays
        //   offset 40: elementSize (8 bytes)
        //   offset 48: dimensions (4 bytes)
        //   offset 52: base (4 bytes)
        //   offset 56: typeSuffix (1 byte)
        
        // Store data pointer at offset 0
        emit("    storel " + dataPtr + ", " + descPtr + "\n");
        m_stats.instructionsGenerated++;
        
        // Store lowerBound1 at offset 8
        std::string lowerBound1Addr = allocTemp("l");
        emit("    " + lowerBound1Addr + " =l add " + descPtr + ", 8\n");
        emit("    storel " + std::to_string(lowerBound) + ", " + lowerBound1Addr + "\n");
        m_stats.instructionsGenerated += 2;
        
        // Store upperBound1 at offset 16
        std::string upperBound1Addr = allocTemp("l");
        emit("    " + upperBound1Addr + " =l add " + descPtr + ", 16\n");
        std::string upperBoundLong1 = allocTemp("l");
        emit("    " + upperBoundLong1 + " =l extsw " + upperBoundTemp1 + "\n");
        emit("    storel " + upperBoundLong1 + ", " + upperBound1Addr + "\n");
        m_stats.instructionsGenerated += 3;
        
        if (is2D) {
            // Store lowerBound2 at offset 24
            std::string lowerBound2Addr = allocTemp("l");
            emit("    " + lowerBound2Addr + " =l add " + descPtr + ", 24\n");
            emit("    storel " + std::to_string(lowerBound) + ", " + lowerBound2Addr + "\n");
            m_stats.instructionsGenerated += 2;
            
            // Store upperBound2 at offset 32
            std::string upperBound2Addr = allocTemp("l");
            emit("    " + upperBound2Addr + " =l add " + descPtr + ", 32\n");
            std::string upperBoundLong2 = allocTemp("l");
            emit("    " + upperBoundLong2 + " =l extsw " + upperBoundTemp2 + "\n");
            emit("    storel " + upperBoundLong2 + ", " + upperBound2Addr + "\n");
            m_stats.instructionsGenerated += 3;
        } else {
            // Store 0 for lowerBound2 at offset 24
            std::string lowerBound2Addr = allocTemp("l");
            emit("    " + lowerBound2Addr + " =l add " + descPtr + ", 24\n");
            emit("    storel 0, " + lowerBound2Addr + "\n");
            m_stats.instructionsGenerated += 2;
            
            // Store 0 for upperBound2 at offset 32
            std::string upperBound2Addr = allocTemp("l");
            emit("    " + upperBound2Addr + " =l add " + descPtr + ", 32\n");
            emit("    storel 0, " + upperBound2Addr + "\n");
            m_stats.instructionsGenerated += 2;
        }
        
        // Store elementSize at offset 40
        std::string elemSizeAddr = allocTemp("l");
        emit("    " + elemSizeAddr + " =l add " + descPtr + ", 40\n");
        emit("    storel " + std::to_string(elementSize) + ", " + elemSizeAddr + "\n");
        m_stats.instructionsGenerated += 2;
        
        // Store dimensions at offset 48
        std::string dimCountAddr = allocTemp("l");
        emit("    " + dimCountAddr + " =l add " + descPtr + ", 48\n");
        emit("    storew " + std::to_string(is2D ? 2 : 1) + ", " + dimCountAddr + "\n");
        m_stats.instructionsGenerated += 2;

        // Store base at offset 52 (currently always 0 until OPTION BASE is threaded through)
        std::string baseAddr = allocTemp("l");
        emit("    " + baseAddr + " =l add " + descPtr + ", 52\n");
        emit("    storew 0, " + baseAddr + "\n");
        m_stats.instructionsGenerated += 2;

        // Store type suffix at offset 56
        std::string typeAddr = allocTemp("l");
        emit("    " + typeAddr + " =l add " + descPtr + ", 56\n");
        emit("    storeb " + std::to_string(static_cast<int>(typeSuffixChar)) + ", " + typeAddr + "\n");
        m_stats.instructionsGenerated += 2;
        
        if (is2D) {
            emitComment("2D Array " + arrayName + " descriptor initialized (element size: " + std::to_string(elementSize) + " bytes)");
        } else {
            emitComment("1D Array " + arrayName + " descriptor initialized (element size: " + std::to_string(elementSize) + " bytes)");
        }
        
        // If in function, track for cleanup
        if (isLocalArray) {
            m_functionStack.top().localArrays.push_back(arrayName);
            emitComment("Local array " + arrayName + " (will be freed on function exit)");
        }
    }
}

// =============================================================================
// END Statement
// =============================================================================

void QBECodeGenerator::emitEnd(const EndStatement* stmt) {
    emit("    jmp @" + getFunctionExitLabel() + "\n");
    m_stats.instructionsGenerated++;
    m_lastStatementWasTerminator = true;  // END is a terminator
}

// =============================================================================
// REM Statement (Comment)
// =============================================================================

void QBECodeGenerator::emitRem(const RemStatement* stmt) {
    if (stmt && m_config.emitComments) {
        emitComment("REM: " + stmt->comment);
    }
}

// =============================================================================
// CALL Statement
// =============================================================================

void QBECodeGenerator::emitCall(const CallStatement* stmt) {
    if (!stmt) return;
    
    emitComment("CALL " + stmt->subName);
    
    // Evaluate arguments (get raw temporaries)
    std::vector<std::string> argTemps;
    std::vector<VariableType> argTypes;
    for (const auto& arg : stmt->arguments) {
        argTemps.push_back(emitExpression(arg.get()));
        argTypes.push_back(inferExpressionType(arg.get()));
    }
    
    std::string subName = stmt->subName;
    
    // Check if this is a user-defined SUB/FUNCTION
    bool isUserDefined = false;
    const ControlFlowGraph* subCFG = nullptr;
    if (m_programCFG) {
        subCFG = m_programCFG->getFunctionCFG(subName);
        isUserDefined = (subCFG != nullptr);
    }
    
    // Pre-convert arguments to match parameter types BEFORE emitting the call
    std::vector<std::string> convertedArgTemps;
    std::vector<std::string> convertedArgTypes;
    
    if (isUserDefined) {
        for (size_t i = 0; i < argTemps.size(); ++i) {
            std::string argTemp = argTemps[i];
            VariableType argType = argTypes[i];
            VariableType paramType = VariableType::DOUBLE;  // Default
            
            // Look up actual parameter type if available
            if (subCFG && i < subCFG->parameterTypes.size()) {
                paramType = subCFG->parameterTypes[i];
            } else if (m_symbols) {
                // Try to find in symbol table
                auto it = m_symbols->functions.find(subName);
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
    
    if (isUserDefined) {
        // Call user-defined SUB with pre-converted arguments
        // SUBs return a dummy value (0) which we discard
        std::string resultTemp = allocTemp("w");
        emit("    " + resultTemp + " =w call $" + subName + "(");
        
        for (size_t i = 0; i < convertedArgTemps.size(); ++i) {
            if (i > 0) emit(", ");
            emit(convertedArgTypes[i] + " " + convertedArgTemps[i]);
        }
        
        emit(")\n");
    } else {
        // Call runtime library function (edge case for CALL)
        std::string runtimeFunc = mapToRuntimeFunction(subName);

        // If this CALL returns an ArrayDescriptor* (currently SPLIT$), capture and destroy
        std::string upperName = subName;
        for (char& c : upperName) c = std::toupper(c);
        bool returnsArray = (upperName == "SPLIT$");

        if (returnsArray) {
            std::string resultTemp = allocTemp("l");
            emit("    " + resultTemp + " =l call $" + runtimeFunc + "(");
            for (size_t i = 0; i < convertedArgTemps.size(); ++i) {
                if (i > 0) emit(", ");
                emit(convertedArgTypes[i] + " " + convertedArgTemps[i]);
            }
            emit(")\n");
            emit("    call $array_descriptor_destroy(l " + resultTemp + ")\n");
            m_stats.instructionsGenerated += 2;
        } else {
            std::string resultTemp = allocTemp("w");
            emit("    " + resultTemp + " =w call $" + runtimeFunc + "(");
            for (size_t i = 0; i < convertedArgTemps.size(); ++i) {
                if (i > 0) emit(", ");
                emit(convertedArgTypes[i] + " " + convertedArgTemps[i]);
            }
            emit(")\n");
            m_stats.instructionsGenerated++;
        }
    }
    
    m_stats.instructionsGenerated++;
}

// =============================================================================
// ERASE Statement
// =============================================================================

void QBECodeGenerator::emitErase(const EraseStatement* stmt) {
    if (!stmt) return;
    
    emitComment("ERASE arrays (free data and reset descriptor)");
    
    for (const auto& arrayName : stmt->arrayNames) {
        std::string descPtr = getArrayRef(arrayName);
        
        emitComment("ERASE array " + arrayName);
        
        // Delegate erase to runtime helper (releases string elements when needed)
        emit("    call $array_descriptor_erase(l " + descPtr + ")\n");
        m_stats.instructionsGenerated++;
        emitComment("Array " + arrayName + " erased");
    }
}

// =============================================================================
// REDIM Statement
// =============================================================================

void QBECodeGenerator::emitRedim(const RedimStatement* stmt) {
    if (!stmt) return;
    
    emitComment(stmt->preserve ? "REDIM PRESERVE (with realloc)" : "REDIM (free and malloc)");
    
    for (const auto& arrayRedim : stmt->arrays) {
        std::string arrayName = arrayRedim.name;
        std::string descPtr = getArrayRef(arrayName);
        
        // Evaluate dimension expressions
        std::vector<std::string> dimTemps;
        for (const auto& dimExpr : arrayRedim.dimensions) {
            dimTemps.push_back(emitExpression(dimExpr.get()));
        }
        
        // For now, only support 1D arrays
        if (dimTemps.size() != 1) {
            emitComment("ERROR: Multi-dimensional arrays not yet supported in REDIM");
            continue;
        }
        
        std::string dimTemp = dimTemps[0];
        
        // Convert dimension to INT if needed
        VariableType dimType = inferExpressionType(arrayRedim.dimensions[0].get());
        if (dimType != VariableType::INT) {
            dimTemp = promoteToType(dimTemp, dimType, VariableType::INT);
        }
        
        // Load element size from descriptor (offset 40)
        std::string elemSizeAddr = allocTemp("l");
        emit("    " + elemSizeAddr + " =l add " + descPtr + ", 40\n");
        std::string elemSize = allocTemp("l");
        emit("    " + elemSize + " =l loadl " + elemSizeAddr + "\n");
        m_stats.instructionsGenerated += 2;
        
        // Calculate new count = dimTemp + 1
        std::string newCount = allocTemp("w");
        emit("    " + newCount + " =w add " + dimTemp + ", 1\n");
        m_stats.instructionsGenerated++;
        
        std::string newCountLong = allocTemp("l");
        emit("    " + newCountLong + " =l extsw " + newCount + "\n");
        m_stats.instructionsGenerated++;
        
        // Calculate new size in bytes
        std::string newSize = allocTemp("l");
        emit("    " + newSize + " =l mul " + newCountLong + ", " + elemSize + "\n");
        m_stats.instructionsGenerated++;
        
        if (stmt->preserve) {
            // REDIM PRESERVE: use realloc to resize and keep data
            emitComment("REDIM PRESERVE " + arrayName + " (using realloc)");
            
            // Load old data pointer from descriptor (offset 0)
            std::string oldPtr = allocTemp("l");
            emit("    " + oldPtr + " =l loadl " + descPtr + "\n");
            m_stats.instructionsGenerated++;
            
            // Load old upper bound (offset 16) to calculate old size
            std::string oldUpperAddr = allocTemp("l");
            emit("    " + oldUpperAddr + " =l add " + descPtr + ", 16\n");
            std::string oldUpper = allocTemp("l");
            emit("    " + oldUpper + " =l loadl " + oldUpperAddr + "\n");
            m_stats.instructionsGenerated += 2;
            
            // oldCount = oldUpper + 1 (assuming lowerBound=0)
            std::string oldCount = allocTemp("l");
            emit("    " + oldCount + " =l add " + oldUpper + ", 1\n");
            m_stats.instructionsGenerated++;
            
            std::string oldSize = allocTemp("l");
            emit("    " + oldSize + " =l mul " + oldCount + ", " + elemSize + "\n");
            m_stats.instructionsGenerated++;
            
            // Call realloc
            std::string newPtr = allocTemp("l");
            emit("    " + newPtr + " =l call $realloc(l " + oldPtr + ", l " + newSize + ")\n");
            m_stats.instructionsGenerated++;
            
            // If growing, zero-fill new elements
            std::string isGrowing = allocTemp("w");
            emit("    " + isGrowing + " =w cultl " + oldSize + ", " + newSize + "\n");
            m_stats.instructionsGenerated++;
            
            std::string zeroFillLabel = allocLabel();
            std::string updateDescLabel = allocLabel();
            
            emit("    jnz " + isGrowing + ", @" + zeroFillLabel + ", @" + updateDescLabel + "\n");
            m_stats.instructionsGenerated++;
            
            // Zero-fill block
            emit("@" + zeroFillLabel + "\n");
            std::string fillStart = allocTemp("l");
            emit("    " + fillStart + " =l add " + newPtr + ", " + oldSize + "\n");
            std::string fillSize = allocTemp("l");
            emit("    " + fillSize + " =l sub " + newSize + ", " + oldSize + "\n");
            emit("    call $memset(l " + fillStart + ", w 0, l " + fillSize + ")\n");
            m_stats.instructionsGenerated += 3;
            
            // Update descriptor
            emit("@" + updateDescLabel + "\n");
            emit("    storel " + newPtr + ", " + descPtr + "\n");
            m_stats.instructionsGenerated++;
            
            // Update upperBound in descriptor (offset 16)
            std::string upperBoundAddr = allocTemp("l");
            emit("    " + upperBoundAddr + " =l add " + descPtr + ", 16\n");
            std::string newUpperLong = allocTemp("l");
            emit("    " + newUpperLong + " =l extsw " + dimTemp + "\n");
            emit("    storel " + newUpperLong + ", " + upperBoundAddr + "\n");
            m_stats.instructionsGenerated += 3;
            
        } else {
            // REDIM without PRESERVE: free old, allocate new
            emitComment("REDIM " + arrayName + " (discard old data, release strings if needed)");
            
            // Use array_descriptor_erase to properly clean up old data
            // This handles string element release before freeing the array
            emit("    call $array_descriptor_erase(l " + descPtr + ")\n");
            m_stats.instructionsGenerated++;
            
            // Allocate new data
            std::string newPtr = allocTemp("l");
            emit("    " + newPtr + " =l call $malloc(l " + newSize + ")\n");
            m_stats.instructionsGenerated++;
            
            // Zero-initialize
            emit("    call $memset(l " + newPtr + ", w 0, l " + newSize + ")\n");
            m_stats.instructionsGenerated++;
            
            // Update descriptor: store new data pointer (offset 0)
            emit("    storel " + newPtr + ", " + descPtr + "\n");
            m_stats.instructionsGenerated++;
            
            // Restore lowerBound1 in descriptor (offset 8) - assuming 0-based or 1-based from OPTION BASE
            std::string lowerBoundAddr = allocTemp("l");
            emit("    " + lowerBoundAddr + " =l add " + descPtr + ", 8\n");
            std::string lowerBound = allocTemp("l");
            emit("    " + lowerBound + " =l copy 0\n");  // 0-based by default (TODO: honor OPTION BASE)
            emit("    storel " + lowerBound + ", " + lowerBoundAddr + "\n");
            m_stats.instructionsGenerated += 3;
            
            // Update upperBound in descriptor (offset 16)
            std::string upperBoundAddr = allocTemp("l");
            emit("    " + upperBoundAddr + " =l add " + descPtr + ", 16\n");
            std::string newUpperLong = allocTemp("l");
            emit("    " + newUpperLong + " =l extsw " + dimTemp + "\n");
            emit("    storel " + newUpperLong + ", " + upperBoundAddr + "\n");
            m_stats.instructionsGenerated += 3;
            
            // Restore dimensions in descriptor (offset 48) - 1D array
            std::string dimensionsAddr = allocTemp("l");
            emit("    " + dimensionsAddr + " =l add " + descPtr + ", 48\n");
            std::string dimensionsVal = allocTemp("w");
            emit("    " + dimensionsVal + " =w copy 1\n");
            emit("    storew " + dimensionsVal + ", " + dimensionsAddr + "\n");
            m_stats.instructionsGenerated += 3;
        }
        
        emitComment("REDIM " + arrayName + " complete");
    }
}

// =============================================================================
// SELECT CASE Statement
// =============================================================================

void QBECodeGenerator::emitCase(const CaseStatement* stmt) {
    if (!stmt || !stmt->caseExpression) return;
    
    // SELECT CASE statement - the comparison logic is handled in test blocks
    // which get their information directly from the CFG
    emitComment("SELECT CASE - test blocks will handle comparisons");
}

// =============================================================================
// EXIT Statement
// =============================================================================

void QBECodeGenerator::emitExit(const ExitStatement* stmt) {
    if (!stmt) return;
    
    // Check for EXIT FUNCTION/SUB first
    if (stmt->exitType == ExitStatement::ExitType::FUNCTION ||
        stmt->exitType == ExitStatement::ExitType::SUB) {
        if (m_inFunction && !m_functionStack.empty()) {
            // Jump to tidy_exit for cleanup
            emit("    jmp @" + m_functionStack.top().tidyExitLabel + "\n");
            m_stats.instructionsGenerated++;
            m_lastStatementWasTerminator = true;
        } else {
            emitComment("EXIT FUNCTION/SUB outside function");
        }
        return;
    }
    
    // EXIT FOR/WHILE/DO - jump to loop exit block
    if (!m_loopStack.empty()) {
        LoopContext* loop = getCurrentLoop();
        if (loop && !loop->exitLabel.empty()) {
            emit("    jmp @" + loop->exitLabel + "\n");
            m_stats.instructionsGenerated++;
            m_lastStatementWasTerminator = true;  // EXIT is a terminator
        }
    } else {
        emitComment("EXIT statement outside loop");
    }
}

// =============================================================================
// REPEAT Statement (Loop Header)
// =============================================================================

void QBECodeGenerator::emitRepeat(const RepeatStatement* stmt) {
    // REPEAT just marks the start of the loop
    // The loop body is in the CFG blocks
    // No code to emit here - CFG handles the structure
    emitComment("REPEAT loop header (CFG-driven)");
}

// =============================================================================
// UNTIL Statement (Loop Footer with Condition)
// =============================================================================

void QBECodeGenerator::emitUntil(const UntilStatement* stmt) {
    if (!stmt || !stmt->condition) return;
    
    // Evaluate UNTIL condition
    std::string condTemp = emitExpression(stmt->condition.get());
    
    // Convert to boolean (UNTIL exits when condition is TRUE)
    std::string boolTemp = allocTemp("w");
    emit("    " + boolTemp + " =w cnew " + condTemp + ", 0\n");
    m_stats.instructionsGenerated++;
    
    // Store condition for block emitter to use with CFG successors
    // The block emitter will emit: jnz condition, successor[0], successor[1]
    // where successor[0] is the exit block and successor[1] is the repeat block
    m_lastCondition = boolTemp;
    emitComment("UNTIL condition - CFG will emit branch to exit/repeat blocks");
}

// =============================================================================
// DO Statement (Loop Header)
// =============================================================================

void QBECodeGenerator::emitDo(const DoStatement* stmt) {
    if (!stmt) return;
    
    // DO can have condition at top (DO WHILE/UNTIL) or none (plain DO)
    if (stmt->conditionType == DoStatement::ConditionType::WHILE ||
        stmt->conditionType == DoStatement::ConditionType::UNTIL) {
        // Pre-test loop: evaluate condition before entering loop body
        if (stmt->condition) {
            std::string condTemp = emitExpression(stmt->condition.get());
            std::string boolTemp = allocTemp("w");
            emit("    " + boolTemp + " =w cnew " + condTemp + ", 0\n");
            m_stats.instructionsGenerated++;
            
            // For DO UNTIL, we want to enter loop if condition is FALSE (negate)
            if (stmt->conditionType == DoStatement::ConditionType::UNTIL) {
                std::string negTemp = allocTemp("w");
                emit("    " + negTemp + " =w ceqw " + boolTemp + ", 0\n");
                m_stats.instructionsGenerated++;
                boolTemp = negTemp;
            }
            
            // Store condition for CFG-driven branch
            m_lastCondition = boolTemp;
            emitComment("DO WHILE/UNTIL condition - CFG will branch to body/exit");
        }
    } else {
        // Plain DO - no condition at top
        emitComment("DO loop header (CFG-driven)");
    }
}

// =============================================================================
// LOOP Statement (Loop Footer)
// =============================================================================

void QBECodeGenerator::emitLoop(const LoopStatement* stmt) {
    if (!stmt) return;
    
    // LOOP can have condition at bottom (LOOP WHILE/UNTIL) or none (plain LOOP)
    if (stmt->conditionType == LoopStatement::ConditionType::WHILE ||
        stmt->conditionType == LoopStatement::ConditionType::UNTIL) {
        // Post-test loop: evaluate condition after loop body
        if (stmt->condition) {
            std::string condTemp = emitExpression(stmt->condition.get());
            std::string boolTemp = allocTemp("w");
            emit("    " + boolTemp + " =w cnew " + condTemp + ", 0\n");
            m_stats.instructionsGenerated++;
            
            // For LOOP UNTIL, we want to exit when condition is TRUE
            // For LOOP WHILE, we want to exit when condition is FALSE (negate)
            if (stmt->conditionType == LoopStatement::ConditionType::WHILE) {
                std::string negTemp = allocTemp("w");
                emit("    " + negTemp + " =w ceqw " + boolTemp + ", 0\n");
                m_stats.instructionsGenerated++;
                boolTemp = negTemp;
            }
            
            // Store condition for block emitter to use with CFG successors
            // The block emitter will emit: jnz condition, successor[0], successor[1]
            // where successor[0] is the exit block and successor[1] is the loop header
            m_lastCondition = boolTemp;
            emitComment("LOOP WHILE/UNTIL condition - CFG will emit branch to exit/repeat blocks");
        }
    } else {
        // Plain LOOP - unconditional back edge to DO
        emitComment("LOOP - unconditional branch back to DO (CFG-driven)");
    }
}

// =============================================================================
// LOCAL Statement (Local Variable Declaration)
// =============================================================================

void QBECodeGenerator::emitLocal(const LocalStatement* stmt) {
    if (!stmt) return;
    
    // LOCAL only makes sense inside a function
    if (!m_inFunction) {
        emitComment("LOCAL outside function - treated as global");
        return;
    }
    
    // Register each variable as local and optionally initialize it
    for (const auto& var : stmt->variables) {
        // Add to local variable set
        m_localVariables.insert(var.name);
        
        // Determine type
        VariableType varType = VariableType::UNKNOWN;
        if (var.hasAsType) {
            // Map AS type name to VariableType
            // TODO: Support user-defined types
            std::string typeName = var.asTypeName;
            for (char& c : typeName) c = std::toupper(c);
            
            if (typeName == "INTEGER" || typeName == "INT") {
                varType = VariableType::INT;
            } else if (typeName == "LONG") {
                varType = VariableType::INT;  // LONG is 64-bit integer
            } else if (typeName == "BYTE") {
                varType = VariableType::INT;  // BYTE promotes to INT
            } else if (typeName == "SHORT") {
                varType = VariableType::INT;  // SHORT promotes to INT
            } else if (typeName == "SINGLE" || typeName == "FLOAT") {
                varType = VariableType::FLOAT;
            } else if (typeName == "DOUBLE") {
                varType = VariableType::DOUBLE;
            } else if (typeName == "STRING") {
                varType = VariableType::STRING;
            }
        } else if (var.typeSuffix != TokenType::UNKNOWN) {
            // Use type suffix
            switch (var.typeSuffix) {
                case TokenType::TYPE_INT: varType = VariableType::INT; break;
                case TokenType::PERCENT: varType = VariableType::INT; break;  // % integer suffix
                case TokenType::AMPERSAND: varType = VariableType::INT; break;  // & long suffix (64-bit)
                case TokenType::TYPE_FLOAT: varType = VariableType::FLOAT; break;
                case TokenType::EXCLAMATION: varType = VariableType::FLOAT; break;  // ! float suffix
                case TokenType::TYPE_DOUBLE: varType = VariableType::DOUBLE; break;
                case TokenType::HASH: varType = VariableType::DOUBLE; break;  // # double suffix
                case TokenType::TYPE_STRING: varType = VariableType::STRING; break;
                case TokenType::TYPE_BYTE: varType = VariableType::INT; break;  // @ byte suffix (promote to INT)
                case TokenType::AT_SUFFIX: varType = VariableType::INT; break;  // @ byte suffix (promote to INT)
                case TokenType::TYPE_SHORT: varType = VariableType::INT; break;  // ^ short suffix (promote to INT)
                case TokenType::CARET: varType = VariableType::INT; break;  // ^ short suffix (promote to INT)
                default: varType = VariableType::FLOAT; break;
            }
        } else {
            // Default to INT (most common for untyped LOCAL variables)
            varType = VariableType::INT;
        }
        
        // Store the type in the map for later lookups
        m_localVariableTypes[var.name] = varType;
        
        // Get QBE type
        std::string qbeType = getQBEType(varType);
        
        // Initialize the local variable
        std::string varRef = getVariableRef(var.name);
        
        if (var.initialValue) {
            // Initialize with expression value
            std::string initTemp = emitExpression(var.initialValue.get());
            emit("    " + varRef + " =" + qbeType + " copy " + initTemp + "\n");
        } else {
            // Initialize with default value
            if (varType == VariableType::STRING) {
                emit("    " + varRef + " =l call $basic_empty_string()\n");
            } else if (varType == VariableType::DOUBLE || varType == VariableType::FLOAT) {
                emit("    " + varRef + " =d copy d_0.0\n");
            } else {
                // Use the correct QBE type for integer types (l for 64-bit LONG/INT)
                emit("    " + varRef + " =" + qbeType + " copy 0\n");
            }
        }
        m_stats.instructionsGenerated++;
    }
}

// =============================================================================
// SHARED Statement (Access Global Variables in Function)
// =============================================================================

void QBECodeGenerator::emitShared(const SharedStatement* stmt) {
    if (!stmt) return;
    
    // SHARED only makes sense inside a function
    if (!m_inFunction) {
        emitComment("SHARED outside function - ignored");
        return;
    }
    
    // Register each variable as shared (global access)
    for (const auto& var : stmt->variables) {
        m_sharedVariables.insert(var.name);
        emitComment("SHARED " + var.name);
    }
}

// =============================================================================
// READ Statement
// =============================================================================

void QBECodeGenerator::emitRead(const ReadStatement* stmt) {
    if (!stmt) return;
    
    emitComment("READ statement");
    
    // Read values for each variable
    for (const auto& varName : stmt->variables) {
        std::string varRef = getVariableRef(varName);
        VariableType varType = getVariableType(varName);
        
        if (varType == VariableType::STRING) {
            // basic_read_string() returns a raw C string pointer (const char*)
            // We need to wrap it in a StringDescriptor using string_new_utf8
            std::string strPtr = allocTemp("l");
            emit("    " + strPtr + " =l call $basic_read_string()\n");
            std::string desc = allocTemp("l");
            emit("    " + desc + " =l call $string_new_utf8(l " + strPtr + ")\n");
            emit("    " + varRef + " =l copy " + desc + "\n");
            m_stats.instructionsGenerated += 3;
        } else if (varType == VariableType::INT) {
            // Runtime returns 32-bit int, but INT variables are 64-bit
            std::string intVal = allocTemp("w");
            emit("    " + intVal + " =w call $basic_read_int()\n");
            std::string extVal = allocTemp("l");
            emit("    " + extVal + " =l extsw " + intVal + "\n");
            emit("    " + varRef + " =l copy " + extVal + "\n");
            m_stats.instructionsGenerated += 3;
        } else if (varType == VariableType::DOUBLE || varType == VariableType::FLOAT) {
            std::string dblVal = allocTemp("d");
            emit("    " + dblVal + " =d call $basic_read_double()\n");
            emit("    " + varRef + " =d copy " + dblVal + "\n");
            m_stats.instructionsGenerated += 2;
        } else {
            emitComment("WARNING: Unsupported variable type for READ: " + varName);
        }
    }
}

// =============================================================================
// RESTORE Statement
// =============================================================================

void QBECodeGenerator::emitRestore(const RestoreStatement* stmt) {
    if (!stmt) return;
    
    emitComment("RESTORE statement");
    
    size_t targetIndex = 0;
    
    if (stmt->isLabel) {
        // RESTORE to label
        auto it = m_labelRestorePoints.find(stmt->label);
        if (it != m_labelRestorePoints.end()) {
            targetIndex = it->second;
        } else {
            emitComment("WARNING: RESTORE label not found: " + stmt->label);
            return;
        }
    } else if (stmt->lineNumber > 0) {
        // RESTORE to line number
        auto it = m_lineRestorePoints.find(stmt->lineNumber);
        if (it != m_lineRestorePoints.end()) {
            targetIndex = it->second;
        } else {
            emitComment("WARNING: RESTORE line not found: " + std::to_string(stmt->lineNumber));
            return;
        }
    } else {
        // RESTORE with no argument - reset to beginning (index 0)
        targetIndex = 0;
    }
    
    // Call runtime function to set data pointer
    emit("    call $basic_restore(l " + std::to_string(targetIndex) + ")\n");
    m_stats.instructionsGenerated++;
}

// =============================================================================
// MID$ Assignment: MID$(var$, pos, len) = replacement$
// =============================================================================

void QBECodeGenerator::emitMidAssign(const MidAssignStatement* stmt) {
    if (!stmt || !stmt->position || !stmt->length || !stmt->replacement) return;
    
    emitComment("MID$ assignment: MID$(" + stmt->variable + ", pos, len) = value");
    
    // Get current value of the variable
    std::string currentStr = getVariableRef(stmt->variable);
    
    // Emit arguments
    std::string posTemp = emitExpression(stmt->position.get());
    std::string lenTemp = emitExpression(stmt->length.get());
    std::string replTemp = emitExpression(stmt->replacement.get());
    
    // Ensure position and length are int64_t
    VariableType posType = inferExpressionType(stmt->position.get());
    if (posType == VariableType::DOUBLE) {
        std::string posInt = allocTemp("w");
        emit("    " + posInt + " =w dtosi " + posTemp + "\n");
        posTemp = allocTemp("l");
        emit("    " + posTemp + " =l extsw " + posInt + "\n");
    } else if (posType == VariableType::INT) {
        std::string posLong = allocTemp("l");
        emit("    " + posLong + " =l extsw " + posTemp + "\n");
        posTemp = posLong;
    }
    
    VariableType lenType = inferExpressionType(stmt->length.get());
    if (lenType == VariableType::DOUBLE) {
        std::string lenInt = allocTemp("w");
        emit("    " + lenInt + " =w dtosi " + lenTemp + "\n");
        lenTemp = allocTemp("l");
        emit("    " + lenTemp + " =l extsw " + lenInt + "\n");
    } else if (lenType == VariableType::INT) {
        std::string lenLong = allocTemp("l");
        emit("    " + lenLong + " =l extsw " + lenTemp + "\n");
        lenTemp = lenLong;
    }
    
    // Call runtime function and assign result back
    std::string result = allocTemp("l");
    emit("    " + result + " =l call $string_mid_assign(l " + currentStr + ", l " + posTemp + ", l " + lenTemp + ", l " + replTemp + ")\n");
    emit("    " + currentStr + " =l copy " + result + "\n");
    m_stats.instructionsGenerated += 2;
}

// =============================================================================
// String Slice Assignment: var$(start TO end) = replacement$
// =============================================================================

void QBECodeGenerator::emitSliceAssign(const SliceAssignStatement* stmt) {
    if (!stmt || !stmt->start || !stmt->end || !stmt->replacement) return;
    
    emitComment("String slice assignment: " + stmt->variable + "$(start TO end) = value");
    
    // Get current value of the variable
    std::string currentStr = getVariableRef(stmt->variable);
    
    // Emit arguments
    std::string startTemp = emitExpression(stmt->start.get());
    std::string endTemp = emitExpression(stmt->end.get());
    std::string replTemp = emitExpression(stmt->replacement.get());
    
    // Ensure start and end are int64_t
    VariableType startType = inferExpressionType(stmt->start.get());
    if (startType == VariableType::DOUBLE) {
        std::string startInt = allocTemp("w");
        emit("    " + startInt + " =w dtosi " + startTemp + "\n");
        startTemp = allocTemp("l");
        emit("    " + startTemp + " =l extsw " + startInt + "\n");
    } else if (startType == VariableType::INT) {
        std::string startLong = allocTemp("l");
        emit("    " + startLong + " =l extsw " + startTemp + "\n");
        startTemp = startLong;
    }
    
    VariableType endType = inferExpressionType(stmt->end.get());
    if (endType == VariableType::DOUBLE) {
        std::string endInt = allocTemp("w");
        emit("    " + endInt + " =w dtosi " + endTemp + "\n");
        endTemp = allocTemp("l");
        emit("    " + endTemp + " =l extsw " + endInt + "\n");
    } else if (endType == VariableType::INT) {
        std::string endLong = allocTemp("l");
        emit("    " + endLong + " =l extsw " + endTemp + "\n");
        endTemp = endLong;
    }
    
    // Call runtime function and assign result back
    std::string result = allocTemp("l");
    std::string callStr = "    " + result + " =l call $string_slice_assign(l " + currentStr + ", l " + startTemp + ", l " + endTemp + ", l " + replTemp + ")\n";
    emit(callStr);
    emit("    " + currentStr + " =l copy " + result + "\n");
    m_stats.instructionsGenerated += 2;
}

// =============================================================================
// Terminal I/O Commands
// =============================================================================

void QBECodeGenerator::emitCls(const SimpleStatement* stmt) {
    emitComment("CLS - Clear screen");
    emit("    call $basic_cls()\n");
    m_stats.instructionsGenerated++;
}

void QBECodeGenerator::emitColor(const ExpressionStatement* stmt) {
    emitComment("COLOR - Set text colors");
    
    // COLOR foreground [, background]
    if (stmt->arguments.empty()) {
        // COLOR with no arguments - reset to defaults
        emit("    call $basic_color(w 7, w 0)\n");
        m_stats.instructionsGenerated++;
        return;
    }
    
    std::string foreground = emitExpression(stmt->arguments[0].get());
    std::string background = "0";  // Default black background
    
    if (stmt->arguments.size() > 1) {
        background = emitExpression(stmt->arguments[1].get());
    }
    
    // Ensure values are words (int32)
    if (inferExpressionType(stmt->arguments[0].get()) == VariableType::DOUBLE) {
        std::string fgInt = allocTemp("w");
        emit("    " + fgInt + " =w dtosi " + foreground + "\n");
        foreground = fgInt;
        m_stats.instructionsGenerated++;
    }
    
    if (stmt->arguments.size() > 1 && inferExpressionType(stmt->arguments[1].get()) == VariableType::DOUBLE) {
        std::string bgInt = allocTemp("w");
        emit("    " + bgInt + " =w dtosi " + background + "\n");
        background = bgInt;
        m_stats.instructionsGenerated++;
    }
    
    emit("    call $basic_color(w " + foreground + ", w " + background + ")\n");
    m_stats.instructionsGenerated++;
}

void QBECodeGenerator::emitLocate(const ExpressionStatement* stmt) {
    emitComment("LOCATE - Move cursor to position");
    
    // LOCATE row, col
    if (stmt->arguments.size() < 2) {
        emitComment("ERROR: LOCATE requires 2 arguments");
        return;
    }
    
    std::string row = emitExpression(stmt->arguments[0].get());
    std::string col = emitExpression(stmt->arguments[1].get());
    
    // Convert to int32 if needed
    if (inferExpressionType(stmt->arguments[0].get()) == VariableType::DOUBLE) {
        std::string rowInt = allocTemp("w");
        emit("    " + rowInt + " =w dtosi " + row + "\n");
        row = rowInt;
        m_stats.instructionsGenerated++;
    }
    
    if (inferExpressionType(stmt->arguments[1].get()) == VariableType::DOUBLE) {
        std::string colInt = allocTemp("w");
        emit("    " + colInt + " =w dtosi " + col + "\n");
        col = colInt;
        m_stats.instructionsGenerated++;
    }
    
    emit("    call $basic_locate(w " + row + ", w " + col + ")\n");
    m_stats.instructionsGenerated++;
}

void QBECodeGenerator::emitWidth(const ExpressionStatement* stmt) {
    emitComment("WIDTH - Set terminal width");
    
    if (stmt->arguments.empty()) {
        emitComment("ERROR: WIDTH requires 1 argument");
        return;
    }
    
    std::string columns = emitExpression(stmt->arguments[0].get());
    
    // Convert to int32 if needed
    if (inferExpressionType(stmt->arguments[0].get()) == VariableType::DOUBLE) {
        std::string colInt = allocTemp("w");
        emit("    " + colInt + " =w dtosi " + columns + "\n");
        columns = colInt;
        m_stats.instructionsGenerated++;
    }
    
    emit("    call $basic_width(w " + columns + ")\n");
    m_stats.instructionsGenerated++;
}

void QBECodeGenerator::emitTryCatch(const TryCatchStatement* stmt) {
    emitComment("TRY/CATCH/FINALLY - Exception handling (TODO: complete implementation)");
    emitComment("NOTE: Exception handling code generation not yet complete");
    
    // For now, just emit a placeholder to avoid hanging
    // The TRY block statements will be emitted by normal block processing
}

void QBECodeGenerator::emitThrow(const ThrowStatement* stmt) {
    emitComment("THROW - Raise exception");
    
    if (!stmt->errorCode) {
        emitComment("ERROR: THROW requires error code expression");
        return;
    }
    
    // Evaluate error code expression
    std::string codeTemp = emitExpression(stmt->errorCode.get());
    VariableType codeType = inferExpressionType(stmt->errorCode.get());
    
    // Convert to int32 if needed
    std::string codeInt = codeTemp;
    if (codeType == VariableType::DOUBLE) {
        codeInt = allocTemp("w");
        emit("    " + codeInt + " =w dtosi " + codeTemp + "\n");
        m_stats.instructionsGenerated++;
    } else if (codeType == VariableType::FLOAT) {
        codeInt = allocTemp("w");
        emit("    " + codeInt + " =w stosi " + codeTemp + "\n");
        m_stats.instructionsGenerated++;
    }
    
    // Call basic_throw - this will longjmp and not return
    emit("    call $basic_throw(w " + codeInt + ")\n");
    m_stats.instructionsGenerated++;
    
    // Mark as unreachable (won't return)
    emitComment("Unreachable - THROW does not return");
}

} // namespace FasterBASIC