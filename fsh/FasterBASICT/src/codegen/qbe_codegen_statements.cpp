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
            
        case ASTNodeType::STMT_IF:
            emitIf(static_cast<const IfStatement*>(stmt));
            break;
            
        case ASTNodeType::STMT_FOR:
            emitFor(static_cast<const ForStatement*>(stmt));
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
            
            std::string indexTemp = indexTemps[0];
            std::string indexLong = allocTemp("l");
            emit("    " + indexLong + " =l extsw " + indexTemp + "\n");
            m_stats.instructionsGenerated++;
            
            std::string offsetTemp = allocTemp("l");
            emit("    " + offsetTemp + " =l mul " + indexLong + ", " + std::to_string(elementSize) + "\n");
            m_stats.instructionsGenerated++;
            
            baseRef = allocTemp("l");
            emit("    " + baseRef + " =l add " + arrayRef + ", " + offsetTemp + "\n");
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
        
        VariableType fieldType = finalField->builtInType;
        
        // 5. Type conversion if needed
        VariableType exprType = inferExpressionType(stmt->value.get());
        if (fieldType != exprType) {
            valueTemp = promoteToType(valueTemp, exprType, fieldType);
        }
        
        // 6. Store value
        std::string qbeType = getQBEType(fieldType);
        if (qbeType == "w") {
            emit("    storew " + valueTemp + ", " + currentPtr + "\n");
        } else if (qbeType == "d") {
            emit("    stored " + valueTemp + ", " + currentPtr + "\n");
        } else if (qbeType == "l") {
            emit("    storel " + valueTemp + ", " + currentPtr + "\n");
        } else {
            // Default to word
            emit("    storew " + valueTemp + ", " + currentPtr + "\n");
        }
        
        m_stats.instructionsGenerated++;
    } else if (stmt->indices.empty()) {
        // Simple variable assignment
        std::string varRef = getVariableRef(stmt->variable);
        VariableType varType = getVariableType(stmt->variable);
        std::string qbeType = getQBEType(varType);
        
        // Infer the type of the expression value
        VariableType exprType = inferExpressionType(stmt->value.get());
        
        // Convert value to match variable type if needed
        if (varType != exprType) {
            // promoteToType emits the conversion and returns the result temp
            std::string convertedValue = promoteToType(valueTemp, exprType, varType);
            emit("    " + varRef + " =" + qbeType + " copy " + convertedValue + "\n");
        } else {
            // Types match, just copy
            emit("    " + varRef + " =" + qbeType + " copy " + valueTemp + "\n");
        }
        m_stats.instructionsGenerated++;
    } else {
        // Array element assignment using descriptor-based approach
        emitComment("Array assignment: " + stmt->variable + "(...) = value");
        
        // Get element pointer (this includes bounds checking)
        std::string elementPtr = emitArrayElementPtr(stmt->variable, stmt->indices);
        
        // Determine value type and store
        VariableType valueType = inferExpressionType(stmt->value.get());
        
        // Store value at element pointer
        if (valueType == VariableType::INT) {
            emit("    storew " + valueTemp + ", " + elementPtr + "\n");
        } else if (valueType == VariableType::DOUBLE || valueType == VariableType::FLOAT) {
            emit("    stored " + valueTemp + ", " + elementPtr + "\n");
        } else if (valueType == VariableType::STRING) {
            emit("    storel " + valueTemp + ", " + elementPtr + "\n");
        } else {
            // Default to word storage
            emit("    storew " + valueTemp + ", " + elementPtr + "\n");
        }
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
    
    // Check if this is an inline IF (with thenStatements) or block-level IF (CFG-driven)
    bool isInlineIF = !stmt->thenStatements.empty();
    bool isBlockLevelIF = (m_currentBlock && m_currentBlock->successors.size() == 2);
    
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
    } else if (isBlockLevelIF) {
        // Block-level IF - store condition for block emitter to use with CFG successors
        m_lastCondition = boolTemp;
        emitComment("Condition stored for CFG-driven branch to successors");
    } else {
        // Fallback: store condition anyway
        m_lastCondition = boolTemp;
    }
}

// =============================================================================
// FOR Statement (Loop Header)
// =============================================================================

