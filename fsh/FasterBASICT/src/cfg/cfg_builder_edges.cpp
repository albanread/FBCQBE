//
// cfg_builder_edges.cpp
// FasterBASIC - Control Flow Graph Builder Edge Construction
//
// Contains Phase 2 (edge building), Phase 3 (loop analysis), Phase 4 (subroutine analysis),
// and Phase 5 (optimization).
// Part of modular CFG builder split (February 2026).
//

#include "cfg_builder.h"
#include <algorithm>
#include <sstream>
#include <iostream>
#include <functional>

namespace FasterBASIC {

// =============================================================================
// Edges, Loop Analysis, Subroutines, Optimization
// =============================================================================

// =============================================================================
// Phase 2: Build Control Flow Edges
// =============================================================================

void CFGBuilder::buildEdges() {
    // Walk through blocks and create edges based on statements
    for (const auto& block : m_currentCFG->blocks) {
        // Check if this is a FOR loop init block
        auto forStructIt = m_currentCFG->forLoopStructure.find(block->id);
        if (forStructIt != m_currentCFG->forLoopStructure.end()) {
            const auto& forBlocks = forStructIt->second;
            
            // FOR init block: unconditional jump to check block
            addUnconditionalEdge(block->id, forBlocks.checkBlock);
            
            // Also need to ensure predecessor blocks connect to this init block
            // This handles nested FOR loops where the outer body should flow to inner init
            if (block->id > 0) {
                const auto& prevBlock = m_currentCFG->blocks[block->id - 1];
                // If previous block doesn't already have this block as a successor, add fallthrough
                if (std::find(prevBlock->successors.begin(), prevBlock->successors.end(), block->id) == prevBlock->successors.end()) {
                    // Check if previous block is a body block that should flow here
                    bool isPreviousBodyBlock = false;
                    for (const auto& pair : m_currentCFG->forLoopStructure) {
                        if (pair.second.bodyBlock == prevBlock->id) {
                            isPreviousBodyBlock = true;
                            break;
                        }
                    }
                    
                    // If it's not already handled and is not a terminator block, add fallthrough
                    if (!prevBlock->isTerminator && !prevBlock->statements.empty()) {
                        const Statement* lastStmt = prevBlock->statements.back();
                        ASTNodeType lastType = lastStmt->getType();
                        // If the last statement isn't a control flow statement, add fallthrough
                        if (lastType != ASTNodeType::STMT_GOTO && 
                            lastType != ASTNodeType::STMT_RETURN &&
                            lastType != ASTNodeType::STMT_END &&
                            lastType != ASTNodeType::STMT_EXIT &&
                            lastType != ASTNodeType::STMT_NEXT &&
                            lastType != ASTNodeType::STMT_WEND &&
                            lastType != ASTNodeType::STMT_LOOP &&
                            lastType != ASTNodeType::STMT_UNTIL) {
                            addFallthroughEdge(prevBlock->id, block->id);
                        }
                    }
                }
            }
            
            continue;  // Skip regular processing for FOR init blocks
        }
        
        // Check if this is a FOR loop check block
        bool isForCheckBlock = false;
        for (const auto& pair : m_currentCFG->forLoopStructure) {
            if (pair.second.checkBlock == block->id) {
                isForCheckBlock = true;
                const auto& forBlocks = pair.second;
                
                // FOR check block: conditional branch
                // True condition: go to body
                addConditionalEdge(block->id, forBlocks.bodyBlock, "true");
                // False condition: go to exit (should be set by NEXT processing)
                if (forBlocks.exitBlock >= 0) {
                    addConditionalEdge(block->id, forBlocks.exitBlock, "false");
                }
                break;
            }
        }
        if (isForCheckBlock) {
            continue;  // Skip regular processing for FOR check blocks
        }
        
        // Check if this is a SELECT CASE test block (empty but needs special handling)
        bool isSelectCaseTestBlock = false;
        for (const auto& ctx : m_selectCaseStack) {
            for (size_t i = 0; i < ctx.testBlocks.size(); i++) {
                if (block->id == ctx.testBlocks[i]) {
                    isSelectCaseTestBlock = true;
                    // Test block: conditional branch to body or next test/else/exit
                    // True: jump to body
                    addConditionalEdge(block->id, ctx.bodyBlocks[i], "true");
                    
                    // False: jump to next test, else, or exit
                    if (i + 1 < ctx.testBlocks.size()) {
                        // Next test block
                        addConditionalEdge(block->id, ctx.testBlocks[i + 1], "false");
                    } else if (ctx.elseBlock >= 0) {
                        // ELSE block
                        addConditionalEdge(block->id, ctx.elseBlock, "false");
                    } else {
                        // Exit (no match)
                        addConditionalEdge(block->id, ctx.exitBlock, "false");
                    }
                    break;
                }
            }
            if (isSelectCaseTestBlock) break;
        }
        
        if (isSelectCaseTestBlock) {
            continue;  // Skip regular processing for test blocks
        }
        
        // Check if this is a TRY/CATCH structure block (dispatch, catch, finally)
        bool isTryCatchBlock = false;
        for (const auto& ctx : m_tryCatchStack) {
            // Check if this is the dispatch block
            if (block->id == ctx.dispatchBlock) {
                isTryCatchBlock = true;
                // Dispatch block: conditional branches to each CATCH block based on error code
                // First, check each CATCH clause in order
                for (size_t i = 0; i < ctx.catchBlocks.size(); i++) {
                    addConditionalEdge(block->id, ctx.catchBlocks[i], "error matches");
                }
                // If no CATCH matches, re-throw (goes to outer handler or terminates)
                // We don't add an explicit edge here as it's handled by runtime
                break;
            }
            
            // Check if this is a TRY body block
            if (block->id == ctx.tryBodyBlock) {
                isTryCatchBlock = true;
                // TRY body on normal completion: jump to FINALLY or exit
                if (ctx.hasFinally) {
                    addFallthroughEdge(block->id, ctx.finallyBlock);
                } else {
                    addFallthroughEdge(block->id, ctx.exitBlock);
                }
                // Note: Exception dispatch is reached via longjmp, not normal CFG flow
                break;
            }
            
            // Check if this is a CATCH block
            for (size_t i = 0; i < ctx.catchBlocks.size(); i++) {
                if (block->id == ctx.catchBlocks[i]) {
                    isTryCatchBlock = true;
                    // CATCH block on completion: jump to FINALLY or exit
                    if (ctx.hasFinally) {
                        addFallthroughEdge(block->id, ctx.finallyBlock);
                    } else {
                        addFallthroughEdge(block->id, ctx.exitBlock);
                    }
                    break;
                }
            }
            if (isTryCatchBlock) break;
            
            // Check if this is a FINALLY block
            if (ctx.hasFinally && block->id == ctx.finallyBlock) {
                isTryCatchBlock = true;
                // FINALLY always jumps to exit
                addFallthroughEdge(block->id, ctx.exitBlock);
                break;
            }
        }
        
        if (isTryCatchBlock) {
            continue;  // Skip regular processing for TRY/CATCH blocks
        }
        
        if (block->statements.empty()) {
            // Empty block - fallthrough to next only if no explicit successors
            if (block->successors.empty() && 
                block->id + 1 < static_cast<int>(m_currentCFG->blocks.size())) {
                addFallthroughEdge(block->id, block->id + 1);
            }
            continue;
        }
        
        // Check last statement in block for control flow
        const Statement* lastStmt = block->statements.back();
        ASTNodeType type = lastStmt->getType();
        
        switch (type) {
            case ASTNodeType::STMT_GOTO: {
                // Unconditional jump to target line (or next available line)
                const auto& gotoStmt = static_cast<const GotoStatement&>(*lastStmt);
                int targetBlock = m_currentCFG->getBlockForLineOrNext(gotoStmt.lineNumber);
                if (targetBlock >= 0) {
                    addUnconditionalEdge(block->id, targetBlock);
                }
                break;
            }
            
            case ASTNodeType::STMT_GOSUB: {
                // Call to subroutine (or next available line), then continue
                const auto& gosubStmt = static_cast<const GosubStatement&>(*lastStmt);
                int targetBlock = m_currentCFG->getBlockForLineOrNext(gosubStmt.lineNumber);
                if (targetBlock >= 0) {
                    addCallEdge(block->id, targetBlock);
                }
                // Continue to the return block that was created by processGosubStatement
                // Use the mapping instead of assuming block->id + 1
                auto it = m_gosubReturnMap.find(block->id);
                if (it != m_gosubReturnMap.end()) {
                    // Found the recorded return block
                    addFallthroughEdge(block->id, it->second);
                } else {
                    // Fallback to old behavior (shouldn't happen with proper processing)
                    if (block->id + 1 < static_cast<int>(m_currentCFG->blocks.size())) {
                        addFallthroughEdge(block->id, block->id + 1);
                    }
                }
                break;
            }
            
            case ASTNodeType::STMT_ON_GOTO: {
                // Multiple potential jump targets based on selector expression
                const auto& onGotoStmt = static_cast<const OnGotoStatement&>(*lastStmt);
                
                // Add edges to all possible targets
                for (size_t i = 0; i < onGotoStmt.isLabelList.size(); ++i) {
                    int targetBlock = -1;
                    if (onGotoStmt.isLabelList[i]) {
                        // Symbolic label
                        if (m_symbols) {
                            auto it = m_symbols->labels.find(onGotoStmt.labels[i]);
                            if (it != m_symbols->labels.end()) {
                                int labelLine = it->second.programLineIndex;
                                if (labelLine >= 0) {
                                    targetBlock = m_currentCFG->getBlockForLine(labelLine);
                                }
                            }
                        }
                    } else {
                        // Line number
                        targetBlock = m_currentCFG->getBlockForLine(onGotoStmt.lineNumbers[i]);
                    }
                    
                    if (targetBlock >= 0) {
                        // Add conditional edge (selector == i+1)
                        addConditionalEdge(block->id, targetBlock, std::to_string(i + 1));
                    }
                }
                
                // If selector is out of range, continue to next block
                if (block->id + 1 < static_cast<int>(m_currentCFG->blocks.size())) {
                    addConditionalEdge(block->id, block->id + 1, "default");
                }
                break;
            }
            
            case ASTNodeType::STMT_ON_GOSUB: {
                // Multiple potential subroutine calls based on selector expression
                const auto& onGosubStmt = static_cast<const OnGosubStatement&>(*lastStmt);
                
                // Add call edges to all possible targets
                for (size_t i = 0; i < onGosubStmt.isLabelList.size(); ++i) {
                    int targetBlock = -1;
                    if (onGosubStmt.isLabelList[i]) {
                        // Symbolic label
                        if (m_symbols) {
                            auto it = m_symbols->labels.find(onGosubStmt.labels[i]);
                            if (it != m_symbols->labels.end()) {
                                int labelLine = it->second.programLineIndex;
                                if (labelLine >= 0) {
                                    targetBlock = m_currentCFG->getBlockForLine(labelLine);
                                }
                            }
                        }
                    } else {
                        // Line number
                        targetBlock = m_currentCFG->getBlockForLine(onGosubStmt.lineNumbers[i]);
                    }
                    
                    if (targetBlock >= 0) {
                        // Add call edge (selector == i+1)
                        addCallEdge(block->id, targetBlock);
                    }
                }
                
                // If selector is out of range, continue to next block
                if (block->id + 1 < static_cast<int>(m_currentCFG->blocks.size())) {
                    addFallthroughEdge(block->id, block->id + 1);
                }
                break;
            }
            
            case ASTNodeType::STMT_IF: {
                // Conditional branch
                const auto& ifStmt = static_cast<const IfStatement&>(*lastStmt);
                if (ifStmt.hasGoto) {
                    // Branch to line (or next available line) or continue
                    int targetBlock = m_currentCFG->getBlockForLineOrNext(ifStmt.gotoLine);
                    if (targetBlock >= 0) {
                        addConditionalEdge(block->id, targetBlock, "true");
                    }
                    if (block->id + 1 < static_cast<int>(m_currentCFG->blocks.size())) {
                        addConditionalEdge(block->id, block->id + 1, "false");
                    }
                } else if (ifStmt.isMultiLine) {
                    // Multi-line IF...END IF
                    // The successors were already set up in processIfStatement
                    // Nothing to do here - edges are already correct
                }
                break;
            }
            
            case ASTNodeType::STMT_WHILE: {
                // WHILE header with condition - needs conditional edges
                // Body should be the next block, exit will be found later
                auto it = m_currentCFG->whileLoopHeaders.find(block->id);
                if (it != m_currentCFG->whileLoopHeaders.end()) {
                    // This is a WHILE header
                    // True condition: go to body (next block)
                    if (block->id + 1 < static_cast<int>(m_currentCFG->blocks.size())) {
                        addConditionalEdge(block->id, block->id + 1, "true");
                    }
                    
                    // False condition: We need to find the exit block
                    // The exit block should be right after the matching WEND
                    // We need to count nesting levels to find the matching WEND
                    // IMPORTANT: We must look at ALL blocks, including empty ones, and
                    // check statements in the order they appear in blocks
                    int nestingLevel = 0;
                    bool foundWend = false;
                    for (size_t i = block->id + 1; i < m_currentCFG->blocks.size(); i++) {
                        const auto& futureBlock = m_currentCFG->blocks[i];
                        
                        // Check all statements in this block
                        for (const Statement* stmt : futureBlock->statements) {
                            if (stmt->getType() == ASTNodeType::STMT_WHILE) {
                                nestingLevel++;
                            } else if (stmt->getType() == ASTNodeType::STMT_WEND) {
                                if (nestingLevel == 0) {
                                    // Found the matching WEND
                                    // The WEND statement is in block i
                                    // After a WEND, the next block should be "After WHILE"
                                    // which is the exit block for this loop
                                    if (i + 1 < m_currentCFG->blocks.size()) {
                                        addConditionalEdge(block->id, i + 1, "false");
                                        foundWend = true;
                                    }
                                    goto found_wend;
                                }
                                nestingLevel--;
                            }
                        }
                    }
                    found_wend:
                    // If we didn't find a matching WEND, something is wrong
                    if (!foundWend && block->id + 1 < static_cast<int>(m_currentCFG->blocks.size())) {
                        // Fallback: exit to next block (shouldn't happen in well-formed code)
                        addConditionalEdge(block->id, block->id + 1, "false");
                    }
                } else {
                    // Fallthrough if not a loop header
                    if (block->id + 1 < static_cast<int>(m_currentCFG->blocks.size())) {
                        addFallthroughEdge(block->id, block->id + 1);
                    }
                }
                break;
            }
            
            case ASTNodeType::STMT_DO: {
                // DO header - needs conditional or unconditional edges based on condition type
                auto it = m_currentCFG->doLoopHeaders.find(block->id);
                if (it != m_currentCFG->doLoopHeaders.end()) {
                    // This is a DO header
                    const auto& doStmt = static_cast<const DoStatement&>(*lastStmt);
                    
                    if (doStmt.conditionType == DoStatement::ConditionType::WHILE ||
                        doStmt.conditionType == DoStatement::ConditionType::UNTIL) {
                        // Pre-test loop: conditional edges
                        // True condition: go to body (next block)
                        if (block->id + 1 < static_cast<int>(m_currentCFG->blocks.size())) {
                            addConditionalEdge(block->id, block->id + 1, "true");
                        }
                        
                        // False condition: find the exit block after matching LOOP
                        int nestingLevel = 0;
                        for (size_t i = block->id + 1; i < m_currentCFG->blocks.size(); i++) {
                            const auto& futureBlock = m_currentCFG->blocks[i];
                            if (!futureBlock->statements.empty()) {
                                for (const Statement* stmt : futureBlock->statements) {
                                    if (stmt->getType() == ASTNodeType::STMT_DO) {
                                        nestingLevel++;
                                    } else if (stmt->getType() == ASTNodeType::STMT_LOOP) {
                                        if (nestingLevel == 0) {
                                            // Found the matching LOOP
                                            // Exit is the block after LOOP
                                            if (i + 1 < m_currentCFG->blocks.size()) {
                                                addConditionalEdge(block->id, i + 1, "false");
                                            }
                                            goto found_loop;
                                        }
                                        nestingLevel--;
                                    }
                                }
                            }
                        }
                        found_loop:;
                    } else {
                        // Plain DO - unconditional jump to body (next block)
                        if (block->id + 1 < static_cast<int>(m_currentCFG->blocks.size())) {
                            addUnconditionalEdge(block->id, block->id + 1);
                        }
                    }
                } else {
                    // Fallthrough if not a loop header
                    if (block->id + 1 < static_cast<int>(m_currentCFG->blocks.size())) {
                        addFallthroughEdge(block->id, block->id + 1);
                    }
                }
                break;
            }
            
            case ASTNodeType::STMT_UNTIL: {
                // UNTIL is the end of a REPEAT loop
                // Need to create:
                // 1. Conditional edge to exit block (next block) when condition is TRUE
                // 2. Conditional edge back to loop header (REPEAT block) when condition is FALSE
                
                // Find the matching REPEAT by looking for loop context with this block in its range
                LoopContext* loopCtx = nullptr;
                for (auto& ctx : m_loopStack) {
                    // The UNTIL block can be the same as or after the header block
                    if (block->id >= ctx.headerBlock) {
                        loopCtx = &ctx;
                        break;
                    }
                }
                
                if (loopCtx) {
                    // When condition is TRUE, exit loop (go to next block)
                    if (block->id + 1 < static_cast<int>(m_currentCFG->blocks.size())) {
                        addConditionalEdge(block->id, block->id + 1, "true");
                    }
                    // When condition is FALSE, repeat (go back to loop header)
                    addConditionalEdge(block->id, loopCtx->headerBlock, "false");
                    
                    // Pop this loop context now that we've handled the UNTIL
                    m_loopStack.erase(std::remove_if(m_loopStack.begin(), m_loopStack.end(),
                        [loopCtx](const LoopContext& ctx) { return &ctx == loopCtx; }), 
                        m_loopStack.end());
                } else {
                    // UNTIL without REPEAT - fallthrough
                    if (block->id + 1 < static_cast<int>(m_currentCFG->blocks.size())) {
                        addFallthroughEdge(block->id, block->id + 1);
                    }
                }
                break;
            }
            
            case ASTNodeType::STMT_LOOP: {
                // LOOP is the end of a DO loop
                // Need to create:
                // 1. Conditional edge to exit block (next block) based on condition
                // 2. Conditional or unconditional edge back to loop header
                
                // Find the matching DO by looking for loop context
                LoopContext* loopCtx = nullptr;
                for (auto& ctx : m_loopStack) {
                    // The LOOP block can be the same as or after the header block
                    if (block->id >= ctx.headerBlock) {
                        loopCtx = &ctx;
                        break;
                    }
                }
                
                if (loopCtx) {
                    // Get the LOOP statement to check condition type
                    const auto& loopStmt = static_cast<const LoopStatement&>(*lastStmt);
                    
                    if (loopStmt.conditionType == LoopStatement::ConditionType::NONE) {
                        // Plain LOOP - unconditional back edge
                        addUnconditionalEdge(block->id, loopCtx->headerBlock);
                    } else {
                        // LOOP WHILE/UNTIL - conditional edges
                        // When condition is TRUE, exit loop (go to next block)
                        if (block->id + 1 < static_cast<int>(m_currentCFG->blocks.size())) {
                            addConditionalEdge(block->id, block->id + 1, "true");
                        }
                        // When condition is FALSE, repeat (go back to loop header)
                        addConditionalEdge(block->id, loopCtx->headerBlock, "false");
                    }
                    
                    // Pop this loop context
                    m_loopStack.erase(std::remove_if(m_loopStack.begin(), m_loopStack.end(),
                        [loopCtx](const LoopContext& ctx) { return &ctx == loopCtx; }), 
                        m_loopStack.end());
                } else {
                    // LOOP without DO - fallthrough
                    if (block->id + 1 < static_cast<int>(m_currentCFG->blocks.size())) {
                        addFallthroughEdge(block->id, block->id + 1);
                    }
                }
                break;
            }
            
            case ASTNodeType::STMT_WEND: {
                // WEND is the end of a WHILE loop
                // Need to create:
                // 1. Unconditional back edge to loop header (WHILE condition block)
                
                // Find the matching WHILE by looking for loop context
                // Search backwards to find the innermost (most recent) loop
                // IMPORTANT: We need to find the loop that this WEND actually closes
                LoopContext* loopCtx = nullptr;
                int loopCtxIndex = -1;
                
                for (int idx = m_loopStack.size() - 1; idx >= 0; --idx) {
                    // The WEND block should be after the header block
                    if (block->id > m_loopStack[idx].headerBlock) {
                        loopCtx = &m_loopStack[idx];
                        loopCtxIndex = idx;
                        break;
                    }
                }
                
                if (loopCtx) {
                    // Unconditional back edge to WHILE header (condition check)
                    addUnconditionalEdge(block->id, loopCtx->headerBlock);
                    
                    // Pop this loop context
                    // Use index-based erase to avoid iterator invalidation
                    if (loopCtxIndex >= 0 && loopCtxIndex < static_cast<int>(m_loopStack.size())) {
                        m_loopStack.erase(m_loopStack.begin() + loopCtxIndex);
                    }
                } else {
                    // WEND without WHILE - fallthrough
                    if (block->id + 1 < static_cast<int>(m_currentCFG->blocks.size())) {
                        addFallthroughEdge(block->id, block->id + 1);
                    }
                }
                break;
            }
            
            case ASTNodeType::STMT_NEXT: {
                // NEXT is the end of a FOR loop
                // Create unconditional back edge to loop check block
                // Use the mapping recorded in processStatement
                
                auto it = m_nextToHeaderMap.find(block->id);
                if (it != m_nextToHeaderMap.end()) {
                    // Found the target header block - create back edge
                    addUnconditionalEdge(block->id, it->second);
                } else {
                    // NEXT without matching FOR - fallthrough
                    if (block->id + 1 < static_cast<int>(m_currentCFG->blocks.size())) {
                        addFallthroughEdge(block->id, block->id + 1);
                    }
                }
                break;
            }
            
            case ASTNodeType::STMT_CASE: {
                // SELECT CASE - multi-way branch
                // Find the matching SELECT CASE context
                SelectCaseContext* caseCtx = nullptr;
                for (auto& ctx : m_selectCaseStack) {
                    if (ctx.selectBlock == block->id) {
                        caseCtx = &ctx;
                        break;
                    }
                }
                
                if (caseCtx && !caseCtx->testBlocks.empty()) {
                    // From SELECT block, jump to first test block
                    addUnconditionalEdge(block->id, caseCtx->testBlocks[0]);
                }
                break;
            }
            
            case ASTNodeType::STMT_TRY_CATCH: {
                // TRY/CATCH - exception handling structure
                // Find the matching TRY/CATCH context
                TryCatchContext* tryCtx = nullptr;
                for (auto& ctx : m_tryCatchStack) {
                    if (ctx.tryBlock == block->id) {
                        tryCtx = &ctx;
                        break;
                    }
                }
                
                if (tryCtx) {
                    // From TRY setup block, conditional jump based on setjmp result
                    // If setjmp returns non-zero (exception), jump to dispatch
                    // If setjmp returns zero (normal), fall through to TRY body
                    addConditionalEdge(block->id, tryCtx->dispatchBlock, "exception");
                    addConditionalEdge(block->id, tryCtx->tryBodyBlock, "normal");
                }
                break;
            }
            
            case ASTNodeType::STMT_THROW:
                // THROW - terminates normal flow, jumps to exception handler
                // The actual exception routing is handled by setjmp/longjmp at runtime
                // In CFG, we mark this as terminator (already done in processStatement)
                break;
            
            case ASTNodeType::STMT_RETURN:
            case ASTNodeType::STMT_END:
                // Terminators - no outgoing edges (or return edge)
                if (m_currentCFG->exitBlock >= 0) {
                    addReturnEdge(block->id, m_currentCFG->exitBlock);
                }
                break;
                
            case ASTNodeType::STMT_EXIT:
                {
                    // EXIT statement handling
                    // For EXIT FOR, edges are already added by NEXT processing (pendingExitBlocks)
                    // For EXIT FUNCTION/SUB, add return edge to function exit
                    const ExitStatement* exitStmt = nullptr;
                    for (const Statement* stmt : block->statements) {
                        if (stmt->getType() == ASTNodeType::STMT_EXIT) {
                            exitStmt = static_cast<const ExitStatement*>(stmt);
                            break;
                        }
                    }
                    
                    if (exitStmt && 
                        (exitStmt->exitType == ExitStatement::ExitType::FUNCTION ||
                         exitStmt->exitType == ExitStatement::ExitType::SUB)) {
                        // EXIT FUNCTION/SUB - jump to function exit
                        if (m_currentCFG->exitBlock >= 0) {
                            addReturnEdge(block->id, m_currentCFG->exitBlock);
                        }
                    }
                    // EXIT FOR edges are already handled by pendingExitBlocks mechanism
                }
                break;
                
            default:
                // Check if this block is part of a SELECT CASE structure
                bool handledBySelectCase = false;
                
                for (const auto& ctx : m_selectCaseStack) {
                    // Check if this is a body block
                    for (size_t i = 0; i < ctx.bodyBlocks.size(); i++) {
                        if (block->id == ctx.bodyBlocks[i]) {
                            // Body block: jump to exit after executing
                            addUnconditionalEdge(block->id, ctx.exitBlock);
                            handledBySelectCase = true;
                            break;
                        }
                    }
                    
                    // Check if this is the else block
                    if (ctx.elseBlock >= 0 && block->id == ctx.elseBlock) {
                        // Else block: jump to exit after executing
                        addUnconditionalEdge(block->id, ctx.exitBlock);
                        handledBySelectCase = true;
                    }
                    
                    if (handledBySelectCase) break;
                }
                
                if (!handledBySelectCase) {
                    // Regular statement - fallthrough to next block
                    // Only add fallthrough if block doesn't already have explicit successors
                    if (block->successors.empty() && 
                        block->id + 1 < static_cast<int>(m_currentCFG->blocks.size())) {
                        addFallthroughEdge(block->id, block->id + 1);
                    }
                }
                break;
        }
    }
}

// =============================================================================
// Phase 3: Identify Loop Structures
// =============================================================================

void CFGBuilder::identifyLoops() {
    // Implement back-edge detection to identify GOTO-based loops
    // A back edge is an edge from block A to block B where B dominates A
    // or in simpler terms, B appears earlier in program order
    
    // For each edge, check if it's a back edge (target block has lower ID than source)
    for (const auto& edge : m_currentCFG->edges) {
        if (edge.type == EdgeType::UNCONDITIONAL && 
            edge.targetBlock < edge.sourceBlock) {
            // This is likely a back edge (GOTO to earlier line)
            BasicBlock* targetBlock = m_currentCFG->getBlock(edge.targetBlock);
            BasicBlock* sourceBlock = m_currentCFG->getBlock(edge.sourceBlock);
            
            if (targetBlock && sourceBlock) {
                // Mark the target as a loop header
                targetBlock->isLoopHeader = true;
                
                // Mark blocks in the loop body between target and source
                for (int blockId = edge.targetBlock; blockId <= edge.sourceBlock; blockId++) {
                    BasicBlock* loopBlock = m_currentCFG->getBlock(blockId);
                    if (loopBlock) {
                        // This block is part of a potential loop
                        // We'll use this information during code generation
                    }
                }
            }
        }
    }
    
    // Also detect cycles using simple DFS
    std::set<int> visited;
    std::set<int> recursionStack;
    
    std::function<void(int)> detectCycles = [&](int blockId) {
        if (recursionStack.count(blockId)) {
            // Found a cycle - mark the target block as a loop header
            BasicBlock* loopHeader = m_currentCFG->getBlock(blockId);
            if (loopHeader) {
                loopHeader->isLoopHeader = true;
            }
            return;
        }
        
        if (visited.count(blockId)) {
            return;
        }
        
        visited.insert(blockId);
        recursionStack.insert(blockId);
        
        BasicBlock* block = m_currentCFG->getBlock(blockId);
        if (block) {
            for (int successor : block->successors) {
                detectCycles(successor);
            }
        }
        
        recursionStack.erase(blockId);
    };
    
    // Start cycle detection from entry block
    if (m_currentCFG->entryBlock >= 0) {
        detectCycles(m_currentCFG->entryBlock);
    }
    
    // Populate CFG's selectCaseInfo map so codegen can look up which CaseStatement each test block belongs to
    for (const auto& ctx : m_selectCaseStack) {
        ControlFlowGraph::SelectCaseInfo info;
        info.selectBlock = ctx.selectBlock;
        info.testBlocks = ctx.testBlocks;
        info.bodyBlocks = ctx.bodyBlocks;
        info.elseBlock = ctx.elseBlock;
        info.exitBlock = ctx.exitBlock;
        info.caseStatement = ctx.caseStatement;
        
        // Map each test block to this SelectCaseInfo
        for (int testBlockId : ctx.testBlocks) {
            m_currentCFG->selectCaseInfo[testBlockId] = info;
        }
    }
}

// =============================================================================
// Phase 4: Identify Subroutines
// =============================================================================

void CFGBuilder::identifySubroutines() {
    // Mark blocks that are GOSUB targets as subroutines
    for (const auto& edge : m_currentCFG->edges) {
        if (edge.type == EdgeType::CALL) {
            BasicBlock* target = m_currentCFG->getBlock(edge.targetBlock);
            if (target) {
                target->isSubroutine = true;
            }
        }
    }
}

// =============================================================================
// Phase 5: Optimize CFG
// =============================================================================

void CFGBuilder::optimizeCFG() {
    // Potential optimizations:
    // - Merge sequential blocks with single predecessor/successor
    // - Remove empty blocks
    // - Simplify edges
    // Not implemented yet
}

} // namespace FasterBASIC
