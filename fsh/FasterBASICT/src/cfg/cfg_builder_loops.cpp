//
// cfg_builder_loops.cpp
// FasterBASIC - Control Flow Graph Builder Loop Handlers
//
// Contains FOR, FOR...IN, WHILE, REPEAT, and DO loop processing.
// Part of modular CFG builder split (February 2026).
//

#include "cfg_builder.h"
#include <algorithm>
#include <sstream>
#include <iostream>
#include <functional>

namespace FasterBASIC {

// =============================================================================
// FOR Loop Handler
// =============================================================================

void CFGBuilder::processForStatement(const ForStatement& stmt, BasicBlock* currentBlock) {
    // FOR creates: init block (with FOR statement), check block, body block
    // Exit block is created later by NEXT to ensure proper block ordering
    // Structure: FOR init → check (condition) → body → NEXT (increment) → check
    
    // Init block contains the FOR statement (initialization)
    BasicBlock* initBlock = createNewBlock("FOR Init");
    
    // Create edge from current block to init block (for nested loops)
    // This ensures the outer loop body flows into the inner loop init
    if (currentBlock->id != initBlock->id) {
        addFallthroughEdge(currentBlock->id, initBlock->id);
    }
    
    // Move the FOR statement to the init block (it was already added to currentBlock)
    if (!currentBlock->statements.empty() && currentBlock->statements.back() == &stmt) {
        // Get the line number for this statement
        int lineNum = 0;
        auto it = currentBlock->statementLineNumbers.find(&stmt);
        if (it != currentBlock->statementLineNumbers.end()) {
            lineNum = it->second;
        }
        
        // Remove from current block and add to init
        currentBlock->statements.pop_back();
        currentBlock->statementLineNumbers.erase(&stmt);
        initBlock->addStatement(&stmt, lineNum);
    }
    
    // Check block evaluates the loop condition (var <= end for positive STEP)
    BasicBlock* loopCheck = createNewBlock("FOR Loop Check");
    loopCheck->isLoopHeader = true;  // The check block is the actual loop header
    
    BasicBlock* loopBody = createNewBlock("FOR Loop Body");
    
    // Track loop context - stores check block as header (for NEXT to jump back to)
    // Exit block will be set to -1 initially and created by NEXT
    LoopContext ctx;
    ctx.headerBlock = loopCheck->id;  // NEXT jumps back to check block
    ctx.exitBlock = -1;  // Will be created by NEXT processing
    ctx.variable = stmt.variable;
    m_loopStack.push_back(ctx);
    
    // Store FOR loop structure for buildEdges to use (exit block added later)
    ControlFlowGraph::ForLoopBlocks forBlocks;
    forBlocks.initBlock = initBlock->id;
    forBlocks.checkBlock = loopCheck->id;
    forBlocks.bodyBlock = loopBody->id;
    forBlocks.exitBlock = -1;  // Will be set by NEXT
    forBlocks.variable = stmt.variable;
    m_currentCFG->forLoopStructure[initBlock->id] = forBlocks;
    
    // Keep legacy mapping for backwards compatibility
    m_currentCFG->forLoopHeaders[initBlock->id] = loopCheck->id;
    m_currentCFG->forLoopHeaders[loopCheck->id] = loopBody->id;
    
    // Continue building in the loop body
    m_currentBlock = loopBody;
}

// =============================================================================
// FOR...IN Loop Handler
// =============================================================================

void CFGBuilder::processForInStatement(const ForInStatement& stmt, BasicBlock* currentBlock) {
    // FOR...IN creates loop header similar to FOR
    BasicBlock* loopHeader = createNewBlock("FOR...IN Loop Header");
    loopHeader->isLoopHeader = true;
    
    BasicBlock* loopBody = createNewBlock("FOR...IN Loop Body");
    BasicBlock* loopExit = createNewBlock("After FOR...IN");
    loopExit->isLoopExit = true;
    
    // Track loop context
    LoopContext ctx;
    ctx.headerBlock = loopHeader->id;
    ctx.exitBlock = loopExit->id;
    ctx.variable = stmt.variable;
    m_loopStack.push_back(ctx);
    
    // Remember this FOR...IN loop
    m_currentCFG->forLoopHeaders[loopHeader->id] = loopHeader->id;
    
    m_currentBlock = loopBody;
}

// =============================================================================
// WHILE Loop Handler
// =============================================================================

void CFGBuilder::processWhileStatement(const WhileStatement& stmt, BasicBlock* currentBlock) {
    // WHILE creates loop header with condition
    BasicBlock* loopHeader = createNewBlock("WHILE Loop Header");
    loopHeader->isLoopHeader = true;
    
    // Add the WHILE statement to the header block (it was already added to currentBlock)
    // We need to move it to the header block
    if (!currentBlock->statements.empty() && currentBlock->statements.back() == &stmt) {
        // Get the line number for this statement
        int lineNum = 0;
        auto it = currentBlock->statementLineNumbers.find(&stmt);
        if (it != currentBlock->statementLineNumbers.end()) {
            lineNum = it->second;
        }
        
        // Remove from current block and add to header
        currentBlock->statements.pop_back();
        currentBlock->statementLineNumbers.erase(&stmt);
        loopHeader->addStatement(&stmt, lineNum);
    }
    
    BasicBlock* loopBody = createNewBlock("WHILE Loop Body");
    
    // Track loop context
    LoopContext ctx;
    ctx.headerBlock = loopHeader->id;
    ctx.exitBlock = -1;  // Will be set when we encounter WEND
    m_loopStack.push_back(ctx);
    
    m_currentCFG->whileLoopHeaders[loopHeader->id] = loopHeader->id;
    
    m_currentBlock = loopBody;
}

// =============================================================================
// REPEAT Loop Handler
// =============================================================================

void CFGBuilder::processRepeatStatement(const RepeatStatement& stmt, BasicBlock* currentBlock) {
    // REPEAT creates loop body
    BasicBlock* loopBody = createNewBlock("REPEAT Loop Body");
    loopBody->isLoopHeader = true;
    
    BasicBlock* loopExit = createNewBlock("After REPEAT");
    loopExit->isLoopExit = true;
    
    // Track loop context
    LoopContext ctx;
    ctx.headerBlock = loopBody->id;
    ctx.exitBlock = loopExit->id;
    m_loopStack.push_back(ctx);
    
    m_currentCFG->repeatLoopHeaders[loopBody->id] = loopBody->id;
    
    m_currentBlock = loopBody;
}

// =============================================================================
// DO Loop Handler
// =============================================================================

void CFGBuilder::processDoStatement(const DoStatement& stmt, BasicBlock* currentBlock) {
    // DO creates loop structure - behavior depends on condition type
    BasicBlock* loopHeader = createNewBlock("DO Loop Header");
    loopHeader->isLoopHeader = true;
    
    // Add the DO statement to the header block (it was already added to currentBlock)
    // We need to move it to the header block
    if (!currentBlock->statements.empty() && currentBlock->statements.back() == &stmt) {
        // Get the line number for this statement
        int lineNum = 0;
        auto it = currentBlock->statementLineNumbers.find(&stmt);
        if (it != currentBlock->statementLineNumbers.end()) {
            lineNum = it->second;
        }
        
        // Remove from current block and add to header
        currentBlock->statements.pop_back();
        currentBlock->statementLineNumbers.erase(&stmt);
        loopHeader->addStatement(&stmt, lineNum);
    }
    
    BasicBlock* loopBody = createNewBlock("DO Loop Body");
    BasicBlock* loopExit = createNewBlock("After DO");
    loopExit->isLoopExit = true;
    
    // Track loop context
    LoopContext ctx;
    ctx.headerBlock = loopHeader->id;
    ctx.exitBlock = loopExit->id;
    m_loopStack.push_back(ctx);
    
    m_currentCFG->doLoopHeaders[loopHeader->id] = loopHeader->id;
    
    // Store DO loop structure (similar to FOR loops)
    ControlFlowGraph::DoLoopBlocks doBlocks;
    doBlocks.headerBlock = loopHeader->id;
    doBlocks.bodyBlock = loopBody->id;
    doBlocks.exitBlock = loopExit->id;
    m_currentCFG->doLoopStructure[loopHeader->id] = doBlocks;
    
    m_currentBlock = loopBody;
}

} // namespace FasterBASIC