void QBECodeGenerator::emitFor(const ForStatement* stmt) {
    if (!stmt || !stmt->start || !stmt->end) return;
    
    // Mark FOR loop variable as integer (hard rule: FOR indices are always integers)
    m_forLoopVariables.insert(stmt->variable);
    
    // Set up loop context for EXIT FOR to work
    // Find the exit block by looking at CFG structure
    // The current block should have a successor that leads to the loop exit
    if (m_currentBlock && m_cfg) {
        // Look for a block labeled "After FOR" or the exit block
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
            pushLoop(exitLabel, "", "FOR");
        }
    }
    
    // Initialize loop variable
    std::string startTemp = emitExpression(stmt->start.get());
    
    // Coerce start value to INTEGER (FOR loop variables are ALWAYS integers)
    VariableType startType = inferExpressionType(stmt->start.get());
    startTemp = promoteToType(startTemp, startType, VariableType::INT);
    
    std::string varRef = getVariableRef(stmt->variable);
    emit("    " + varRef + " =w copy " + startTemp + "\n");
    m_stats.instructionsGenerated++;
    
    // Emit step value (default 1)
    std::string stepTemp;
    if (stmt->step) {
        stepTemp = emitExpression(stmt->step.get());
        
        // Coerce step value to INTEGER (ALWAYS)
        VariableType stepType = inferExpressionType(stmt->step.get());
        stepTemp = promoteToType(stepTemp, stepType, VariableType::INT);
    } else {
        stepTemp = allocTemp("w");
        emit("    " + stepTemp + " =w copy 1\n");
        m_stats.instructionsGenerated++;
    }
    
    // Store step for NEXT statement
    std::string stepVar = "%step_" + stmt->variable;
    emit("    " + stepVar + " =w copy " + stepTemp + "\n");
    m_stats.instructionsGenerated++;
    
    // Emit end value (evaluate once and store)
    std::string endTemp = emitExpression(stmt->end.get());
    
    // Coerce end value to INTEGER (ALWAYS)
    VariableType endType = inferExpressionType(stmt->end.get());
    endTemp = promoteToType(endTemp, endType, VariableType::INT);
    
    std::string endVar = "%end_" + stmt->variable;
    emit("    " + endVar + " =w copy " + endTemp + "\n");
    m_stats.instructionsGenerated++;
    
    // Note: Loop condition check happens in the FOR check block (separate block)
    // This init block just sets up the loop variables and falls through to check block
}

// =============================================================================
// NEXT Statement (Loop Footer)
// =============================================================================

