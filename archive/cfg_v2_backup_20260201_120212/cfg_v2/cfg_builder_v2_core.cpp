//
// cfg_builder_v2_core.cpp
// FasterBASIC - Control Flow Graph Builder V2 Core Implementation
//
// ARCHITECTURAL REDESIGN (February 2025)
//
// This file contains the core functionality:
// - Constructor/destructor
// - Main entry points (build, buildFromProgram)
// - Block and edge management
// - Label and line number resolution
// - Helper functions
// - CFG dump functionality
//

#include "cfg_builder_v2.h"
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <algorithm>

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
                
                case ASTNodeType::STMT_SELECT_CASE:
                    currentBlock = buildSelectCase(
                        static_cast<const SelectCaseStatement&>(*stmt),
                        currentBlock,
                        nullptr, nullptr, nullptr, nullptr
                    );
                    break;
                
                case ASTNodeType::STMT_TRY:
                    currentBlock = buildTryCatch(
                        static_cast<const TryStatement&>(*stmt),
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
            for (int target : onGotoStmt->targetLines) {
                m_jumpTargets.insert(target);
            }
            break;
        }
        
        case ASTNodeType::STMT_ON_GOSUB: {
            const auto* onGosubStmt = static_cast<const OnGosubStatement*>(stmt);
            for (int target : onGosubStmt->targetLines) {
                m_jumpTargets.insert(target);
            }
            break;
        }
        
        // Recursively scan control structures
        case ASTNodeType::STMT_IF: {
            const auto* ifStmt = static_cast<const IfStatement*>(stmt);
            for (const auto& s : ifStmt->thenBlock) {
                collectJumpTargetsFromStatement(s.get());
            }
            for (const auto& s : ifStmt->elseBlock) {
                collectJumpTargetsFromStatement(s.get());
            }
            break;
        }
        
        case ASTNodeType::STMT_WHILE: {
            const auto* whileStmt = static_cast<const WhileStatement*>(stmt);
            for (const auto& s : whileStmt->body) {
                collectJumpTargetsFromStatement(s.get());
            }
            break;
        }
        
        case ASTNodeType::STMT_FOR: {
            const auto* forStmt = static_cast<const ForStatement*>(stmt);
            for (const auto& s : forStmt->body) {
                collectJumpTargetsFromStatement(s.get());
            }
            break;
        }
        
        case ASTNodeType::STMT_REPEAT: {
            const auto* repeatStmt = static_cast<const RepeatStatement*>(stmt);
            for (const auto& s : repeatStmt->body) {
                collectJumpTargetsFromStatement(s.get());
            }
            break;
        }
        
        case ASTNodeType::STMT_DO: {
            const auto* doStmt = static_cast<const DoStatement*>(stmt);
            for (const auto& s : doStmt->body) {
                collectJumpTargetsFromStatement(s.get());
            }
            break;
        }
        
        case ASTNodeType::STMT_SELECT_CASE: {
            const auto* selectStmt = static_cast<const SelectCaseStatement*>(stmt);
            for (const auto& caseClause : selectStmt->cases) {
                for (const auto& s : caseClause->statements) {
                    collectJumpTargetsFromStatement(s.get());
                }
            }
            for (const auto& s : selectStmt->otherwiseStatements) {
                collectJumpTargetsFromStatement(s.get());
            }
            break;
        }
        
        case ASTNodeType::STMT_TRY: {
            const auto* tryStmt = static_cast<const TryStatement*>(stmt);
            for (const auto& s : tryStmt->tryBlock) {
                collectJumpTargetsFromStatement(s.get());
            }
            for (const auto& s : tryStmt->catchBlock) {
                collectJumpTargetsFromStatement(s.get());
            }
            for (const auto& s : tryStmt->finallyBlock) {
                collectJumpTargetsFromStatement(s.get());
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
// Block and Edge Management
// =============================================================================

BasicBlock* CFGBuilderV2::createBlock(const std::string& label) {
    BasicBlock* block = new BasicBlock();
    block->id = m_nextBlockId++;
    block->label = label.empty() ? ("BB" + std::to_string(block->id)) : label;
    
    m_cfg->blocks.push_back(block);
    m_totalBlocksCreated++;
    
    if (m_debugMode) {
        std::cout << "[CFGv2] Created block " << block->id << " (" << block->label << ")" << std::endl;
    }
    
    return block;
}

BasicBlock* CFGBuilderV2::createUnreachableBlock() {
    return createBlock("Unreachable");
}

void CFGBuilderV2::addEdge(int fromBlockId, int toBlockId, const std::string& label) {
    // Find blocks
    BasicBlock* fromBlock = nullptr;
    BasicBlock* toBlock = nullptr;
    
    for (auto* block : m_cfg->blocks) {
        if (block->id == fromBlockId) fromBlock = block;
        if (block->id == toBlockId) toBlock = block;
    }
    
    if (!fromBlock || !toBlock) {
        if (m_debugMode) {
            std::cerr << "[CFGv2] WARNING: Cannot add edge " << fromBlockId 
                     << " -> " << toBlockId << " (block not found)" << std::endl;
        }
        return;
    }
    
    // Add edge
    CFGEdge edge;
    edge.from = fromBlockId;
    edge.to = toBlockId;
    edge.label = label;
    
    m_cfg->edges.push_back(edge);
    
    // Update predecessor/successor lists
    fromBlock->successors.push_back(toBlockId);
    toBlock->predecessors.push_back(fromBlockId);
    
    m_totalEdgesCreated++;
    
    if (m_debugMode) {
        std::cout << "[CFGv2] Added edge: " << fromBlockId << " -> " << toBlockId;
        if (!label.empty()) {
            std::cout << " (" << label << ")";
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
        block->terminated = true;
    }
}

bool CFGBuilderV2::isTerminated(const BasicBlock* block) const {
    return block && block->terminated;
}

// =============================================================================
// Statement Management
// =============================================================================

void CFGBuilderV2::addStatementToBlock(BasicBlock* block, const Statement* stmt, int lineNumber) {
    if (!block || !stmt) return;
    
    // Add statement to block (copy it)
    block->statements.push_back(StatementPtr(const_cast<Statement*>(stmt)->clone()));
    
    // Track line number mapping
    if (lineNumber > 0) {
        m_cfg->lineToBlock[lineNumber] = block->id;
    }
}

int CFGBuilderV2::getLineNumber(const Statement* stmt) {
    // TODO: Extract line number from statement metadata
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
    return -1;  // Not found
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
    return -1;  // Not found
}

void CFGBuilderV2::resolveDeferredEdges() {
    for (const auto& deferred : m_deferredEdges) {
        int targetBlockId = resolveLineNumberToBlock(deferred.targetLineNumber);
        if (targetBlockId >= 0) {
            addEdge(deferred.sourceBlockId, targetBlockId, deferred.label);
        } else {
            std::cerr << "[CFGv2] ERROR: Cannot resolve deferred edge to line " 
                     << deferred.targetLineNumber << std::endl;
        }
    }
    m_deferredEdges.clear();
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
    if (!block || block->statements.empty()) {
        return block;
    }
    
    // Create new block and move statements
    BasicBlock* newBlock = createBlock("Split");
    newBlock->statements = std::move(block->statements);
    block->statements.clear();
    
    return newBlock;
}

void CFGBuilderV2::dumpCFG(const std::string& phase) const {
    if (!m_cfg) return;
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "CFG DUMP";
    if (!phase.empty()) {
        std::cout << " (" << phase << ")";
    }
    std::cout << "\n========================================" << std::endl;
    
    // Statistics
    std::cout << "\nStatistics:" << std::endl;
    std::cout << "  Blocks: " << m_cfg->blocks.size() << std::endl;
    std::cout << "  Edges: " << m_cfg->edges.size() << std::endl;
    std::cout << "  Entry: " << m_cfg->entryBlock << std::endl;
    std::cout << "  Exit: " << m_cfg->exitBlock << std::endl;
    
    // Blocks
    std::cout << "\nBlocks:" << std::endl;
    for (const auto* block : m_cfg->blocks) {
        std::cout << "  Block " << block->id << " (" << block->label << ")";
        if (block->terminated) {
            std::cout << " [TERMINATED]";
        }
        std::cout << std::endl;
        
        std::cout << "    Predecessors: ";
        if (block->predecessors.empty()) {
            std::cout << "(none)";
        } else {
            for (size_t i = 0; i < block->predecessors.size(); i++) {
                if (i > 0) std::cout << ", ";
                std::cout << block->predecessors[i];
            }
        }
        std::cout << std::endl;
        
        std::cout << "    Successors: ";
        if (block->successors.empty()) {
            std::cout << "(none)";
        } else {
            for (size_t i = 0; i < block->successors.size(); i++) {
                if (i > 0) std::cout << ", ";
                std::cout << block->successors[i];
            }
        }
        std::cout << std::endl;
        
        if (!block->statements.empty()) {
            std::cout << "    Statements: " << block->statements.size() << std::endl;
        }
    }
    
    // Edges
    std::cout << "\nEdges:" << std::endl;
    for (const auto& edge : m_cfg->edges) {
        std::cout << "  " << edge.from << " -> " << edge.to;
        if (!edge.label.empty()) {
            std::cout << " [" << edge.label << "]";
        }
        std::cout << std::endl;
    }
    
    // Line mappings
    if (!m_cfg->lineToBlock.empty()) {
        std::cout << "\nLine -> Block mappings:" << std::endl;
        
        // Sort by line number for easier reading
        std::vector<std::pair<int, int>> sorted(
            m_cfg->lineToBlock.begin(), 
            m_cfg->lineToBlock.end()
        );
        std::sort(sorted.begin(), sorted.end());
        
        for (const auto& pair : sorted) {
            std::cout << "  Line " << pair.first << " -> Block " << pair.second << std::endl;
        }
    }
    
    std::cout << "\n========================================\n" << std::endl;
}

} // namespace FasterBASIC