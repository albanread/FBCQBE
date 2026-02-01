//
// cfg_builder_conditional.cpp
// FasterBASIC - Control Flow Graph Builder Conditional Handlers (V2)
//
// Contains IF...THEN...ELSE and SELECT CASE statement processing.
// Part of modular CFG builder split (February 2026).
//
// V2 IMPLEMENTATION: Single-pass recursive construction with immediate edge wiring
//

#include "cfg_builder.h"
#include <iostream>
#include <stdexcept>

namespace FasterBASIC {

// =============================================================================
// IF Statement Handler
// =============================================================================
//
// IF...THEN...ELSE...END IF
// Creates blocks for condition, then branch, else branch, and merge point
// Recursively processes nested statements in each branch
//
BasicBlock* CFGBuilder::buildIf(
    const IfStatement& stmt,
    BasicBlock* incoming,
    LoopContext* loop,
    SelectContext* select,
    TryContext* tryCtx,
    SubroutineContext* sub
) {
    if (m_debugMode) {
        std::cout << "[CFG] Building IF statement" << std::endl;
    }
    
    // Check if this is a single-line IF with GOTO
    if (stmt.hasGoto && stmt.thenStatements.empty() && stmt.elseStatements.empty()) {
        // Single-line IF...THEN GOTO line_number
        // This is just a conditional branch, handle inline
        if (m_debugMode) {
            std::cout << "[CFG] Single-line IF GOTO to line " << stmt.gotoLine << std::endl;
        }
        
        addStatementToBlock(incoming, &stmt, getLineNumber(&stmt));
        
        // Create merge block for fallthrough (when condition is false)
        BasicBlock* mergeBlock = createBlock("If_Merge");
        
        // Add conditional edge to GOTO target (when true)
        int targetBlock = resolveLineNumberToBlock(stmt.gotoLine);
        if (targetBlock >= 0) {
            addConditionalEdge(incoming->id, targetBlock, "true");
        } else {
            // Forward reference - defer
            DeferredEdge edge;
            edge.sourceBlockId = incoming->id;
            edge.targetLineNumber = stmt.gotoLine;
            edge.label = "true";
            m_deferredEdges.push_back(edge);
        }
        
        // Add fallthrough edge (when false)
        addConditionalEdge(incoming->id, mergeBlock->id, "false");
        
        if (m_debugMode) {
            std::cout << "[CFG] IF GOTO complete, merge block: " << mergeBlock->id << std::endl;
        }
        
        return mergeBlock;
    }
    
    // Check if this is a single-line IF with inline statements
    if (!stmt.isMultiLine && !stmt.thenStatements.empty()) {
        // Single-line IF...THEN statement [ELSE statement]
        // e.g., IF x > 5 THEN PRINT "yes" ELSE PRINT "no"
        if (m_debugMode) {
            std::cout << "[CFG] Single-line IF with inline statements" << std::endl;
        }
        
        addStatementToBlock(incoming, &stmt, getLineNumber(&stmt));
        
        // Create blocks for then/else/merge
        BasicBlock* thenBlock = createBlock("If_Then");
        BasicBlock* elseBlock = stmt.elseStatements.empty() ? nullptr : createBlock("If_Else");
        BasicBlock* mergeBlock = createBlock("If_Merge");
        
        // Wire condition to branches
        addConditionalEdge(incoming->id, thenBlock->id, "true");
        
        if (elseBlock) {
            addConditionalEdge(incoming->id, elseBlock->id, "false");
        } else {
            addConditionalEdge(incoming->id, mergeBlock->id, "false");
        }
        
        // Build THEN branch
        BasicBlock* thenExit = buildStatementRange(
            stmt.thenStatements,
            thenBlock,
            loop,
            select,
            tryCtx,
            sub
        );
        
        // Wire THEN to merge
        if (!isTerminated(thenExit)) {
            addUnconditionalEdge(thenExit->id, mergeBlock->id);
        }
        
        // Build ELSE branch if present
        if (elseBlock) {
            BasicBlock* elseExit = buildStatementRange(
                stmt.elseStatements,
                elseBlock,
                loop,
                select,
                tryCtx,
                sub
            );
            
            if (!isTerminated(elseExit)) {
                addUnconditionalEdge(elseExit->id, mergeBlock->id);
            }
        }
        
        if (m_debugMode) {
            std::cout << "[CFG] Single-line IF complete, merge block: " << mergeBlock->id << std::endl;
        }
        
        return mergeBlock;
    }
    
    // Multi-line IF...THEN...ELSE...END IF
    if (m_debugMode) {
        std::cout << "[CFG] Multi-line IF statement" << std::endl;
    }
    
    // 1. Setup blocks
    BasicBlock* conditionBlock = incoming;
    BasicBlock* thenEntry = createBlock("If_Then");
    BasicBlock* elseEntry = stmt.elseStatements.empty() ? nullptr : createBlock("If_Else");
    BasicBlock* mergeBlock = createBlock("If_Merge");
    
    // 2. Add condition check to incoming block
    addStatementToBlock(conditionBlock, &stmt, getLineNumber(&stmt));
    
    // 3. Wire condition to branches
    addConditionalEdge(conditionBlock->id, thenEntry->id, "true");
    
    if (elseEntry) {
        addConditionalEdge(conditionBlock->id, elseEntry->id, "false");
    } else {
        // No ELSE branch: false goes directly to merge
        addConditionalEdge(conditionBlock->id, mergeBlock->id, "false");
    }
    
    // 4. Recursively build THEN branch
    // KEY FIX: This handles nested loops/IFs automatically!
    BasicBlock* thenExit = buildStatementRange(
        stmt.thenStatements,
        thenEntry,
        loop,
        select,
        tryCtx,
        sub
    );
    
    // 5. Wire THEN exit to merge (if not terminated by GOTO/RETURN)
    if (!isTerminated(thenExit)) {
        addUnconditionalEdge(thenExit->id, mergeBlock->id);
    }
    
    // 6. Recursively build ELSE branch (if exists)
    if (elseEntry) {
        BasicBlock* elseExit = buildStatementRange(
            stmt.elseStatements,
            elseEntry,
            loop,
            select,
            tryCtx,
            sub
        );
        
        // Wire ELSE exit to merge (if not terminated)
        if (!isTerminated(elseExit)) {
            addUnconditionalEdge(elseExit->id, mergeBlock->id);
        }
    }
    
    if (m_debugMode) {
        std::cout << "[CFG] Multi-line IF complete, merge block: " << mergeBlock->id << std::endl;
    }
    
    // 7. Return merge point
    // The next statement in the outer scope connects here
    return mergeBlock;
}

// =============================================================================
// SELECT CASE Statement Handler
// =============================================================================
//
// SELECT CASE expression
//   CASE value1, value2, ...
//     statements
//   CASE ELSE
//     statements
// END SELECT
//
// Creates blocks for each case, evaluates expression once, branches to matching case
//
BasicBlock* CFGBuilder::buildSelectCase(
    const CaseStatement& stmt,
    BasicBlock* incoming,
    LoopContext* loop,
    SelectContext* outerSelect,
    TryContext* tryCtx,
    SubroutineContext* sub
) {
    if (m_debugMode) {
        std::cout << "[CFG] Building SELECT CASE statement with " 
                  << stmt.whenClauses.size() << " when clauses" << std::endl;
    }
    
    // 1. Add SELECT CASE to incoming block (evaluates expression)
    addStatementToBlock(incoming, &stmt, getLineNumber(&stmt));
    
    // 2. Create exit block for the entire SELECT
    BasicBlock* exitBlock = createBlock("Select_Exit");
    
    // 3. Create SELECT context (though EXIT SELECT doesn't exist in this dialect)
    SelectContext selectCtx;
    selectCtx.exitBlockId = exitBlock->id;
    selectCtx.outerSelect = outerSelect;
    
    // 4. Process each WHEN clause
    BasicBlock* previousCaseCheck = incoming;
    
    for (size_t i = 0; i < stmt.whenClauses.size(); i++) {
        const auto& whenClause = stmt.whenClauses[i];
        
        if (m_debugMode) {
            std::cout << "[CFG] Processing WHEN clause " << i << std::endl;
        }
        
        // Create block for this when's statements
        BasicBlock* whenBlock = createBlock("When_" + std::to_string(i));
        
        // Create block for next when check (or otherwise for last when)
        BasicBlock* nextCheck = (i < stmt.whenClauses.size() - 1) 
            ? createBlock("When_Check_" + std::to_string(i + 1))
            : createBlock("When_Otherwise");
        
        // Wire from previous check to this when (if match) and to next check (if not)
        std::string whenLabel = "when_" + std::to_string(i);
        addConditionalEdge(previousCaseCheck->id, whenBlock->id, whenLabel);
        addConditionalEdge(previousCaseCheck->id, nextCheck->id, "no_match");
        
        // Recursively build when statements
        BasicBlock* whenExit = buildStatementRange(
            whenClause.statements,
            whenBlock,
            loop,
            &selectCtx,
            tryCtx,
            sub
        );
        
        // Wire when exit to SELECT exit (if not terminated)
        // Note: Cases don't fall through in most BASIC dialects
        if (!isTerminated(whenExit)) {
            addUnconditionalEdge(whenExit->id, exitBlock->id);
        }
        
        // Move to next when check
        previousCaseCheck = nextCheck;
    }
    
    // 5. Process OTHERWISE clause if present
    if (!stmt.otherwiseStatements.empty()) {
        BasicBlock* otherwiseBlock = createBlock("Otherwise");
        addUnconditionalEdge(previousCaseCheck->id, otherwiseBlock->id);
        
        BasicBlock* otherwiseExit = buildStatementRange(
            stmt.otherwiseStatements,
            otherwiseBlock,
            loop,
            &selectCtx,
            tryCtx,
            sub
        );
        
        if (!isTerminated(otherwiseExit)) {
            addUnconditionalEdge(otherwiseExit->id, exitBlock->id);
        }
    } else {
        // No OTHERWISE - wire the final check to exit (no case matched)
        addUnconditionalEdge(previousCaseCheck->id, exitBlock->id);
    }
    
    if (m_debugMode) {
        std::cout << "[CFG] SELECT CASE complete, exit block: " << exitBlock->id << std::endl;
    }
    
    // 6. Return exit block
    return exitBlock;
}

} // namespace FasterBASIC