void QBECodeGenerator::emitNext(const NextStatement* stmt) {
    if (!stmt) return;
    
    std::string varName = stmt->variable.empty() ? "I" : stmt->variable;
    std::string varRef = getVariableRef(varName);
    std::string stepVar = "%step_" + varName;
    
    // Increment loop variable: var = var + step
    std::string newValueTemp = allocTemp("w");
    emit("    " + newValueTemp + " =w add " + varRef + ", " + stepVar + "\n");
    emit("    " + varRef + " =w copy " + newValueTemp + "\n");
    m_stats.instructionsGenerated += 2;
    
    // The condition check happens at the FOR header (loop header block)
    // NEXT just increments and jumps back (CFG provides the back-edge)
    emitComment("NEXT: increment and jump back to FOR header");
    
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
    
    // GOSUB needs return address tracking
    // For now, emit a simple jump (proper implementation needs return stack)
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
        targetBlock = m_cfg->getBlockForLine(stmt->lineNumber);
    }
    
    if (targetBlock >= 0) {
        std::string targetLabel = getBlockLabel(targetBlock);
        emitComment("GOSUB (TODO: implement return stack)");
        emit("    jmp @" + targetLabel + "\n");
        m_stats.instructionsGenerated++;
        m_lastStatementWasTerminator = true;  // GOSUB is a terminator
    } else {
        emitComment("ERROR: GOSUB target not found");
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
            if (m_cfg && m_cfg->returnType == VariableType::DOUBLE) {
                qbeType = "d";
            } else if (m_cfg && m_cfg->returnType == VariableType::FLOAT) {
                qbeType = "d";
            } else if (m_cfg && m_cfg->returnType == VariableType::STRING) {
                qbeType = "l";
            }
            
            // Assign to return variable
            emit("    " + returnVar + " =" + qbeType + " copy " + valueTemp + "\n");
            m_stats.instructionsGenerated++;
        }
        
        // Jump to function exit (tidy_exit for cleanup)
        emit("    jmp @" + getFunctionExitLabel() + "\n");
        m_stats.instructionsGenerated++;
        m_lastStatementWasTerminator = true;
    } else {
        // RETURN from GOSUB
        // TODO: Implement return stack for GOSUB
        emitComment("RETURN from GOSUB (TODO: implement return stack)");
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
        
        // For now, only support 1D arrays
        if (dimTemps.size() != 1) {
            emitComment("ERROR: Multi-dimensional arrays not yet supported with descriptors");
            continue;
        }
        
        // Determine element size
        size_t elementSize;
        if (isUDTArray) {
            elementSize = calculateTypeSize(udtTypeName);
            m_arrayElementTypes[arrayName] = udtTypeName;
        } else {
            // Regular BASIC types: INT=4, DOUBLE=8, STRING=8 (pointer)
            elementSize = 8; // Default to 8 for now (handles most types)
        }
        
        std::string dimTemp = dimTemps[0];
        
        // Convert dimension to INT if needed
        VariableType dimType = inferExpressionType(arrayDecl.dimensions[0].get());
        if (dimType != VariableType::INT) {
            dimTemp = promoteToType(dimTemp, dimType, VariableType::INT);
        }
        
        // Get array descriptor address (assuming descriptor is stored with array reference)
        // For now, arrayRef points to the descriptor structure
        std::string descPtr = arrayRef;
        
        // Calculate bounds (BASIC: DIM A(N) creates 0 to N, or 1 to N with OPTION BASE 1)
        // We'll use 0-based for simplicity (lowerBound=0, upperBound=N)
        int64_t lowerBound = 0; // TODO: respect OPTION BASE
        
        // upperBound = dimTemp (N)
        std::string upperBoundTemp = dimTemp;
        
        // count = upperBound - lowerBound + 1 = N + 1
        std::string countTemp = allocTemp("w");
        emit("    " + countTemp + " =w add " + upperBoundTemp + ", 1\n");
        m_stats.instructionsGenerated++;
        
        // Convert to long for size calculations
        std::string countLong = allocTemp("l");
        emit("    " + countLong + " =l extsw " + countTemp + "\n");
        m_stats.instructionsGenerated++;
        
        // totalBytes = count * elementSize
        std::string totalBytes = allocTemp("l");
        emit("    " + totalBytes + " =l mul " + countLong + ", " + std::to_string(elementSize) + "\n");
        m_stats.instructionsGenerated++;
        
        // Allocate array data
        std::string dataPtr = allocTemp("l");
        emit("    " + dataPtr + " =l call $malloc(l " + totalBytes + ")\n");
        m_stats.instructionsGenerated++;
        
        // Zero-initialize the array
        emit("    call $memset(l " + dataPtr + ", w 0, l " + totalBytes + ")\n");
        m_stats.instructionsGenerated++;
        
        // Initialize descriptor fields
        // ArrayDescriptor layout:
        //   offset 0:  data pointer (8 bytes)
        //   offset 8:  lowerBound (8 bytes)
        //   offset 16: upperBound (8 bytes)
        //   offset 24: elementSize (8 bytes)
        //   offset 32: dimensions (4 bytes)
        
        // Store data pointer at offset 0
        emit("    storel " + dataPtr + ", " + descPtr + "\n");
        m_stats.instructionsGenerated++;
        
        // Store lowerBound at offset 8
        std::string lowerBoundAddr = allocTemp("l");
        emit("    " + lowerBoundAddr + " =l add " + descPtr + ", 8\n");
        emit("    storel " + std::to_string(lowerBound) + ", " + lowerBoundAddr + "\n");
        m_stats.instructionsGenerated += 2;
        
        // Store upperBound at offset 16
        std::string upperBoundAddr = allocTemp("l");
        emit("    " + upperBoundAddr + " =l add " + descPtr + ", 16\n");
        std::string upperBoundLong = allocTemp("l");
        emit("    " + upperBoundLong + " =l extsw " + upperBoundTemp + "\n");
        emit("    storel " + upperBoundLong + ", " + upperBoundAddr + "\n");
        m_stats.instructionsGenerated += 3;
        
        // Store elementSize at offset 24
        std::string elemSizeAddr = allocTemp("l");
        emit("    " + elemSizeAddr + " =l add " + descPtr + ", 24\n");
        emit("    storel " + std::to_string(elementSize) + ", " + elemSizeAddr + "\n");
        m_stats.instructionsGenerated += 2;
        
        // Store dimensions at offset 32
        std::string dimCountAddr = allocTemp("l");
        emit("    " + dimCountAddr + " =l add " + descPtr + ", 32\n");
        emit("    storew 1, " + dimCountAddr + "\n"); // 1 dimension
        m_stats.instructionsGenerated += 2;
        
        emitComment("Array " + arrayName + " descriptor initialized (element size: " + std::to_string(elementSize) + " bytes)");
        
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
        // Call runtime library function (shouldn't normally happen with CALL, but handle it)
        std::string runtimeFunc = mapToRuntimeFunction(subName);
        std::string resultTemp = allocTemp("w");
        
        emit("    " + resultTemp + " =w call $" + runtimeFunc + "(");
        
        for (size_t i = 0; i < convertedArgTemps.size(); ++i) {
            if (i > 0) emit(", ");
            emit(convertedArgTypes[i] + " " + convertedArgTemps[i]);
        }
        
        emit(")\n");
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
        
        // Load data pointer from descriptor (offset 0)
        std::string dataPtr = allocTemp("l");
        emit("    " + dataPtr + " =l loadl " + descPtr + "\n");
        m_stats.instructionsGenerated++;
        
        // Free the data
        emit("    call $free(l " + dataPtr + ")\n");
        m_stats.instructionsGenerated++;
        
        // Set data pointer to NULL (offset 0)
        emit("    storel 0, " + descPtr + "\n");
        m_stats.instructionsGenerated++;
        
        // Set upperBound to -1 to indicate empty array (offset 16)
        std::string upperBoundAddr = allocTemp("l");
        emit("    " + upperBoundAddr + " =l add " + descPtr + ", 16\n");
        emit("    storel -1, " + upperBoundAddr + "\n");
        m_stats.instructionsGenerated += 2;
        
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
        
        // Load element size from descriptor (offset 24)
        std::string elemSizeAddr = allocTemp("l");
        emit("    " + elemSizeAddr + " =l add " + descPtr + ", 24\n");
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
            emitComment("REDIM " + arrayName + " (discard old data)");
            
            // Load old data pointer (offset 0)
            std::string oldPtr = allocTemp("l");
            emit("    " + oldPtr + " =l loadl " + descPtr + "\n");
            m_stats.instructionsGenerated++;
            
            // Free old data
            emit("    call $free(l " + oldPtr + ")\n");
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
            
            // Update upperBound in descriptor (offset 16)
            std::string upperBoundAddr = allocTemp("l");
            emit("    " + upperBoundAddr + " =l add " + descPtr + ", 16\n");
            std::string newUpperLong = allocTemp("l");
            emit("    " + newUpperLong + " =l extsw " + dimTemp + "\n");
            emit("    storel " + newUpperLong + ", " + upperBoundAddr + "\n");
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
    
    emitComment("SELECT CASE - evaluate expression");
    
    // Evaluate the SELECT CASE expression once and store it
    m_selectCaseValue = emitExpression(stmt->caseExpression.get());
    
    // Store the CASE values for test blocks to use
    m_caseClauseValues.clear();
    m_currentCaseClauseIndex = 0;
    
    for (const auto& clause : stmt->whenClauses) {
        std::vector<std::string> values;
        for (const auto& valueExpr : clause.values) {
            std::string valueTemp = emitExpression(valueExpr.get());
            values.push_back(valueTemp);
        }
        m_caseClauseValues.push_back(values);
    }
    
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
                case TokenType::TYPE_FLOAT: varType = VariableType::FLOAT; break;
                case TokenType::TYPE_DOUBLE: varType = VariableType::DOUBLE; break;
                case TokenType::TYPE_STRING: varType = VariableType::STRING; break;
                default: varType = VariableType::FLOAT; break;
            }
        } else {
            // Default to INT (most common for untyped LOCAL variables)
            varType = VariableType::INT;
        }
        
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
                emit("    " + varRef + " =w copy 0\n");
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
            std::string strPtr = allocTemp("l");
            emit("    " + strPtr + " =l call $basic_read_string()\n");
            emit("    " + varRef + " =l copy " + strPtr + "\n");
            m_stats.instructionsGenerated += 2;
        } else if (varType == VariableType::INT) {
            std::string intVal = allocTemp("w");
            emit("    " + intVal + " =w call $basic_read_int()\n");
            emit("    " + varRef + " =w copy " + intVal + "\n");
            m_stats.instructionsGenerated += 2;
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

} // namespace FasterBASIC