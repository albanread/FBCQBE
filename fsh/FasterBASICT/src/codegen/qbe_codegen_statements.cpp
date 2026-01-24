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
    
    // Check if this is array assignment or variable assignment
    if (stmt->indices.empty()) {
        // Simple variable assignment
        std::string varRef = getVariableRef(stmt->variable);
        VariableType varType = getVariableType(stmt->variable);
        std::string qbeType = getQBEType(varType);
        
        // QBE requires explicit type conversions
        // Always convert int literals to double when assigning to double variables
        std::string convertedValue = valueTemp;
        
        // Check if we need int->double conversion
        // If target is double and source looks like int (number literal, binary op result, etc.)
        if (qbeType == "d") {
            // Always convert to double - this handles number literals and int expressions
            std::string convertTemp = allocTemp("d");
            emit("    " + convertTemp + " =d swtof " + valueTemp + "\n");
            convertedValue = convertTemp;
            m_stats.instructionsGenerated++;
        } else if (qbeType == "w" && stmt->value->getType() == ASTNodeType::EXPR_FUNCTION_CALL) {
            // May need double->int conversion (truncate)
            // For now just copy - will fix if needed
            convertedValue = valueTemp;
        }
        
        emit("    " + varRef + " =" + qbeType + " copy " + convertedValue + "\n");
        m_stats.instructionsGenerated++;
    } else {
        // Array element assignment
        std::vector<std::string> indexTemps;
        for (const auto& indexExpr : stmt->indices) {
            indexTemps.push_back(emitExpression(indexExpr.get()));
        }
        emitArrayStore(stmt->variable, indexTemps, valueTemp);
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
    std::string varRef = getVariableRef(stmt->variable);
    emit("    " + varRef + " =w copy " + startTemp + "\n");
    m_stats.instructionsGenerated++;
    
    // Emit step value (default 1)
    std::string stepTemp;
    if (stmt->step) {
        stepTemp = emitExpression(stmt->step.get());
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
        
        // Jump to function exit
        emit("    jmp @exit\n");
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
        
        // Evaluate dimension expressions
        std::vector<std::string> dimTemps;
        for (const auto& dimExpr : arrayDecl.dimensions) {
            dimTemps.push_back(emitExpression(dimExpr.get()));
        }
        
        // Create array
        std::string arrayPtr = emitArrayCreate(arrayName, dimTemps);
        std::string arrayRef = getArrayRef(arrayName);
        emit("    " + arrayRef + " =l copy " + arrayPtr + "\n");
        m_stats.instructionsGenerated++;
    }
}

// =============================================================================
// END Statement
// =============================================================================

void QBECodeGenerator::emitEnd(const EndStatement* stmt) {
    emit("    jmp @exit\n");
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
    
    // CALL statement - invoke subroutine
    // TODO: Implement based on actual CallStatement structure
    emitComment("CALL statement (TODO: implement)");
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
        if (m_inFunction) {
            // Jump to function exit block
            emit("    jmp @exit\n");
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

} // namespace FasterBASIC