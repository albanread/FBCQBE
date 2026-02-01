//
// cfg_builder_jumps.cpp
// FasterBASIC - Control Flow Graph Builder Jump Handlers
//
// Contains GOTO, GOSUB, RETURN, ON GOTO, ON GOSUB, and label processing.
// Part of modular CFG builder split (February 2026).
//

#include "cfg_builder.h"
#include <algorithm>
#include <sstream>
#include <iostream>
#include <functional>

namespace FasterBASIC {

// =============================================================================
// Jump Statement Handlers
// =============================================================================

void CFGBuilder::processGotoStatement(const GotoStatement& stmt, BasicBlock* currentBlock) {
    // GOTO creates unconditional jump - start new block after this
    BasicBlock* nextBlock = createNewBlock();
    m_currentBlock = nextBlock;
    
    // Edge will be added in buildEdges phase when we know target block IDs
}

void CFGBuilder::processGosubStatement(const GosubStatement& stmt, BasicBlock* currentBlock) {
    // GOSUB is like a call - execution continues after it in a new block
    // Create a new block for the return point (statement after GOSUB)
    BasicBlock* nextBlock = createNewBlock();
    
    // Record the mapping from GOSUB block to its return continuation block
    // This is needed because blocks may not be sequential when GOSUB is inside IF/WHILE
    m_gosubReturnMap[currentBlock->id] = nextBlock->id;
    
    // Track this block as a GOSUB return point for optimization
    // This allows RETURN statements to only check reachable return blocks
    if (m_currentCFG) {
        m_currentCFG->gosubReturnBlocks.insert(nextBlock->id);
    }
    
    m_currentBlock = nextBlock;
    
    // Edge will be added in buildEdges phase when we know target block IDs
}

void CFGBuilder::processOnGotoStatement(const OnGotoStatement& stmt, BasicBlock* currentBlock) {
    // ON GOTO creates multiple potential jump targets - like GOTO, it's a terminator
    // If selector is out of range, execution continues to next statement
    BasicBlock* nextBlock = createNewBlock();
    m_currentBlock = nextBlock;
    
    // Edges will be added in buildEdges phase when we know target block IDs
}

void CFGBuilder::processOnGosubStatement(const OnGosubStatement& stmt, BasicBlock* currentBlock) {
    // ON GOSUB creates multiple potential subroutine calls - like GOSUB, execution can continue
    // If selector is out of range, execution continues to next statement
    // Since it's a terminator, start new block for next statement
    BasicBlock* nextBlock = createNewBlock();
    m_currentBlock = nextBlock;
    // Edges will be added in buildEdges phase
}

void CFGBuilder::processLabelStatement(const LabelStatement& stmt, BasicBlock* currentBlock) {
    // Labels are jump targets - start a new block for the label
    BasicBlock* labelBlock = createNewBlock("Label_" + stmt.labelName);
    m_currentBlock = labelBlock;
}

} // namespace FasterBASIC