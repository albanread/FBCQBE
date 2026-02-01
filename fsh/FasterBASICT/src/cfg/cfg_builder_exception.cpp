//
// cfg_builder_exception.cpp
// FasterBASIC - Control Flow Graph Builder Exception Handlers
//
// Contains TRY/CATCH/FINALLY processing.
// Part of modular CFG builder split (February 2026).
//

#include "cfg_builder.h"
#include <algorithm>
#include <sstream>
#include <iostream>
#include <functional>

namespace FasterBASIC {

// =============================================================================
// TRY/CATCH/FINALLY Handler
// =============================================================================

void CFGBuilder::processTryCatchStatement(const TryCatchStatement& stmt, BasicBlock* currentBlock) {
    // TRY/CATCH/FINALLY creates an exception handling structure
    // Structure:
    //   - TRY block (current): Sets up exception context (setjmp)
    //   - TRY body block: Executes TRY statements
    //   - Dispatch block: Checks error code and routes to appropriate CATCH
    //   - CATCH blocks: One per CATCH clause
    //   - FINALLY block: Optional, executes cleanup code
    //   - Exit block: Continue after END TRY
    
    // The TRY statement (setup) stays in current block
    int trySetupBlockId = currentBlock->id;
    
    // Create TRY body block
    BasicBlock* tryBodyBlock = createNewBlock("TRY Body");
    int tryBodyBlockId = tryBodyBlock->id;
    
    // Process TRY block statements
    m_currentBlock = tryBodyBlock;
    for (const auto& tryStmt : stmt.tryBlock) {
        if (tryStmt) {
            processStatement(*tryStmt, m_currentBlock, 0);
        }
    }
    
    // Create exception dispatch block
    BasicBlock* dispatchBlock = createNewBlock("Exception Dispatch");
    int dispatchBlockId = dispatchBlock->id;
    
    // Create CATCH blocks
    std::vector<int> catchBlockIds;
    for (size_t i = 0; i < stmt.catchClauses.size(); i++) {
        const auto& clause = stmt.catchClauses[i];
        
        std::string catchLabel = "CATCH";
        if (!clause.errorCodes.empty()) {
            catchLabel += " ";
            for (size_t j = 0; j < clause.errorCodes.size(); j++) {
                if (j > 0) catchLabel += ",";
                catchLabel += std::to_string(clause.errorCodes[j]);
            }
        } else {
            catchLabel += " (all)";
        }
        
        BasicBlock* catchBlock = createNewBlock(catchLabel);
        catchBlockIds.push_back(catchBlock->id);
        
        // Process CATCH block statements
        m_currentBlock = catchBlock;
        for (const auto& catchStmt : clause.block) {
            if (catchStmt) {
                processStatement(*catchStmt, m_currentBlock, 0);
            }
        }
    }
    
    // Create FINALLY block if present
    int finallyBlockId = -1;
    if (stmt.hasFinally) {
        BasicBlock* finallyBlock = createNewBlock("FINALLY");
        finallyBlockId = finallyBlock->id;
        
        // Process FINALLY block statements
        m_currentBlock = finallyBlock;
        for (const auto& finallyStmt : stmt.finallyBlock) {
            if (finallyStmt) {
                processStatement(*finallyStmt, m_currentBlock, 0);
            }
        }
    }
    
    // Create exit block
    BasicBlock* exitBlock = createNewBlock("After TRY");
    int exitBlockId = exitBlock->id;
    
    // Store TRY/CATCH structure info for buildEdges phase
    TryCatchContext ctx;
    ctx.tryBlock = trySetupBlockId;
    ctx.tryBodyBlock = tryBodyBlockId;
    ctx.dispatchBlock = dispatchBlockId;
    ctx.catchBlocks = catchBlockIds;
    ctx.finallyBlock = finallyBlockId;
    ctx.exitBlock = exitBlockId;
    ctx.hasFinally = stmt.hasFinally;
    ctx.tryStatement = &stmt;
    m_tryCatchStack.push_back(ctx);
    
    // Also store in the CFG for later reference
    ControlFlowGraph::TryCatchBlocks cfgBlocks;
    cfgBlocks.tryBlock = trySetupBlockId;
    cfgBlocks.tryBodyBlock = tryBodyBlockId;
    cfgBlocks.dispatchBlock = dispatchBlockId;
    cfgBlocks.catchBlocks = catchBlockIds;
    cfgBlocks.finallyBlock = finallyBlockId;
    cfgBlocks.exitBlock = exitBlockId;
    cfgBlocks.hasFinally = stmt.hasFinally;
    cfgBlocks.tryStatement = &stmt;
    m_currentCFG->tryCatchStructure[trySetupBlockId] = cfgBlocks;
    
    // Continue with exit block
    m_currentBlock = exitBlock;
}

} // namespace FasterBASIC