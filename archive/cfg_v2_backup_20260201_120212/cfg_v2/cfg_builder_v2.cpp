//
// cfg_builder_v2.cpp
// FasterBASIC - Control Flow Graph Builder V2 Implementation
//
// ARCHITECTURAL REDESIGN (February 2025)
//
// This implements the single-pass recursive CFG construction approach.
// See cfg_builder_v2.h for architectural overview and design rationale.
//

#include "cfg_builder_v2.h"
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace FasterBASIC {

// =============================================================================
// Constructor / Destructor
// =============================================================================

CFGBuilderV2::CFGBuilderV2()
    : m_cfg(nullptr)
    , m_nextBlockId(0)
    , m_totalBlocksCreated(0)
    , m_totalEdgesCreated(0)
    , m_debugMode(false)
    , m_entryBlock(nullptr)
    , m_exitBlock(nullptr)
{
}

CFGBuilderV2::~CFGBuilderV2() {
    // Note: m_cfg ownership is transferred via takeCFG()
    // Don't delete here
}

// =============================================================================
// Main Entry Point
// =============================================================================

ControlFlowGraph* CFGBuilderV2::build(const std::vector<StatementPtr>& statements) {
    if (m_debugMode) {
        std::cout << "[CFGv2] Starting CFG construction..." << std::endl;
        std::cout << "[CFGv2] Total statements to process: " << statements.size() << std::endl;
    }
    
    // Create the CFG
    m_cfg = new ControlFlowGraph();
    m_nextBlockId = 0;
    m_totalBlocksCreated = 0;
    m_totalEdgesCreated = 0;
    
    // PHASE 0: Pre-scan to collect all GOTO/GOSUB targets
    // This identifies "landing zones" that require block boundaries
    collectJumpTargets(statements);
    
    if (m_debugMode) {
        std::cout << "[CFGv2] Pre-scan found " << m_lineNumberToBlock.size() 
                  << " jump targets" << std::endl;
    }
    
    // Create entry block
    m_entryBlock = createBlock("Entry");
    m_cfg->entryBlock = m_entryBlock->id;
    
    // Build the main program body
    // No context parameters (not in any loop/select/try)
    BasicBlock* finalBlock = buildStatementRange(
        statements,
        m_entryBlock,
        nullptr,  // No loop context
        nullptr,  // No select context
        nullptr,  // No try context
        nullptr   // No subroutine context
    );
    
    // Create exit block
    m_exitBlock = createBlock("Exit");
    m_cfg->exitBlock = m_exitBlock->id;
    
    // Wire final block to exit (if not already terminated)
    if (finalBlock && !isTerminated(finalBlock)) {
        addUnconditionalEdge(finalBlock->id, m_exitBlock->id);
    }
    
    // PHASE 2: Resolve any deferred edges (forward GOTOs)
    resolveDeferredEdges();
    
    if (m_debugMode) {
        std::cout << "[CFGv2] CFG construction complete" << std::endl;
        std::cout << "[CFGv2] Total blocks created: " << m_totalBlocksCreated << std::endl;
        std::cout << "[CFGv2] Total edges created: " << m_totalEdgesCreated << std::endl;
        dumpCFG("Final");
    }
    
    return m_cfg;
}

ControlFlowGraph* CFGBuilderV2::takeCFG() {
    ControlFlowGraph* result = m_cfg;
    m_cfg = nullptr;
    return result;
}

// Adapter: Build CFG from Program structure
ControlFlowGraph* CFGBuilderV2::buildFromProgram(const Program& program) {
    if (m_debugMode) {
        std::cout << "[CFGv2] Building CFG from Program with " 
                  << program.lines.size() << " lines" << std::endl;
    }
    
    // Create the CFG
    m_cfg = new ControlFlowGraph();
    m_nextBlockId = 0;
    m_totalBlocksCreated = 0;
    m_totalEdgesCreated = 0;
    
    // PHASE 0: Pre-scan to collect all GOTO/GOSUB targets
    collectJumpTargetsFromProgram(program);
    
    if (m_debugMode) {
        std::cout << "[CFGv2] Pre-scan found " << m_lineNumberToBlock.size() 
                  << " jump targets" << std::endl;
    }
    
    // Create entry block
    m_entryBlock = createBlock("Entry");
    m_cfg->entryBlock = m_entryBlock->id;
    
    // PHASE 1: Process each line's statements
    BasicBlock* currentBlock = m_entryBlock;
    
    for (const auto& line : program.lines) {
        int lineNum = line->lineNumber;
        
        // PRE-CHECK: Is this line a jump target?
        // If so, we must start a new block here (landing zone)
        if (lineNum > 0 && isJumpTarget(lineNum)) {
            BasicBlock* targetBlock = createBlock("Target_" + std::to_string(lineNum));
            
            // Map this line number to the new block
            registerLineNumberBlock(lineNum, targetBlock->id);
            
            // Link previous block to this one (if not terminated)
            if (!isTerminated(currentBlock)) {
                addUnconditionalEdge(currentBlock->id, targetBlock->id);
            }
            
            currentBlock = targetBlock;
            
            if (m_debugMode) {
                std::cout << "[CFGv2] Created landing zone block " << targetBlock->id 
                          << " for line " << lineNum << std::endl;
            }
        } else if (lineNum > 0) {
            // Register line number even if not a jump target (for debugging)
            registerLineNumberBlock(lineNum, currentBlock->id);
        }
        
        // Process statements in this line
        for (const auto& stmt : line->statements) {
            if (isTerminated(currentBlock)) {
                // Previous statement was a terminator (GOTO/RETURN)
                // Create unreachable block for following code
                currentBlock = createUnreachableBlock();
                m_unreachableBlocks.push_back(currentBlock);
                
                if (m_debugMode) {
                    std::cout << "[CFGv2] Created unreachable block " << currentBlock->id 
                              << " after terminator" << std::endl;
                }
            }
            
            ASTNodeType type = stmt->getType();
            
            // Handle control structures and chaos elements
            switch (type) {
                case ASTNodeType::STMT_IF:
                    currentBlock = buildIf(
                        static_cast<const IfStatement&>(*stmt),
                        currentBlock,
                        nullptr, nullptr, nullptr, nullptr
                    );
                    break;
                
                case ASTNodeType::STMT_WHILE:
                    currentBlock = buildWhile(
                        static_cast<const WhileStatement&>(*stmt),
                        currentBlock,
                        nullptr, nullptr, nullptr, nullptr
                    );
                    break;
                
                case ASTNodeType::STMT_FOR:
                    currentBlock = buildFor(
                        static_cast<const ForStatement&>(*stmt),
                        currentBlock,
                        nullptr, nullptr, nullptr, nullptr
                    );
                    break;
                
                case ASTNodeType::STMT_REPEAT:
                    currentBlock = buildRepeat(
                        static_cast<const RepeatStatement&>(*stmt),
                        currentBlock,
                        nullptr, nullptr, nullptr, nullptr
                    );
                    break;
                
                case ASTNodeType::STMT_DO:
                    currentBlock = buildDo(
                        static_cast<const DoStatement&>(*stmt),
                        currentBlock,
                        nullptr, nullptr, nullptr, nullptr
                    );
                    break;
                
                // CHAOS ELEMENTS (Teleporters)
                case ASTNodeType::STMT_GOTO:
                    currentBlock = handleGoto(
                        static_cast<const GotoStatement&>(*stmt),
                        currentBlock
                    );
                    break;
                
                case ASTNodeType::STMT_GOSUB:
                    currentBlock = handleGosub(
                        static_cast<const GosubStatement&>(*stmt),
                        currentBlock,
                        nullptr, nullptr, nullptr, nullptr
                    );
                    break;
                
                case ASTNodeType::STMT_RETURN:
                    currentBlock = handleReturn(
                        static_cast<const ReturnStatement&>(*stmt),
                        currentBlock,
                        nullptr
                    );
                    break;
                
                case ASTNodeType::STMT_ON_GOTO:
                    currentBlock = handleOnGoto(
                        static_cast<const OnGotoStatement&>(*stmt),
                        currentBlock
                    );
                    break;
                
                case ASTNodeType::STMT_ON_GOSUB:
                    currentBlock = handleOnGosub(
                        static_cast<const OnGosubStatement&>(*stmt),
                        currentBlock,
                        nullptr, nullptr, nullptr, nullptr
                    );
                    break;
                
                case ASTNodeType::STMT_END:
                    currentBlock = handleEnd(
                        static_cast<const EndStatement&>(*stmt),
                        currentBlock
                    );
                    break;
                
                default:
                    // Regular statement - add to current block
                    addStatementToBlock(currentBlock, stmt.get(), lineNum);
                    break;
            }
        }
    }
    
    // Create exit block
    m_exitBlock = createBlock("Exit");
    if (!isTerminated(currentBlock)) {
        addUnconditionalEdge(currentBlock->id, m_exitBlock->id);
    }
    
    // PHASE 2: Resolve any deferred edges (forward GOTOs)
    resolveDeferredEdges();
    
    if (m_debugMode) {
        std::cout << "[CFGv2] CFG construction complete." << std::endl;
        std::cout << "[CFGv2] Total blocks created: " << m_totalBlocksCreated << std::endl;
        std::cout << "[CFGv2] Total edges created: " << m_totalEdgesCreated << std::endl;
    }
    
    return m_cfg;
}

// =============================================================================
// Jump Target Collection (Phase 0)
// =============================================================================

void CFGBuilderV2::collectJumpTargets(const std::vector<StatementPtr>& statements) {
    for (const auto& stmt : statements) {
        collectJumpTargetsFromStatement(stmt.get());
    }
}

void CFGBuilderV2::collectJumpTargetsFromProgram(const Program& program) {
    for (const auto& line : program.lines) {
        for (const auto& stmt : line->statements) {
            collectJumpTargetsFromStatement(stmt.get());
        }
    }
}

void CFGBuilderV2::collectJumpTargetsFromStatement(const Statement* stmt) {
    if (!stmt) return;
    
    ASTNodeType type = stmt->getType();
    
    switch (type) {
        case ASTNodeType::STMT_GOTO: {
            const auto* gotoStmt = static_cast<const GotoStatement*>(stmt);
            m_jumpTargets.insert(gotoStmt->targetLine);
            break;
        }
        
        case ASTNodeType::STMT_GOSUB: {
            const auto* gosubStmt = static_cast<const GosubStatement*>(stmt);
            m_jumpTargets.insert(gosubStmt->targetLine);
            break;
        }
        
        case ASTNodeType::STMT_ON_GOTO: {
            const auto* onGotoStmt = static_cast<const OnGotoStatement*>(stmt);
            for (int target : onGotoStmt->lineNumbers) {
                m_jumpTargets.insert(target);
            }
            break;
        }
        
        case ASTNodeType::STMT_ON_GOSUB: {
            const auto* onGosubStmt = static_cast<const OnGosubStatement*>(stmt);
            for (int target : onGosubStmt->lineNumbers) {
                m_jumpTargets.insert(target);
            }
            break;
        }
        
        // Recursively scan structured statements
        case ASTNodeType::STMT_IF: {
            const auto* ifStmt = static_cast<const IfStatement*>(stmt);
            for (const auto& thenStmt : ifStmt->thenStatements) {
                collectJumpTargetsFromStatement(thenStmt.get());
            }
            for (const auto& elseStmt : ifStmt->elseStatements) {
                collectJumpTargetsFromStatement(elseStmt.get());
            }
            break;
        }
        
        case ASTNodeType::STMT_WHILE: {
            const auto* whileStmt = static_cast<const WhileStatement*>(stmt);
            for (const auto& bodyStmt : whileStmt->body) {
                collectJumpTargetsFromStatement(bodyStmt.get());
            }
            break;
        }
        
        case ASTNodeType::STMT_FOR: {
            const auto* forStmt = static_cast<const ForStatement*>(stmt);
            for (const auto& bodyStmt : forStmt->body) {
                collectJumpTargetsFromStatement(bodyStmt.get());
            }
            break;
        }
        
        case ASTNodeType::STMT_REPEAT: {
            const auto* repeatStmt = static_cast<const RepeatStatement*>(stmt);
            for (const auto& bodyStmt : repeatStmt->body) {
                collectJumpTargetsFromStatement(bodyStmt.get());
            }
            break;
        }
        
        case ASTNodeType::STMT_DO: {
            const auto* doStmt = static_cast<const DoStatement*>(stmt);
            for (const auto& bodyStmt : doStmt->body) {
                collectJumpTargetsFromStatement(bodyStmt.get());
            }
            break;
        }
        
        default:
            // Other statements don't contain jump targets
            break;
    }
}

bool CFGBuilderV2::isJumpTarget(int lineNumber) const {
    return m_jumpTargets.find(lineNumber) != m_jumpTargets.end();
}

// =============================================================================
// Core Recursive Builder
// =============================================================================

// Helper structure for tracking how many statements were consumed
struct BuildResult {
    BasicBlock* exitBlock;
    size_t statementsConsumed;
};

BasicBlock* CFGBuilderV2::buildStatementRange(
    const std::vector<StatementPtr>& statements,
    BasicBlock* incoming,
    LoopContext* currentLoop,
    SelectContext* currentSelect,
    TryContext* currentTry,
    SubroutineContext* currentSub
) {
    if (!incoming) {
        throw std::runtime_error("CFGv2: incoming block is null in buildStatementRange");
    }
    
    BasicBlock* currentBlock = incoming;
    
    // Use index-based iteration to handle multi-statement structures
    for (size_t i = 0; i < statements.size(); /* incremented in loop */) {
        const auto& stmt = statements[i];
        
        // If current block is terminated (GOTO, RETURN, etc.),
        // create a new unreachable block for subsequent statements
        if (isTerminated(currentBlock)) {
            if (m_debugMode) {
                std::cout << "[CFGv2] Current block is terminated, creating unreachable block" << std::endl;
            }
            currentBlock = createUnreachableBlock();
            m_unreachableBlocks.push_back(currentBlock);
        }
        
        // Get statement type
        ASTNodeType type = stmt->getType();
        size_t consumed = 1;  // By default, consume 1 statement
        
        // Dispatch to appropriate builder based on statement type
        switch (type) {
            case ASTNodeType::STMT_IF:
                currentBlock = buildIf(
                    static_cast<const IfStatement&>(*stmt),
                    currentBlock,
                    currentLoop,
                    currentSelect,
                    currentTry,
                    currentSub
                );
                break;
            
            case ASTNodeType::STMT_WHILE: {
                auto result = buildWhileFromIndex(
                    statements,
                    i,
                    currentBlock,
                    currentLoop,
                    currentSelect,
                    currentTry,
                    currentSub
                );
                currentBlock = result.exitBlock;
                consumed = result.statementsConsumed;
                break;
            }</parameter>
                break;
            
            case ASTNodeType::STMT_FOR: {
                auto result = buildForFromIndex(
                    statements,
                    i,
                    currentBlock,
                    currentLoop,
                    currentSelect,
                    currentTry,
                    currentSub
                );
                currentBlock = result.exitBlock;
                consumed = result.statementsConsumed;
                break;
            }
            
            case ASTNodeType::STMT_REPEAT: {
                auto result = buildRepeatFromIndex(
                    statements,
                    i,
                    currentBlock,
                    currentLoop,
                    currentSelect,
                    currentTry,
                    currentSub
                );
                currentBlock = result.exitBlock;
                consumed = result.statementsConsumed;
                break;
            }
            
            case ASTNodeType::STMT_DO: {
                auto result = buildDoFromIndex(
                    statements,
                    i,
                    currentBlock,
                    currentLoop,
                    currentSelect,
                    currentTry,
                    currentSub
                );
                currentBlock = result.exitBlock;
                consumed = result.statementsConsumed;
                break;
            }</parameter>
                break;
            
            case ASTNodeType::STMT_SELECT_CASE:
                currentBlock = buildSelectCase(
                    static_cast<const SelectCaseStatement&>(*stmt),
                    currentBlock,
                    currentLoop,
                    currentSelect,
                    currentTry,
                    currentSub
                );
                break;
            
            case ASTNodeType::STMT_TRY:
                currentBlock = buildTryCatch(
                    static_cast<const TryStatement&>(*stmt),
                    currentBlock,
                    currentLoop,
                    currentSelect,
                    currentTry,
                    currentSub
                );
                break;
            
            case ASTNodeType::STMT_GOTO:
                currentBlock = handleGoto(
                    static_cast<const GotoStatement&>(*stmt),
                    currentBlock
                );
                break;
            
            case ASTNodeType::STMT_GOSUB:
                currentBlock = handleGosub(
                    static_cast<const GosubStatement&>(*stmt),
                    currentBlock,
                    currentLoop,
                    currentSelect,
                    currentTry,
                    currentSub
                );
                break;
            
            case ASTNodeType::STMT_RETURN:
                currentBlock = handleReturn(
                    static_cast<const ReturnStatement&>(*stmt),
                    currentBlock,
                    currentSub
                );
                break;
            
            case ASTNodeType::STMT_EXIT_FOR:
                currentBlock = handleExitFor(currentBlock, currentLoop);
                break;
            
            case ASTNodeType::STMT_EXIT_WHILE:
                currentBlock = handleExitWhile(currentBlock, currentLoop);
                break;
            
            case ASTNodeType::STMT_EXIT_DO:
                currentBlock = handleExitDo(currentBlock, currentLoop);
                break;
            
            case ASTNodeType::STMT_EXIT_SELECT:
                currentBlock = handleExitSelect(currentBlock, currentSelect);
                break;
            
            case ASTNodeType::STMT_CONTINUE:
                currentBlock = handleContinue(currentBlock, currentLoop);
                break;
            
            case ASTNodeType::STMT_END:
                currentBlock = handleEnd(
                    static_cast<const EndStatement&>(*stmt),
                    currentBlock
                );
                break;
            
            case ASTNodeType::STMT_THROW:
                currentBlock = handleThrow(
                    static_cast<const ThrowStatement&>(*stmt),
                    currentBlock,
                    currentTry
                );
                break;
            
            // Loop end markers - skip them (already handled by loop builders)
            case ASTNodeType::STMT_WEND:
            case ASTNode   // Regular statement (LET, PRINT, DIM, etc.)
                // Just add to current block
                addStatementToBlock(currentBlock, stmt.get(), getLineNumber(stmt.get()));
                break;
        }
    }
    
    return currentBlock;
}

// =============================================================================
// IF Statement Builder
// =============================================================================

BasicBlock* CFGBuilderV2::buildIf(
    const IfStatement& stmt,
    BasicBlock* incoming,
    LoopContext* loop,
    SelectContext* select,
    TryContext* tryCtx,
    SubroutineContext* sub
) {
    if (m_debugMode) {
        std::cout << "[CFGv2] Building IF statement" << std::endl;
    }
    
    // Check if this is a single-line IF with GOTO
    if (stmt.hasGoto && stmt.thenStatements.empty() && stmt.elseStatements.empty()) {
        // Single-line IF...THEN GOTO
        // This is just a conditional branch, handle inline
        addStatementToBlock(incoming, &stmt, getLineNumber(&stmt));
        
        // Create merge block for fallthrough (when condition is false)
        BasicBlock* mergeBlock = createBlock("If_Merge");
        
        // Add conditional edge to GOTO target (when true)
        int targetBlock = resolveLineNumberToBlock(stmt.gotoTarget);
        addConditionalEdge(incoming->id, targetBlock, "true");
        
        // Add fallthrough edge (when false)
        addConditionalEdge(incoming->id, mergeBlock->id, "false");
        
        return mergeBlock;
    }
    
    // Multi-line IF...THEN...ELSE...END IF
    
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
    
    // 7. Handle ELSEIF clauses (if any)
    // TODO: Implement ELSEIF handling
    // For now, treat as nested IF...ELSE
    
    if (m_debugMode) {
        std::cout << "[CFGv2] IF statement complete, merge block: " << mergeBlock->id << std::endl;
    }
    
    // 8. Return merge point
    // The next statement in the outer scope connects here
    return mergeBlock;
}

// =============================================================================
// Block and Edge Management
// =============================================================================

BasicBlock* CFGBuilderV2::createBlock(const std::string& label) {
    BasicBlock* block = new BasicBlock();
    block->id = m_nextBlockId++;
    block->label = label;
    block->isLoopHeader = false;
    block->isLoopExit = false;
    block->isSubroutine = false;
    block->isTerminator = false;
    
    m_cfg->blocks.push_back(block);
    m_totalBlocksCreated++;
    
    if (m_debugMode) {
        std::cout << "[CFGv2] Created block " << block->id << " (" << label << ")" << std::endl;
    }
    
    return block;
}

BasicBlock* CFGBuilderV2::createUnreachableBlock() {
    BasicBlock* block = createBlock("Unreachable");
    return block;
}

void CFGBuilderV2::addEdge(int fromBlockId, int toBlockId, const std::string& label) {
    // Create edge structure
    CFGEdge edge;
    edge.sourceBlock = fromBlockId;
    edge.targetBlock = toBlockId;
    edge.type = EdgeType::FALLTHROUGH;
    edge.label = label;
    
    m_cfg->edges.push_back(edge);
    
    // Update block successor/predecessor lists
    if (fromBlockId >= 0 && fromBlockId < static_cast<int>(m_cfg->blocks.size())) {
        m_cfg->blocks[fromBlockId]->successors.push_back(toBlockId);
    }
    if (toBlockId >= 0 && toBlockId < static_cast<int>(m_cfg->blocks.size())) {
        m_cfg->blocks[toBlockId]->predecessors.push_back(fromBlockId);
    }
    
    m_totalEdgesCreated++;
    
    if (m_debugMode) {
        std::cout << "[CFGv2] Added edge: Block " << fromBlockId 
                  << " -> Block " << toBlockId;
        if (!label.empty()) {
            std::cout << " [" << label << "]";
        }
        std::cout << std::endl;
    }
}

void CFGBuilderV2::addConditionalEdge(int fromBlockId, int toBlockId, const std::string& condition) {
    addEdge(fromBlockId, toBlockId, condition);
}

void CFGBuilderV2::addUnconditionalEdge(int fromBlockId, int toBlockId) {
    addEdge(fromBlockId, toBlockId, "");
}

void CFGBuilderV2::markTerminated(BasicBlock* block) {
    if (block) {
        block->isTerminator = true;
    }
}

bool CFGBuilderV2::isTerminated(const BasicBlock* block) const {
    return block && block->isTerminator;
}

// =============================================================================
// Statement Management
// =============================================================================

void CFGBuilderV2::addStatementToBlock(BasicBlock* block, const Statement* stmt, int lineNumber) {
    if (!block || !stmt) return;
    
    block->statements.push_back(stmt);
    
    if (lineNumber >= 0) {
        block->lineNumbers.insert(lineNumber);
        block->statementLineNumbers[stmt] = lineNumber;
    }
}

int CFGBuilderV2::getLineNumber(const Statement* stmt) {
    // TODO: Extract line number from statement metadata
    // For now, return -1 (unknown)
    return -1;
}

// =============================================================================
// Label and Line Number Resolution
// =============================================================================

int CFGBuilderV2::resolveLineNumberToBlock(int lineNumber) {
    auto it = m_lineNumberToBlock.find(lineNumber);
    if (it != m_lineNumberToBlock.end()) {
        return it->second;
    }
    
    // Line number not yet seen - create deferred edge
    // This will be resolved later
    return -1;  // Placeholder
}

void CFGBuilderV2::registerLineNumberBlock(int lineNumber, int blockId) {
    m_lineNumberToBlock[lineNumber] = blockId;
}

void CFGBuilderV2::registerLabel(const std::string& label, int blockId) {
    m_labelToBlock[label] = blockId;
}

int CFGBuilderV2::resolveLabelToBlock(const std::string& label) {
    auto it = m_labelToBlock.find(label);
    if (it != m_labelToBlock.end()) {
        return it->second;
    }
    return -1;
}

void CFGBuilderV2::resolveDeferredEdges() {
    // Resolve forward references (GOTOs to later line numbers)
    for (const auto& deferred : m_deferredEdges) {
        int targetBlock = resolveLineNumberToBlock(deferred.targetLineNumber);
        if (targetBlock >= 0) {
            addEdge(deferred.sourceBlockId, targetBlock, deferred.label);
        } else {
            if (m_debugMode) {
                std::cout << "[CFGv2] Warning: Could not resolve line number " 
                          << deferred.targetLineNumber << std::endl;
            }
        }
    }
}

// =============================================================================
// Helper Functions
// =============================================================================

LoopContext* CFGBuilderV2::findLoopContext(LoopContext* ctx, const std::string& loopType) {
    while (ctx) {
        if (ctx->loopType == loopType) {
            return ctx;
        }
        ctx = ctx->outerLoop;
    }
    return nullptr;
}

BasicBlock* CFGBuilderV2::splitBlockIfNeeded(BasicBlock* block) {
    if (!block->statements.empty()) {
        // Block already has statements, create a new one
        BasicBlock* newBlock = createBlock(block->label + "_Split");
        addUnconditionalEdge(block->id, newBlock->id);
        return newBlock;
    }
    return block;
}

void CFGBuilderV2::dumpCFG(const std::string& phase) const {
    if (!m_cfg) return;
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "    CFG V2 DUMP (" << phase << ")" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Function: " << (m_cfg->functionName.empty() ? "main" : m_cfg->functionName) << std::endl;
    std::cout << "Total Blocks: " << m_cfg->blocks.size() << std::endl;
    std::cout << "Total Edges: " << m_cfg->edges.size() << std::endl;
    std::cout << "Entry Block: " << m_cfg->entryBlock << std::endl;
    std::cout << "Exit Block: " << m_cfg->exitBlock << std::endl;
    
    // Helper to get statement type name
    auto getStmtTypeName = [](const Statement* stmt) -> std::string {
        if (!stmt) return "NULL";
        switch (stmt->getType()) {
            case ASTNodeType::STMT_PRINT: return "PRINT";
            case ASTNodeType::STMT_LET: return "LET";
            case ASTNodeType::STMT_IF: return "IF";
            case ASTNodeType::STMT_WHILE: return "WHILE";
            case ASTNodeType::STMT_WEND: return "WEND";
            case ASTNodeType::STMT_FOR: return "FOR";
            case ASTNodeType::STMT_NEXT: return "NEXT";
            case ASTNodeType::STMT_REPEAT: return "REPEAT";
            case ASTNodeType::STMT_UNTIL: return "UNTIL";
            case ASTNodeType::STMT_DO: return "DO";
            case ASTNodeType::STMT_LOOP: return "LOOP";
            case ASTNodeType::STMT_GOTO: return "GOTO";
            case ASTNodeType::STMT_GOSUB: return "GOSUB";
            case ASTNodeType::STMT_RETURN: return "RETURN";
            case ASTNodeType::STMT_ON_GOTO: return "ON...GOTO";
            case ASTNodeType::STMT_ON_GOSUB: return "ON...GOSUB";
            case ASTNodeType::STMT_EXIT_FOR: return "EXIT FOR";
            case ASTNodeType::STMT_EXIT_WHILE: return "EXIT WHILE";
            case ASTNodeType::STMT_EXIT_DO: return "EXIT DO";
            case ASTNodeType::STMT_DIM: return "DIM";
            case ASTNodeType::STMT_REDIM: return "REDIM";
            case ASTNodeType::STMT_SELECT_CASE: return "SELECT CASE";
            case ASTNodeType::STMT_CASE: return "CASE";
            case ASTNodeType::STMT_END_SELECT: return "END SELECT";
            case ASTNodeType::STMT_TRY: return "TRY";
            case ASTNodeType::STMT_CATCH: return "CATCH";
            case ASTNodeType::STMT_FINALLY: return "FINALLY";
            case ASTNodeType::STMT_END_TRY: return "END TRY";
            case ASTNodeType::STMT_THROW: return "THROW";
            default: return "OTHER";
        }
    };
    
    // Dump all blocks
    std::cout << "\n----------------------------------------" << std::endl;
    std::cout << "BLOCKS:" << std::endl;
    std::cout << "----------------------------------------" << std::endl;
    
    for (const auto& blockPtr : m_cfg->blocks) {
        const BasicBlock* block = blockPtr.get();
        if (!block) continue;
        
        std::cout << "\n╔═══ Block " << block->id;
        if (!block->label.empty()) {
            std::cout << " (" << block->label << ")";
        }
        std::cout << " ═══" << std::endl;
        
        // Block flags
        std::cout << "║ Flags: ";
        if (block->isLoopHeader) std::cout << "[LOOP_HEADER] ";
        if (block->isLoopExit) std::cout << "[LOOP_EXIT] ";
        if (block->isTerminator) std::cout << "[TERMINATED] ";
        if (block->statements.empty()) std::cout << "[EMPTY] ";
        std::cout << std::endl;
        
        // Line numbers
        if (!block->lineNumbers.empty()) {
            std::cout << "║ Line Numbers: ";
            for (int line : block->lineNumbers) {
                std::cout << line << " ";
            }
            std::cout << std::endl;
        }
        
        // Statements
        std::cout << "║ Statements (" << block->statements.size() << "):" << std::endl;
        int stmtNum = 0;
        for (const Statement* stmt : block->statements) {
            std::cout << "║   " << (++stmtNum) << ". " << getStmtTypeName(stmt);
            
            // Show line number if available
            auto lineIt = block->statementLineNumbers.find(stmt);
            if (lineIt != block->statementLineNumbers.end()) {
                std::cout << " [line " << lineIt->second << "]";
            }
            std::cout << std::endl;
        }
        
        if (block->statements.empty()) {
            std::cout << "║   (no statements)" << std::endl;
        }
        
        // Predecessors
        std::cout << "║ Predecessors (" << block->predecessors.size() << "): ";
        if (block->predecessors.empty()) {
            std::cout << "(none)";
        } else {
            for (size_t i = 0; i < block->predecessors.size(); i++) {
                if (i > 0) std::cout << ", ";
                std::cout << block->predecessors[i];
            }
        }
        std::cout << std::endl;
        
        // Successors
        std::cout << "║ Successors (" << block->successors.size() << "): ";
        if (block->successors.empty()) {
            std::cout << "(none)";
        } else {
            for (size_t i = 0; i < block->successors.size(); i++) {
                if (i > 0) std::cout << ", ";
                std::cout << block->successors[i];
            }
        }
        std::cout << std::endl;
        
        std::cout << "╚═══════════════════════════════════" << std::endl;
    }
    
    // Dump all edges
    std::cout << "\n----------------------------------------" << std::endl;
    std::cout << "EDGES:" << std::endl;
    std::cout << "----------------------------------------" << std::endl;
    
    for (size_t i = 0; i < m_cfg->edges.size(); i++) {
        const CFGEdge& edge = m_cfg->edges[i];
        std::cout << "Edge " << i << ": Block " << edge.sourceBlock 
                  << " → Block " << edge.targetBlock;
        
        // Edge type
        switch (edge.type) {
            case EdgeType::FALLTHROUGH:
                std::cout << " [FALLTHROUGH]";
                break;
            case EdgeType::CONDITIONAL_TRUE:
                std::cout << " [TRUE]";
                break;
            case EdgeType::CONDITIONAL_FALSE:
                std::cout << " [FALSE]";
                break;
            case EdgeType::JUMP:
                std::cout << " [JUMP]";
                break;
            case EdgeType::CALL:
                std::cout << " [CALL]";
                break;
            case EdgeType::RETURN:
                std::cout << " [RETURN]";
                break;
            default:
                std::cout << " [UNKNOWN]";
                break;
        }
        
        // Edge label
        if (!edge.label.empty()) {
            std::cout << " \"" << edge.label << "\"";
        }
        
        std::cout << std::endl;
    }
    
    // Statistics
    std::cout << "\n----------------------------------------" << std::endl;
    std::cout << "STATISTICS:" << std::endl;
    std::cout << "----------------------------------------" << std::endl;
    std::cout << "Blocks created: " << m_totalBlocksCreated << std::endl;
    std::cout << "Edges created: " << m_totalEdgesCreated << std::endl;
    std::cout << "Jump targets: " << m_jumpTargets.size() << std::endl;
    std::cout << "Unreachable blocks: " << m_unreachableBlocks.size() << std::endl;
    std::cout << "Deferred edges: " << m_deferredEdges.size() << std::endl;
    
    // Line number mappings
    if (!m_lineNumberToBlock.empty()) {
        std::cout << "\nLine Number → Block mappings:" << std::endl;
        for (const auto& [line, block] : m_lineNumberToBlock) {
            std::cout << "  Line " << line << " → Block " << block << std::endl;
        }
    }
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "    END CFG V2 DUMP" << std::endl;
    std::cout << "========================================\n" << std::endl;
}

// =============================================================================
// Terminator Handlers (Stubs for now)
// =============================================================================

BasicBlock* CFGBuilderV2::handleGoto(const GotoStatement& stmt, BasicBlock* incoming) {
    // TODO: Implement
    addStatementToBlock(incoming, &stmt, getLineNumber(&stmt));
    markTerminated(incoming);
    return createUnreachableBlock();
}

BasicBlock* CFGBuilderV2::handleGosub(const GosubStatement& stmt, BasicBlock* incoming,
                                      LoopContext* loop, SelectContext* select,
                                      TryContext* tryCtx, SubroutineContext* outerSub) {
    if (m_debugMode) {
        std::cout << "[CFGv2] Handling GOSUB to line " << stmt.targetLine << std::endl;
    }
    
    // Add GOSUB statement to current block
    addStatementToBlock(incoming, &stmt, getLineNumber(&stmt));
    
    // Create the return point block (where execution continues after RETURN)
    BasicBlock* returnBlock = createBlock("Return_Point");
    
    // Edge A: Call edge to subroutine target
    int targetBlockId = resolveLineNumberToBlock(stmt.targetLine);
    
    if (targetBlockId >= 0) {
        // Target already exists, wire directly
        addEdge(incoming->id, targetBlockId, "call");
    } else {
        // Forward reference - defer until Phase 2
        DeferredEdge edge;
        edge.sourceBlockId = incoming->id;
        edge.targetLineNumber = stmt.targetLine;
        edge.label = "call";
        m_deferredEdges.push_back(edge);
        
        if (m_debugMode) {
            std::cout << "[CFGv2] Deferred GOSUB call edge to line " << stmt.targetLine << std::endl;
        }
    }
    
    // Edge B: Fallthrough edge to return point
    // (Execution continues here after the subroutine RETURNs)
    addUnconditionalEdge(incoming->id, returnBlock->id);
    
    // Create subroutine context for nested code
    SubroutineContext subCtx;
    subCtx.returnBlockId = returnBlock->id;
    subCtx.outerSub = outerSub;
    
    if (m_debugMode) {
        std::cout << "[CFGv2] GOSUB from block " << incoming->id 
                  << " with return point " << returnBlock->id << std::endl;
    }
    
    // Continue building from return point
    return returnBlock;
}

BasicBlock* CFGBuilderV2::handleReturn(const ReturnStatement& stmt, BasicBlock* incoming,
                                        SubroutineContext* sub) {
    if (m_debugMode) {
        std::cout << "[CFGv2] Handling RETURN statement" << std::endl;
    }
    
    // Add RETURN statement to current block
    addStatementToBlock(incoming, &stmt, getLineNumber(&stmt));
    
    // RETURN is a terminator - it pops the call stack and returns to caller
    // We don't know where it goes locally (depends on runtime call stack)
    
    if (sub && sub->returnBlockId >= 0) {
        // We're inside a GOSUB context - return to the caller's return point
        addUnconditionalEdge(incoming->id, sub->returnBlockId);
        
        if (m_debugMode) {
            std::cout << "[CFGv2] RETURN jumps to return point block " << sub->returnBlockId << std::endl;
        }
    } else {
        // RETURN outside of GOSUB context - this is an error in structured code
        // but in BASIC it might just end the program
        // We'll treat it as a terminator with no successor
        if (m_debugMode) {
            std::cout << "[CFGv2] RETURN outside of GOSUB context" << std::endl;
        }
    }
    
    // Mark as terminator
    markTerminated(incoming);
    
    // Create unreachable block for code following RETURN
    BasicBlock* unreachableBlock = createUnreachableBlock();
    m_unreachableBlocks.push_back(unreachableBlock);
    
    return unreachableBlock;
}

BasicBlock* CFGBuilderV2::handleOnGoto(const OnGotoStatement& stmt, BasicBlock* incoming) {
    if (m_debugMode) {
        std::cout << "[CFGv2] Handling ON...GOTO with " << stmt.lineNumbers.size() 
                  << " targets" << std::endl;
    }
    
    // Add ON GOTO statement to current block
    addStatementToBlock(incoming, &stmt, getLineNumber(&stmt));
    
    // ON GOTO is NOT a terminator! If the selector is out of range, 
    // execution falls through to the next statement
    
    // Create fallthrough block for out-of-range case
    BasicBlock* fallthroughBlock = createBlock("OnGoto_Fallthrough");
    
    // Add conditional edges to all targets
    for (size_t i = 0; i < stmt.lineNumbers.size(); i++) {
        int targetLine = stmt.lineNumbers[i];
        int targetBlockId = resolveLineNumberToBlock(targetLine);
        
        if (targetBlockId >= 0) {
            // Target already exists
            addConditionalEdge(incoming->id, targetBlockId, 
                             "case_" + std::to_string(i + 1));
        } else {
            // Forward reference - defer
            DeferredEdge edge;
            edge.sourceBlockId = incoming->id;
            edge.targetLineNumber = targetLine;
            edge.label = "case_" + std::to_string(i + 1);
            m_deferredEdges.push_back(edge);
        }
    }
    
    // Add fallthrough edge for out-of-range selector
    addConditionalEdge(incoming->id, fallthroughBlock->id, "default");
    
    if (m_debugMode) {
        std::cout << "[CFGv2] ON...GOTO from block " << incoming->id 
                  << " with fallthrough to " << fallthroughBlock->id << std::endl;
    }
    
    return fallthroughBlock;
}

BasicBlock* CFGBuilderV2::handleOnGosub(const OnGosubStatement& stmt, BasicBlock* incoming,
                                       LoopContext* loop, SelectContext* select,
                                       TryContext* tryCtx, SubroutineContext* outerSub) {
    if (m_debugMode) {
        std::cout << "[CFGv2] Handling ON...GOSUB with " << stmt.lineNumbers.size() 
                  << " targets" << std::endl;
    }
    
    // Add ON GOSUB statement to current block
    addStatementToBlock(incoming, &stmt, getLineNumber(&stmt));
    
    // ON GOSUB is like multiple GOSUB calls with a selector
    // It always continues to the next statement (either after RETURN or if out of range)
    
    // Create return point block
    BasicBlock* returnBlock = createBlock("OnGosub_Return_Point");
    
    // Add call edges to all targets
    for (size_t i = 0; i < stmt.lineNumbers.size(); i++) {
        int targetLine = stmt.lineNumbers[i];
        int targetBlockId = resolveLineNumberToBlock(targetLine);
        
        if (targetBlockId >= 0) {
            // Target already exists
            addConditionalEdge(incoming->id, targetBlockId, 
                             "call_" + std::to_string(i + 1));
        } else {
            // Forward reference - defer
            DeferredEdge edge;
            edge.sourceBlockId = incoming->id;
            edge.targetLineNumber = targetLine;
            edge.label = "call_" + std::to_string(i + 1);
            m_deferredEdges.push_back(edge);
        }
    }
    
    // All paths (call + return, or out-of-range) lead to return block
    addUnconditionalEdge(incoming->id, returnBlock->id);
    
    // Create subroutine context for nested code
    SubroutineContext subCtx;
    subCtx.returnBlockId = returnBlock->id;
    subCtx.outerSub = outerSub;
    
    if (m_debugMode) {
        std::cout << "[CFGv2] ON...GOSUB from block " << incoming->id 
                  << " with return point " << returnBlock->id << std::endl;
    }
    
    return returnBlock;
}

BasicBlock* CFGBuilderV2::handleExitFor(BasicBlock* incoming, LoopContext* loop) {
    if (!loop) {
        throw std::runtime_error("EXIT FOR outside of FOR loop");
    }
    addUnconditionalEdge(incoming->id, loop->exitBlockId);
    markTerminated(incoming);
    return createUnreachableBlock();
}

BasicBlock* CFGBuilderV2::handleExitWhile(BasicBlock* incoming, LoopContext* loop) {
    // TODO: Implement
    if (!loop) {
        throw std::runtime_error("EXIT WHILE outside of WHILE loop");
    }
    addUnconditionalEdge(incoming->id, loop->exitBlockId);
    markTerminated(incoming);
    return createUnreachableBlock();
}

BasicBlock* CFGBuilderV2::handleExitDo(BasicBlock* incoming, LoopContext* loop) {
    // TODO: Implement
    if (!loop) {
        throw std::runtime_error("EXIT DO outside of DO loop");
    }
    addUnconditionalEdge(incoming->id, loop->exitBlockId);
    markTerminated(incoming);
    return createUnreachableBlock();
}

BasicBlock* CFGBuilderV2::handleExitSelect(BasicBlock* incoming, SelectContext* select) {
    // TODO: Implement
    if (!select) {
        throw std::runtime_error("EXIT SELECT outside of SELECT CASE");
    }
    addUnconditionalEdge(incoming->id, select->exitBlockId);
    markTerminated(incoming);
    return createUnreachableBlock();
}

BasicBlock* CFGBuilderV2::handleContinue(BasicBlock* incoming, LoopContext* loop) {
    // TODO: Implement
    if (!loop) {
        throw std::runtime_error("CONTINUE outside of loop");
    }
    addUnconditionalEdge(incoming->id, loop->headerBlockId);
    markTerminated(incoming);
    return createUnreachableBlock();
}

BasicBlock* CFGBuilderV2::handleEnd(const EndStatement& stmt, BasicBlock* incoming) {
    addStatementToBlock(incoming, &stmt, getLineNumber(&stmt));
    markTerminated(incoming);
    return createUnreachableBlock();
}

BasicBlock* CFGBuilderV2::handleThrow(const ThrowStatement& stmt, BasicBlock* incoming,
                                       TryContext* tryCtx) {
    // TODO: Implement
    addStatementToBlock(incoming, &stmt, getLineNumber(&stmt));
    if (tryCtx) {
        addUnconditionalEdge(incoming->id, tryCtx->catchBlockId);
    }
    markTerminated(incoming);
    return createUnreachableBlock();
}

// =============================================================================
// Loop Builders (Stubs - To be implemented)
// =============================================================================

BasicBlock* CFGBuilderV2::buildWhile(const WhileStatement& stmt, BasicBlock* incoming,
                                      LoopContext* outerLoop, SelectContext* select,
                                      TryContext* tryCtx, SubroutineContext* sub) {
    if (m_debugMode) {
        std::cout << "[CFGv2] Building WHILE loop" << std::endl;
    }
    
    // WHILE loop structure (pre-test):
    // incoming -> header [condition check]
    //             header -> body [true]
    //             header -> exit [false]
    //             body -> header [back-edge]
    //             return exit
    
    // 1. Create blocks
    BasicBlock* headerBlock = createBlock("While_Header");
    BasicBlock* bodyBlock = createBlock("While_Body");
    BasicBlock* exitBlock = createBlock("While_Exit");
    
    // 2. Wire incoming to header
    if (!isTerminated(incoming)) {
        addUnconditionalEdge(incoming->id, headerBlock->id);
    }
    
    // 3. Add condition check to header
    addStatementToBlock(headerBlock, &stmt, getLineNumber(&stmt));
    
    // 4. Wire header to body (true) and exit (false)
    addConditionalEdge(headerBlock->id, bodyBlock->id, "true");
    addConditionalEdge(headerBlock->id, exitBlock->id, "false");
    
    // 5. Create loop context for EXIT WHILE and nested loops
    LoopContext loopCtx;
    loopCtx.headerBlockId = headerBlock->id;
    loopCtx.exitBlockId = exitBlock->id;
    loopCtx.loopType = "WHILE";
    loopCtx.outerLoop = outerLoop;
    
    // 6. Recursively build loop body
    // KEY FIX: Use the body field from the AST (now pre-parsed by parser)
    // This handles nested structures automatically!
    BasicBlock* bodyExit = buildStatementRange(
        stmt.body,
        bodyBlock,
        &loopCtx,  // Pass loop context to nested statements
        select,
        tryCtx,
        sub
    );
    
    // 7. Wire body exit back to header (back-edge)
    // This is created immediately, not deferred!
    if (!isTerminated(bodyExit)) {
        addUnconditionalEdge(bodyExit->id, headerBlock->id);
    }
    
    if (m_debugMode) {
        std::cout << "[CFGv2] WHILE loop complete, exit block: " << exitBlock->id << std::endl;
    }
    
    // 8. Return exit block for next statement
    return exitBlock;
}

BasicBlock* CFGBuilderV2::buildFor(const ForStatement& stmt, BasicBlock* incoming,
                                    LoopContext* outerLoop, SelectContext* select,
                                    TryContext* tryCtx, SubroutineContext* sub) {
    if (m_debugMode) {
        std::cout << "[CFGv2] Building FOR loop" << std::endl;
    }
    
    // FOR loop structure (pre-test with initialization):
    // incoming -> init [var = start]
    //             init -> header [condition check: var <= end or var >= end]
    //             header -> body [true]
    //             header -> exit [false]
    //             body -> increment [var = var + step]
    //             increment -> header [back-edge]
    //             return exit
    
    // 1. Create blocks
    BasicBlock* initBlock = createBlock("For_Init");
    BasicBlock* headerBlock = createBlock("For_Header");
    BasicBlock* bodyBlock = createBlock("For_Body");
    BasicBlock* incrementBlock = createBlock("For_Increment");
    BasicBlock* exitBlock = createBlock("For_Exit");
    
    // 2. Wire incoming to init
    if (!isTerminated(incoming)) {
        addUnconditionalEdge(incoming->id, initBlock->id);
    }
    
    // 3. Add initialization to init block (var = start value)
    // This represents: FOR i = 1 TO 10
    addStatementToBlock(initBlock, &stmt, getLineNumber(&stmt));
    
    // 4. Wire init to header
    addUnconditionalEdge(initBlock->id, headerBlock->id);
    
    // 5. Header contains the loop condition check (i <= 10 or i >= 10 depending on STEP)
    // Wire header to body (true) and exit (false)
    addConditionalEdge(headerBlock->id, bodyBlock->id, "true");
    addConditionalEdge(headerBlock->id, exitBlock->id, "false");
    
    // 6. Create loop context for EXIT FOR and nested loops
    LoopContext loopCtx;
    loopCtx.headerBlockId = headerBlock->id;
    loopCtx.exitBlockId = exitBlock->id;
    loopCtx.loopType = "FOR";
    loopCtx.outerLoop = outerLoop;
    
    // 7. Recursively build loop body
    BasicBlock* bodyExit = buildStatementRange(
        stmt.body,
        bodyBlock,
        &loopCtx,
        select,
        tryCtx,
        sub
    );
    
    // 8. Wire body exit to increment block (if not terminated)
    if (!isTerminated(bodyExit)) {
        addUnconditionalEdge(bodyExit->id, incrementBlock->id);
    }
    
    // 9. Increment block contains: var = var + STEP
    // Then wire back to header
    addUnconditionalEdge(incrementBlock->id, headerBlock->id);
    
    if (m_debugMode) {
        std::cout << "[CFGv2] FOR loop complete, exit block: " << exitBlock->id << std::endl;
    }
    
    // 10. Return exit block for next statement
    return exitBlock;
}

BasicBlock* CFGBuilderV2::buildRepeat(const RepeatStatement& stmt, BasicBlock* incoming,
                                       LoopContext* outerLoop, SelectContext* select,
                                       TryContext* tryCtx, SubroutineContext* sub) {
    if (m_debugMode) {
        std::cout << "[CFGv2] Building REPEAT loop (post-test)" << std::endl;
    }
    
    // REPEAT...UNTIL loop structure (post-test):
    // incoming -> body
    //             body -> condition [check at end]
    //             condition -> body [false - continue looping]
    //             condition -> exit [true - condition met]
    //             return exit
    //
    // KEY DIFFERENCE FROM WHILE: Body executes at least once!
    
    // 1. Create blocks
    BasicBlock* bodyBlock = createBlock("Repeat_Body");
    BasicBlock* conditionBlock = createBlock("Repeat_Condition");
    BasicBlock* exitBlock = createBlock("Repeat_Exit");
    
    // 2. Wire incoming to body (executes at least once)
    if (!isTerminated(incoming)) {
        addUnconditionalEdge(incoming->id, bodyBlock->id);
    }
    
    // 3. Create loop context for EXIT and nested loops
    // Use condition block as "header" for CONTINUE-like semantics
    LoopContext loopCtx;
    loopCtx.headerBlockId = conditionBlock->id;
    loopCtx.exitBlockId = exitBlock->id;
    loopCtx.loopType = "REPEAT";
    loopCtx.outerLoop = outerLoop;
    
    // 4. Recursively build loop body (now using body field from AST)
    BasicBlock* bodyExit = buildStatementRange(
        stmt.body,
        bodyBlock,
        &loopCtx,
        select,
        tryCtx,
        sub
    );
    
    // 5. Wire body exit to condition block (if not terminated)
    if (!isTerminated(bodyExit)) {
        addUnconditionalEdge(bodyExit->id, conditionBlock->id);
    }
    
    // 6. Add UNTIL condition check to condition block
    // The condition is now stored in RepeatStatement.condition (moved from UntilStatement)
    addStatementToBlock(conditionBlock, &stmt, getLineNumber(&stmt));
    
    // 7. Wire condition to exit (true) and back to body (false)
    // UNTIL means: exit when condition is TRUE
    addConditionalEdge(conditionBlock->id, exitBlock->id, "true");
    addConditionalEdge(conditionBlock->id, bodyBlock->id, "false");
    
    if (m_debugMode) {
        std::cout << "[CFGv2] REPEAT loop complete, exit block: " << exitBlock->id << std::endl;
    }
    
    // 8. Return exit block for next statement
    return exitBlock;
}

BasicBlock* CFGBuilderV2::buildDo(const DoStatement& stmt, BasicBlock* incoming,
                                   LoopContext* outerLoop, SelectContext* select,
                                   TryContext* tryCtx, SubroutineContext* sub) {
    if (m_debugMode) {
        std::cout << "[CFGv2] Building DO loop" << std::endl;
    }
    
    // DO loop has multiple variants:
    // 1. DO WHILE condition ... LOOP (pre-test, continue while true)
    // 2. DO UNTIL condition ... LOOP (pre-test, continue until true)
    // 3. DO ... LOOP WHILE condition (post-test, continue while true)
    // 4. DO ... LOOP UNTIL condition (post-test, continue until true)
    // 5. DO ... LOOP (infinite loop, needs EXIT DO)
    
    // Determine loop variant from AST (updated field names)
    bool hasPreCondition = (stmt.preConditionType != DoStatement::ConditionType::NONE);
    bool hasPostCondition = (stmt.postConditionType != DoStatement::ConditionType::NONE);
    bool isPreWhile = (stmt.preConditionType == DoStatement::ConditionType::WHILE);
    bool isPostWhile = (stmt.postConditionType == DoStatement::ConditionType::WHILE);
    
    if (hasPreCondition) {
        // Pre-test variant (like WHILE)
        // incoming -> header [condition check]
        //             header -> body [condition met]
        //             header -> exit [condition not met]
        //             body -> header [back-edge]
        
        BasicBlock* headerBlock = createBlock("Do_Header");
        BasicBlock* bodyBlock = createBlock("Do_Body");
        BasicBlock* exitBlock = createBlock("Do_Exit");
        
        if (!isTerminated(incoming)) {
            addUnconditionalEdge(incoming->id, headerBlock->id);
        }
        
        addStatementToBlock(headerBlock, &stmt, getLineNumber(&stmt));
        
        // Wire header to body and exit based on WHILE vs UNTIL
        if (isPreWhile) {
            // DO WHILE: continue when true
            addConditionalEdge(headerBlock->id, bodyBlock->id, "true");
            addConditionalEdge(headerBlock->id, exitBlock->id, "false");
        } else {
            // DO UNTIL: continue when false
            addConditionalEdge(headerBlock->id, bodyBlock->id, "false");
            addConditionalEdge(headerBlock->id, exitBlock->id, "true");
        }
        
        LoopContext loopCtx;
        loopCtx.headerBlockId = headerBlock->id;
        loopCtx.exitBlockId = exitBlock->id;
        loopCtx.loopType = "DO";
        loopCtx.outerLoop = outerLoop;
        
        BasicBlock* bodyExit = buildStatementRange(
            stmt.body,
            bodyBlock,
            &loopCtx,
            select,
            tryCtx,
            sub
        );
        
        if (!isTerminated(bodyExit)) {
            addUnconditionalEdge(bodyExit->id, headerBlock->id);
        }
        
        if (m_debugMode) {
            std::cout << "[CFGv2] DO (pre-test) loop complete, exit block: " << exitBlock->id << std::endl;
        }
        
        return exitBlock;
        
    } else if (hasPostCondition) {
        // Post-test variant (like REPEAT)
        // incoming -> body
        //             body -> condition [check at end]
        //             condition -> body [condition met]
        //             condition -> exit [condition not met]
        
        BasicBlock* bodyBlock = createBlock("Do_Body");
        BasicBlock* conditionBlock = createBlock("Do_Condition");
        BasicBlock* exitBlock = createBlock("Do_Exit");
        
        if (!isTerminated(incoming)) {
            addUnconditionalEdge(incoming->id, bodyBlock->id);
        }
        
        LoopContext loopCtx;
        loopCtx.headerBlockId = conditionBlock->id;
        loopCtx.exitBlockId = exitBlock->id;
        loopCtx.loopType = "DO";
        loopCtx.outerLoop = outerLoop;
        
        BasicBlock* bodyExit = buildStatementRange(
            stmt.body,
            bodyBlock,
            &loopCtx,
            select,
            tryCtx,
            sub
        );
        
        if (!isTerminated(bodyExit)) {
            addUnconditionalEdge(bodyExit->id, conditionBlock->id);
        }
        
        addStatementToBlock(conditionBlock, &stmt, getLineNumber(&stmt));
        
        // Wire condition based on WHILE vs UNTIL
        if (isPostWhile) {
            // LOOP WHILE: continue when true
            addConditionalEdge(conditionBlock->id, bodyBlock->id, "true");
            addConditionalEdge(conditionBlock->id, exitBlock->id, "false");
        } else {
            // LOOP UNTIL: continue when false (exit when true)
            addConditionalEdge(conditionBlock->id, exitBlock->id, "true");
            addConditionalEdge(conditionBlock->id, bodyBlock->id, "false");
        }
        
        if (m_debugMode) {
            std::cout << "[CFGv2] DO (post-test) loop complete, exit block: " << exitBlock->id << std::endl;
        }
        
        return exitBlock;
        
    } else {
        // Infinite loop variant: DO ... LOOP (no condition)
        // incoming -> body
        //             body -> body [back-edge]
        //             exit block is created but only reachable via EXIT DO
        
        BasicBlock* bodyBlock = createBlock("Do_Body");
        BasicBlock* exitBlock = createBlock("Do_Exit");
        
        if (!isTerminated(incoming)) {
            addUnconditionalEdge(incoming->id, bodyBlock->id);
        }
        
        LoopContext loopCtx;
        loopCtx.headerBlockId = bodyBlock->id;
        loopCtx.exitBlockId = exitBlock->id;
        loopCtx.loopType = "DO";
        loopCtx.outerLoop = outerLoop;
        
        BasicBlock* bodyExit = buildStatementRange(
            stmt.body,
            bodyBlock,
            &loopCtx,
            select,
            tryCtx,
            sub
        );
        
        if (!isTerminated(bodyExit)) {
            // Infinite loop: back-edge to body
            addUnconditionalEdge(bodyExit->id, bodyBlock->id);
        }
        
        if (m_debugMode) {
            std::cout << "[CFGv2] DO (infinite) loop complete, exit block: " << exitBlock->id << std::endl;
        }
        
        // Exit block is only reachable via EXIT DO
        return exitBlock;
    }
}

BasicBlock* CFGBuilderV2::buildSelectCase(const SelectCaseStatement& stmt, BasicBlock* incoming,
                                           LoopContext* loop, SelectContext* outerSelect,
                                           TryContext* tryCtx, SubroutineContext* sub) {
    // TODO: Implement full SELECT CASE building
    addStatementToBlock(incoming, &stmt, getLineNumber(&stmt));
    return incoming;
}

BasicBlock* CFGBuilderV2::buildTryCatch(const TryStatement& stmt, BasicBlock* incoming,
                                         LoopContext* loop, SelectContext* select,
                                         TryContext* outerTry, SubroutineContext* sub) {
    // TODO: Implement full TRY/CATCH building
    addStatementToBlock(incoming, &stmt, getLineNumber(&stmt));
    return incoming;
}

} // namespace FasterBASIC