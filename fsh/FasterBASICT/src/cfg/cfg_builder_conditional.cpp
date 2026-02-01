//
// cfg_builder_conditional.cpp
// FasterBASIC - Control Flow Graph Builder Conditional Handlers
//
// Contains IF/THEN/ELSE and SELECT CASE processing.
// Part of modular CFG builder split (February 2026).
//

#include "cfg_builder.h"
#include <algorithm>
#include <sstream>
#include <iostream>
#include <functional>

namespace FasterBASIC {

// =============================================================================
// IF Statement Handler
// =============================================================================

void CFGBuilder::processIfStatement(const IfStatement& stmt, BasicBlock* currentBlock) {
    // IF creates conditional branch
    std::cerr << "[DEBUG] processIfStatement called: isMultiLine=" << stmt.isMultiLine 
              << ", hasGoto=" << stmt.hasGoto 
              << ", thenStmts=" << stmt.thenStatements.size()
              << ", elseStmts=" << stmt.elseStatements.size() << "\n";

    if (stmt.hasGoto) {
        // IF ... THEN GOTO creates two-way branch
        BasicBlock* nextBlock = createNewBlock();
        m_currentBlock = nextBlock;
    } else if (stmt.isMultiLine) {
        // Multi-line IF...END IF: Create separate blocks for THEN/ELSE branches
        // Use proper block ordering: create convergence point AFTER nested statements
        std::cerr << "[DEBUG] Creating THEN/ELSE blocks for multiline IF\n";
        
        // 1. Create the branch targets
        BasicBlock* thenBlock = createNewBlock("IF THEN");
        BasicBlock* elseBlock = createNewBlock("IF ELSE");
        std::cerr << "[DEBUG] Created thenBlock=" << thenBlock->id << ", elseBlock=" << elseBlock->id << "\n";
        
        // 2. Link the IF header to both branches immediately
        addConditionalEdge(currentBlock->id, thenBlock->id, "true");
        addConditionalEdge(currentBlock->id, elseBlock->id, "false");
        
        // 3. Process the THEN branch
        // This might create many internal blocks if there are nested loops
        m_currentBlock = thenBlock;
        std::cerr << "[DEBUG] Processing THEN branch with " << stmt.thenStatements.size() << " statements\n";
        if (!stmt.thenStatements.empty()) {
            processNestedStatements(stmt.thenStatements, thenBlock, stmt.location.line);
        }
        std::cerr << "[DEBUG] THEN branch processed, m_currentBlock=" << m_currentBlock->id << "\n";
        // Capture the "exit tip" of the THEN branch
        BasicBlock* thenExitTip = m_currentBlock;
        
        // 4. Process the ELSE branch
        m_currentBlock = elseBlock;
        if (!stmt.elseStatements.empty()) {
            processNestedStatements(stmt.elseStatements, elseBlock, stmt.location.line);
        }
        // Capture the "exit tip" of the ELSE branch
        BasicBlock* elseExitTip = m_currentBlock;
        
        // 5. Create the convergence point (After IF)
        // By creating this AFTER the nested statements, it will naturally
        // have a higher ID than anything inside the THEN/ELSE blocks
        BasicBlock* afterIfBlock = createNewBlock("After IF");
        
        // 6. Bridge the exit tips to the convergence point
        // Only bridge if the branch didn't end in a terminator
        bool thenHasTerminator = !thenExitTip->statements.empty() && 
                                (thenExitTip->statements.back()->getType() == ASTNodeType::STMT_EXIT ||
                                 thenExitTip->statements.back()->getType() == ASTNodeType::STMT_RETURN ||
                                 thenExitTip->statements.back()->getType() == ASTNodeType::STMT_GOTO ||
                                 thenExitTip->statements.back()->getType() == ASTNodeType::STMT_END);
        
        bool elseHasTerminator = !elseExitTip->statements.empty() && 
                                (elseExitTip->statements.back()->getType() == ASTNodeType::STMT_EXIT ||
                                 elseExitTip->statements.back()->getType() == ASTNodeType::STMT_RETURN ||
                                 elseExitTip->statements.back()->getType() == ASTNodeType::STMT_GOTO ||
                                 elseExitTip->statements.back()->getType() == ASTNodeType::STMT_END);
        
        if (!thenHasTerminator) {
            addFallthroughEdge(thenExitTip->id, afterIfBlock->id);
        }
        if (!elseHasTerminator) {
            addFallthroughEdge(elseExitTip->id, afterIfBlock->id);
        }
        
        // 7. Update the builder's state
        m_currentBlock = afterIfBlock;
    } else {
        // Single-line IF: IF x THEN statement
        // Do NOT process nested statements here - leave them in the AST
        // The code generator will emit them with proper conditional branching
        // This is different from multi-line IF which uses CFG-driven branching
        
        // Single-line IF statements should be handled by emitIf() in codegen
        // which will emit: evaluate condition, jnz to then/else labels, emit statements
    }
}

// =============================================================================
// SELECT CASE Statement Handler
// =============================================================================

void CFGBuilder::processCaseStatement(const CaseStatement& stmt, BasicBlock* currentBlock) {
    // SELECT CASE creates a multi-way branch structure
    // Structure:
    //   - SELECT block (current): Evaluates the SELECT CASE expression
    //   - Test blocks: One per CASE clause, contains comparison logic
    //   - Body blocks: One per CASE clause, executes CASE statements
    //   - ELSE block: Optional, for ELSE clause
    //   - Exit block: Continue after END SELECT
    
    // The SELECT statement stays in current block for expression evaluation
    
    // Create exit block
    BasicBlock* exitBlock = createNewBlock("After SELECT CASE");
    
    // For each CASE clause, create test block and body block
    std::vector<int> testBlockIds;
    std::vector<int> bodyBlockIds;
    
    for (size_t i = 0; i < stmt.whenClauses.size(); i++) {
        // Test block will contain the comparison logic
        BasicBlock* testBlock = createNewBlock("CASE " + std::to_string(i) + " Test");
        testBlockIds.push_back(testBlock->id);
        
        // Body block will contain the CASE statements
        BasicBlock* bodyBlock = createNewBlock("CASE " + std::to_string(i) + " Body");
        bodyBlockIds.push_back(bodyBlock->id);
        
        // Process statements in the body block
        m_currentBlock = bodyBlock;
        for (const auto& caseStmt : stmt.whenClauses[i].statements) {
            if (caseStmt) {
                processStatement(*caseStmt, bodyBlock, 0);
            }
        }
    }
    
    // Create ELSE block if there are OTHERWISE statements
    int elseBlockId = -1;
    if (!stmt.otherwiseStatements.empty()) {
        BasicBlock* elseBlock = createNewBlock("CASE ELSE");
        elseBlockId = elseBlock->id;
        
        m_currentBlock = elseBlock;
        for (const auto& elseStmt : stmt.otherwiseStatements) {
            if (elseStmt) {
                processStatement(*elseStmt, elseBlock, 0);
            }
        }
    }
    
    // Store SELECT CASE info for buildEdges phase
    SelectCaseContext ctx;
    ctx.selectBlock = currentBlock->id;
    ctx.testBlocks = testBlockIds;
    ctx.bodyBlocks = bodyBlockIds;
    ctx.elseBlock = elseBlockId;
    ctx.exitBlock = exitBlock->id;
    ctx.caseStatement = &stmt;
    m_selectCaseStack.push_back(ctx);
    
    // Continue with exit block
    m_currentBlock = exitBlock;
}

} // namespace FasterBASIC