//
// cfg_builder_statements.cpp
// FasterBASIC - Control Flow Graph Builder Statement Processing
//
// Contains Phase 1 block building and main statement dispatcher.
// Part of modular CFG builder split (February 2026).
//

#include "cfg_builder.h"
#include <algorithm>
#include <sstream>
#include <iostream>
#include <functional>

namespace FasterBASIC {

// =============================================================================
// Phase 1: Build Basic Blocks
// =============================================================================

void CFGBuilder::buildBlocks(const Program& program, const std::set<int>& jumpTargets) {
    // Create entry block for main program
    BasicBlock* entryBlock = createNewBlock("Entry");
    m_currentCFG->entryBlock = entryBlock->id;
    m_currentBlock = entryBlock;
    
    // Track if we've hit an END statement - code after END is only reachable via GOSUB
    bool afterEnd = false;
    
    // Process each program line
    for (const auto& line : program.lines) {
        int lineNumber = line->lineNumber;
        
        // If this line is a jump target, start a new block
        if (lineNumber > 0 && jumpTargets.count(lineNumber) > 0) {
            // Only create new block if current block is not empty
            if (!m_currentBlock->statements.empty() || !m_currentBlock->lineNumbers.empty()) {
                BasicBlock* targetBlock = createNewBlock("Target_" + std::to_string(lineNumber));
                // Add fallthrough edge from previous block if it doesn't end with a jump
                // and we haven't encountered END yet
                if (!afterEnd && !m_currentBlock->statements.empty()) {
                    const Statement* lastStmt = m_currentBlock->statements.back();
                    ASTNodeType lastType = lastStmt->getType();
                    if (lastType != ASTNodeType::STMT_GOTO && 
                        lastType != ASTNodeType::STMT_END &&
                        lastType != ASTNodeType::STMT_RETURN &&
                        lastType != ASTNodeType::STMT_EXIT) {
                        // Fallthrough will be added in buildEdges phase
                    }
                }
                m_currentBlock = targetBlock;
            }
        }
        
        // Map line number to current block
        if (lineNumber > 0) {
            m_currentCFG->mapLineToBlock(lineNumber, m_currentBlock->id);
            m_currentBlock->addLineNumber(lineNumber);
        }
        
        // Process each statement in the line
        for (const auto& stmt : line->statements) {
            processStatement(*stmt, m_currentBlock, lineNumber);
            
            // Check if this statement is END - if so, subsequent code is only reachable via GOSUB
            // BUT: We need to distinguish standalone END from END IF, END SELECT, etc.
            // Check if this is truly a standalone END statement (EndStatement class)
            if (stmt->getType() == ASTNodeType::STMT_END) {
                // Check if it's actually an EndStatement object (not part of END IF, etc.)
                const EndStatement* endStmt = dynamic_cast<const EndStatement*>(stmt.get());
                if (endStmt != nullptr) {
                    afterEnd = true;
                }
            }
        }
    }
    
    // Create exit block if requested
    if (m_createExitBlock) {
        BasicBlock* exitBlock = createNewBlock("Exit");
        exitBlock->isTerminator = true;
        m_currentCFG->exitBlock = exitBlock->id;
        
        // Connect last block to exit
        if (m_currentBlock && m_currentBlock->id != exitBlock->id) {
            addFallthroughEdge(m_currentBlock->id, exitBlock->id);
        }
    }
}

// =============================================================================
// Statement Processing
// =============================================================================

void CFGBuilder::processStatement(const Statement& stmt, BasicBlock* currentBlock, int lineNumber) {
    // Handle control flow statements
    ASTNodeType type = stmt.getType();
    

    // Don't add FUNCTION/SUB/DEF statements to main CFG - they define separate CFGs
    // Add all statements to current block (except functions/subs)
    if (type != ASTNodeType::STMT_FUNCTION && type != ASTNodeType::STMT_SUB && type != ASTNodeType::STMT_DEF) {
        // Add statement to current block with its line number
        currentBlock->addStatement(&stmt, lineNumber);
    }
    
    switch (type) {
        case ASTNodeType::STMT_TRY_CATCH:
            processTryCatchStatement(static_cast<const TryCatchStatement&>(stmt), currentBlock);
            break;
            
        case ASTNodeType::STMT_LABEL:
            processLabelStatement(static_cast<const LabelStatement&>(stmt), currentBlock);
            break;
            
        case ASTNodeType::STMT_GOTO:
            processGotoStatement(static_cast<const GotoStatement&>(stmt), currentBlock);
            break;
            
        case ASTNodeType::STMT_GOSUB:
            processGosubStatement(static_cast<const GosubStatement&>(stmt), currentBlock);
            break;
            
        case ASTNodeType::STMT_ON_GOTO:
            processOnGotoStatement(static_cast<const OnGotoStatement&>(stmt), currentBlock);
            break;
            
        case ASTNodeType::STMT_ON_GOSUB:
            processOnGosubStatement(static_cast<const OnGosubStatement&>(stmt), currentBlock);
            currentBlock->isTerminator = true;
            break;
            
        case ASTNodeType::STMT_IF:
            processIfStatement(static_cast<const IfStatement&>(stmt), currentBlock);
            break;
            
        case ASTNodeType::STMT_FOR:
            processForStatement(static_cast<const ForStatement&>(stmt), currentBlock);
            break;
            
        case ASTNodeType::STMT_FOR_IN:
            processForInStatement(static_cast<const ForInStatement&>(stmt), currentBlock);
            break;
            
        case ASTNodeType::STMT_WHILE:
            processWhileStatement(static_cast<const WhileStatement&>(stmt), currentBlock);
            break;
            
        case ASTNodeType::STMT_REPEAT:
            processRepeatStatement(static_cast<const RepeatStatement&>(stmt), currentBlock);
            break;
            
        case ASTNodeType::STMT_DO:
            processDoStatement(static_cast<const DoStatement&>(stmt), currentBlock);
            break;
            
        case ASTNodeType::STMT_CASE:
            processCaseStatement(static_cast<const CaseStatement&>(stmt), currentBlock);
            break;
        
        case ASTNodeType::STMT_THROW:
            // THROW is a terminator (throws exception and doesn't return normally)
            currentBlock->isTerminator = true;
            break;
            
        case ASTNodeType::STMT_FUNCTION:
            processFunctionStatement(static_cast<const FunctionStatement&>(stmt), currentBlock);
            break;
            
        case ASTNodeType::STMT_SUB:
            processSubStatement(static_cast<const SubStatement&>(stmt), currentBlock);
            break;
            
        case ASTNodeType::STMT_DEF:
            processDefStatement(static_cast<const DefStatement&>(stmt), currentBlock);
            break;
            
        case ASTNodeType::STMT_NEXT:
            {
                // NEXT creates the incrementor block and the exit block
                // This is "The Closer" - where the loop structure is finalized
                const NextStatement& nextStmt = static_cast<const NextStatement&>(stmt);
                LoopContext* matchingLoop = nullptr;
                
                // Find the matching loop by variable name (or innermost if no variable specified)
                for (auto it = m_loopStack.rbegin(); it != m_loopStack.rend(); ++it) {
                    if (nextStmt.variable.empty() || it->variable == nextStmt.variable) {
                        matchingLoop = &(*it);
                        break;
                    }
                }
                
                if (matchingLoop) {
                    // 1. Create the NEXT block itself (the incrementor block)
                    BasicBlock* nextBlock = createNewBlock("FOR Next/Increment");
                    
                    // 2. Move the NEXT statement from current block to NEXT block
                    //    (it was already added to currentBlock at line 276)
                    if (!currentBlock->statements.empty() && currentBlock->statements.back() == &stmt) {
                        // Get the line number for this statement
                        int lineNum = 0;
                        auto it = currentBlock->statementLineNumbers.find(&stmt);
                        if (it != currentBlock->statementLineNumbers.end()) {
                            lineNum = it->second;
                        }
                        
                        // Remove from current block and add to NEXT block
                        currentBlock->statements.pop_back();
                        currentBlock->statementLineNumbers.erase(&stmt);
                        nextBlock->addStatement(&stmt, lineNum);
                    }
                    
                    // 3. Current block (end of loop body) flows into NEXT block
                    //    (unless it's a terminator)
                    if (!currentBlock->isTerminator && currentBlock->successors.empty()) {
                        addFallthroughEdge(currentBlock->id, nextBlock->id);
                    }
                    
                    // 4. Record the mapping from NEXT block to loop header for buildEdges
                    //    (NEXT block always jumps back to the Check block)
                    m_nextToHeaderMap[nextBlock->id] = matchingLoop->headerBlock;
                    
                    // 5. NOW create the Exit block - its ID will be higher than everything in the body
                    BasicBlock* loopExit = createNewBlock("After FOR");
                    loopExit->isLoopExit = true;
                    
                    // 6. Update the loop context with the exit block ID
                    matchingLoop->exitBlock = loopExit->id;
                    
                    // 7. Wire all pending EXIT FOR blocks to the exit block
                    for (int exitBlockId : matchingLoop->pendingExitBlocks) {
                        addFallthroughEdge(exitBlockId, loopExit->id);
                    }
                    
                    // 8. Update the FOR loop structure if it exists
                    for (auto& pair : m_currentCFG->forLoopStructure) {
                        if (pair.second.checkBlock == matchingLoop->headerBlock) {
                            pair.second.exitBlock = loopExit->id;
                            break;
                        }
                    }
                    
                    // 9. Switch to the exit block for subsequent statements
                    m_currentBlock = loopExit;
                    
                    // 10. Pop this loop context - we're done with this loop
                    m_loopStack.erase(std::remove_if(m_loopStack.begin(), m_loopStack.end(),
                        [matchingLoop](const LoopContext& ctx) { return &ctx == matchingLoop; }), 
                        m_loopStack.end());
                } else {
                    // Fallback: create a new block if no matching loop found
                    BasicBlock* afterLoop = createNewBlock("After NEXT");
                    m_currentBlock = afterLoop;
                }
            }
            break;
            
        case ASTNodeType::STMT_WEND:
            // WEND ends the loop body and starts a new block for code after the loop
            {
                BasicBlock* nextBlock = createNewBlock("After WHILE");
                m_currentBlock = nextBlock;
                
                // Don't pop loop context here - buildEdges() needs it to create back edge
            }
            break;
            
        case ASTNodeType::STMT_LOOP:
            // LOOP ends the loop body and starts a new block for code after the loop
            {
                BasicBlock* nextBlock = createNewBlock("After DO");
                m_currentBlock = nextBlock;
                // Don't pop loop context - buildEdges() needs it
            }
            break;
            
        case ASTNodeType::STMT_UNTIL:
            // UNTIL ends the loop body and starts a new block for code after the loop
            {
                BasicBlock* nextBlock = createNewBlock("After REPEAT");
                m_currentBlock = nextBlock;
                
                // Don't pop loop context here - buildEdges() needs it to create back edge
            }
            break;
            
        case ASTNodeType::STMT_RETURN:
        case ASTNodeType::STMT_END:
            // Only mark as terminator if we're not processing nested statements
            // (nested statements in IF branches shouldn't terminate the parent block)
            if (!m_processingNestedStatements) {
                currentBlock->isTerminator = true;
            }
            break;
            
        case ASTNodeType::STMT_EXIT:
            {
                // EXIT handling: for EXIT FOR, track pending exit blocks
                const ExitStatement& exitStmt = static_cast<const ExitStatement&>(stmt);
                
                if (exitStmt.exitType == ExitStatement::ExitType::FOR_LOOP) {
                    // EXIT FOR - add to pending exits for the innermost FOR loop
                    if (!m_loopStack.empty()) {
                        // Find innermost FOR loop
                        for (auto it = m_loopStack.rbegin(); it != m_loopStack.rend(); ++it) {
                            if (!it->variable.empty()) {  // FOR loops have a variable
                                it->pendingExitBlocks.push_back(currentBlock->id);
                                break;
                            }
                        }
                    }
                    // Create a new block after the EXIT FOR statement
                    // (this block will be unreachable but maintains CFG structure)
                    BasicBlock* afterExit = createNewBlock("After EXIT FOR");
                    m_currentBlock = afterExit;
                }
                
                // Mark as terminator if not processing nested statements
                if (!m_processingNestedStatements) {
                    currentBlock->isTerminator = true;
                }
            }
            break;
            
        default:
            // Regular statements don't affect control flow
            break;
    }
}

// =============================================================================
// Nested Statement Processing
// =============================================================================

void CFGBuilder::processNestedStatements(const std::vector<StatementPtr>& statements, 
                                         BasicBlock* currentBlock, int defaultLineNumber) {
    // Set flag to indicate we're processing nested statements
    bool wasProcessingNested = m_processingNestedStatements;
    m_processingNestedStatements = true;

    
    for (const auto& nestedStmt : statements) {
        // Get the line number for this nested statement
        // For multi-line IF blocks, nested statements might not have their own line numbers
        // so we use the parent IF's line number as a fallback
        int lineNumber = defaultLineNumber;
        
        // Check if this is a control-flow statement that needs CFG processing
        ASTNodeType type = nestedStmt->getType();
        
        bool isControlFlow = (type == ASTNodeType::STMT_IF ||
                             type == ASTNodeType::STMT_WHILE ||
                             type == ASTNodeType::STMT_FOR ||
                             type == ASTNodeType::STMT_FOR_IN ||
                             type == ASTNodeType::STMT_DO ||
                             type == ASTNodeType::STMT_REPEAT ||
                             type == ASTNodeType::STMT_CASE ||
                             type == ASTNodeType::STMT_TRY_CATCH ||
                             type == ASTNodeType::STMT_WEND ||
                             type == ASTNodeType::STMT_NEXT ||
                             type == ASTNodeType::STMT_LOOP ||
                             type == ASTNodeType::STMT_UNTIL ||
                             type == ASTNodeType::STMT_GOTO ||
                             type == ASTNodeType::STMT_GOSUB ||
                             type == ASTNodeType::STMT_ON_GOTO ||
                             type == ASTNodeType::STMT_ON_GOSUB ||
                             type == ASTNodeType::STMT_LABEL ||
                             type == ASTNodeType::STMT_RETURN ||
                             type == ASTNodeType::STMT_EXIT ||
                             // Note: STMT_END here includes END IF, END SELECT, etc.
                             // We should NOT treat these as program termination END
                             // type == ASTNodeType::STMT_END ||
                             type == ASTNodeType::STMT_THROW ||
                             type == ASTNodeType::STMT_FUNCTION ||
                             type == ASTNodeType::STMT_SUB ||
                             type == ASTNodeType::STMT_DEF);
        
        if (isControlFlow) {
            // Process control-flow statements through the regular processStatement method
            // This ensures they create proper CFG blocks and edges

            processStatement(*nestedStmt, m_currentBlock, lineNumber);
        } else {
            // For non-control-flow statements, just add them to the current block
            // (don't call processStatement to avoid double-adding)

            m_currentBlock->addStatement(nestedStmt.get(), lineNumber);
        }
    }
    
    // Restore flag
    m_processingNestedStatements = wasProcessingNested;
}

} // namespace FasterBASIC