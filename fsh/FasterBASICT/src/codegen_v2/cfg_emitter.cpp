#include "cfg_emitter.h"
#include <algorithm>
#include <queue>

namespace fbc {

using namespace FasterBASIC;

CFGEmitter::CFGEmitter(QBEBuilder& builder, TypeManager& typeManager,
                       SymbolMapper& symbolMapper, ASTEmitter& astEmitter)
    : builder_(builder)
    , typeManager_(typeManager)
    , symbolMapper_(symbolMapper)
    , astEmitter_(astEmitter)
    , currentFunction_("")
{
}

// === CFG Emission ===

void CFGEmitter::emitCFG(const ControlFlowGraph* cfg, const std::string& functionName) {
    if (!cfg) {
        builder_.emitComment("ERROR: null CFG");
        return;
    }
    
    enterFunction(functionName);
    
    builder_.emitComment("CFG: " + (functionName.empty() ? "main" : functionName));
    builder_.emitComment("Blocks: " + std::to_string(cfg->blocks.size()));
    builder_.emitBlankLine();
    
    // Compute reachability
    computeReachability(cfg);
    
    // Get block emission order
    std::vector<int> emissionOrder = getEmissionOrder(cfg);
    
    builder_.emitComment("Emission order computed: " + std::to_string(emissionOrder.size()) + " blocks");
    
    // Emit all blocks in order
    for (int blockId : emissionOrder) {
        if (blockId >= 0 && blockId < static_cast<int>(cfg->blocks.size())) {
            const BasicBlock* block = cfg->blocks[blockId].get();
            if (block) {
                emitBlock(block, cfg);
            }
        }
    }
    
    exitFunction();
}

void CFGEmitter::emitBlock(const BasicBlock* block, const ControlFlowGraph* cfg) {
    if (!block) {
        builder_.emitComment("ERROR: null block");
        return;
    }
    
    int blockId = block->id;
    
    // Emit label for this block
    std::string label = getBlockLabel(blockId);
    
    // Add comment about block type
    std::string blockInfo = "Block " + std::to_string(blockId);
    if (!block->label.empty()) {
        blockInfo += " (label: " + block->label + ")";
    }
    
    builder_.emitComment(blockInfo);
    builder_.emitLabel(label);
    
    // Check if this is a FOR loop header - emit condition check
    if (block->isLoopHeader && block->label.find("For_Header") != std::string::npos) {
        // Find the ForStatement in the predecessor init block
        const ForStatement* forStmt = findForStatementForHeader(block, cfg);
        if (forStmt) {
            std::string condition = astEmitter_.emitForCondition(forStmt);
            // Store condition for use by terminator
            currentLoopCondition_ = condition;
        }
    }
    
    // Check if this is a WHILE loop header - emit condition check
    if (block->isLoopHeader && block->label.find("While_Header") != std::string::npos) {
        // The WhileStatement is in this block
        for (const Statement* stmt : block->statements) {
            if (stmt && stmt->getType() == ASTNodeType::STMT_WHILE) {
                const WhileStatement* whileStmt = static_cast<const WhileStatement*>(stmt);
                std::string condition = astEmitter_.emitWhileCondition(whileStmt);
                // Store condition for use by terminator
                currentLoopCondition_ = condition;
                break;
            }
        }
    }
    
    // Check if this is a FOR loop increment block - emit increment
    if (block->label.find("For_Increment") != std::string::npos) {
        // Find the ForStatement by searching predecessors
        const ForStatement* forStmt = findForStatementInLoop(block, cfg);
        if (forStmt) {
            astEmitter_.emitForIncrement(forStmt);
        }
    }
    
    // Emit statements in this block
    emitBlockStatements(block);
    
    // Emit terminator (control flow)
    emitBlockTerminator(block, cfg);
    
    builder_.emitBlankLine();
    
    // Mark label as emitted
    emittedLabels_.insert(blockId);
}

// === Edge Handling ===

void CFGEmitter::emitBlockTerminator(const BasicBlock* block, const ControlFlowGraph* cfg) {
    std::vector<CFGEdge> outEdges = getOutEdges(block, cfg);
    
    if (outEdges.empty()) {
        // No out-edges - this is an exit block
        // If we're in main, just return 0; otherwise, error
        if (currentFunction_.empty() || currentFunction_ == "main") {
            builder_.emitComment("Implicit return 0");
            builder_.emitReturn("0");
        } else {
            builder_.emitComment("WARNING: block with no out-edges (missing return?)");
            builder_.emitReturn();
        }
        return;
    }
    
    // Analyze edge types
    EdgeType edgeType = outEdges[0].type;
    
    if (edgeType == EdgeType::RETURN) {
        // Return edge
        builder_.emitComment("Return edge");
        emitReturn();
        return;
    }
    
    if (edgeType == EdgeType::FALLTHROUGH || edgeType == EdgeType::JUMP) {
        // Simple fallthrough/jump - unconditional jump
        if (outEdges.size() == 1) {
            builder_.emitComment(edgeType == EdgeType::FALLTHROUGH ? "Fallthrough edge" : "Jump edge");
            emitFallthrough(outEdges[0].targetBlock);
        } else {
            builder_.emitComment("ERROR: multiple FALLTHROUGH edges");
            emitFallthrough(outEdges[0].targetBlock);
        }
        return;
    }
    
    if (edgeType == EdgeType::CONDITIONAL_TRUE || edgeType == EdgeType::CONDITIONAL_FALSE) {
        // Conditional branch (IF, WHILE, etc.)
        if (outEdges.size() == 2) {
            builder_.emitComment("Conditional edge");
            
            // Check if we have a stored loop condition first (for FOR/WHILE headers)
            std::string condition;
            if (!currentLoopCondition_.empty()) {
                condition = currentLoopCondition_;
                currentLoopCondition_.clear();  // Clear after use
            } else if (!block->statements.empty()) {
                // Find the last statement - should be an IF or loop condition
                const Statement* lastStmt = block->statements.back();
                if (lastStmt && lastStmt->getType() == ASTNodeType::STMT_IF) {
                    const IfStatement* ifStmt = static_cast<const IfStatement*>(lastStmt);
                    condition = astEmitter_.emitIfCondition(ifStmt);
                } else {
                    // Generic condition - assume it's already evaluated
                    builder_.emitComment("WARNING: conditional without IF statement");
                    condition = "1";  // Default to true
                }
            } else {
                condition = "1";  // Default to true
            }
            
            // Determine which edge is true and which is false
            int trueTarget = -1;
            int falseTarget = -1;
            
            for (const auto& edge : outEdges) {
                if (edge.type == EdgeType::CONDITIONAL_TRUE) {
                    trueTarget = edge.targetBlock;
                } else if (edge.type == EdgeType::CONDITIONAL_FALSE) {
                    falseTarget = edge.targetBlock;
                }
            }
            
            // If not explicitly labeled, use order
            if (trueTarget == -1) trueTarget = outEdges[0].targetBlock;
            if (falseTarget == -1) falseTarget = outEdges[1].targetBlock;
            
            emitConditional(condition, trueTarget, falseTarget);
        } else {
            builder_.emitComment("ERROR: conditional with != 2 edges");
            if (!outEdges.empty()) {
                emitFallthrough(outEdges[0].targetBlock);
            }
        }
        return;
    }
    
    if (edgeType == EdgeType::CALL) {
        // Subroutine call (GOSUB)
        builder_.emitComment("GOSUB call edge");
        
        if (outEdges.size() >= 1) {
            // First edge is the call target
            int callTarget = outEdges[0].targetBlock;
            
            // TODO: Emit proper GOSUB calling convention
            // For now, just jump (we'll need runtime stack management)
            builder_.emitComment("TODO: Implement GOSUB call stack");
            emitFallthrough(callTarget);
        }
        return;
    }
    
    if (edgeType == EdgeType::EXCEPTION) {
        // Exception handling edge
        builder_.emitComment("Exception edge");
        if (!outEdges.empty()) {
            emitFallthrough(outEdges[0].targetBlock);
        }
        return;
    }
    
    // Multiple edges without clear type - treat as multiway
    if (outEdges.size() > 2) {
        builder_.emitComment("Multiway edge (" + std::to_string(outEdges.size()) + " targets)");
        
        std::vector<int> targets;
        int defaultTarget = -1;
        
        for (const auto& edge : outEdges) {
            if (edge.label == "default" || edge.label == "otherwise") {
                defaultTarget = edge.targetBlock;
            } else {
                targets.push_back(edge.targetBlock);
            }
        }
        
        // If no default, use the last target
        if (defaultTarget == -1 && !targets.empty()) {
            defaultTarget = targets.back();
        }
        
        // TODO: Get selector value from statement
        std::string selector = "1";  // Placeholder
        
        emitMultiway(selector, targets, defaultTarget);
        return;
    }
    
    // Unknown edge type - fallthrough to first edge
    builder_.emitComment("WARNING: unknown edge type, using fallthrough");
    if (!outEdges.empty()) {
        emitFallthrough(outEdges[0].targetBlock);
    }
}

void CFGEmitter::emitFallthrough(int targetBlockId) {
    std::string targetLabel = getBlockLabel(targetBlockId);
    builder_.emitJump(targetLabel);
}

void CFGEmitter::emitConditional(const std::string& condition,
                                int trueBlockId, int falseBlockId) {
    std::string trueLabel = getBlockLabel(trueBlockId);
    std::string falseLabel = getBlockLabel(falseBlockId);
    builder_.emitBranch(condition, trueLabel, falseLabel);
}

void CFGEmitter::emitMultiway(const std::string& selector,
                             const std::vector<int>& targetBlockIds,
                             int defaultBlockId) {
    // Emit a switch-like structure using conditional jumps
    builder_.emitComment("Multiway dispatch");
    
    std::string defaultLabel = getBlockLabel(defaultBlockId);
    
    for (size_t i = 0; i < targetBlockIds.size(); ++i) {
        std::string caseValue = std::to_string(i + 1);
        std::string targetLabel = getBlockLabel(targetBlockIds[i]);
        
        // Compare selector with case value
        std::string cmpResult = builder_.newTemp();
        builder_.emitCompare(cmpResult, "w", "eq", selector, caseValue);
        
        // If match, jump to target; otherwise continue
        std::string nextCaseLabel = symbolMapper_.getUniqueLabel("case_next");
        builder_.emitBranch(cmpResult, targetLabel, nextCaseLabel);
        builder_.emitLabel(nextCaseLabel);
    }
    
    // No match - jump to default
    builder_.emitJump(defaultLabel);
}

void CFGEmitter::emitReturn(const std::string& returnValue) {
    builder_.emitReturn(returnValue);
}

// === Block Ordering ===

std::vector<int> CFGEmitter::getEmissionOrder(const ControlFlowGraph* cfg) {
    std::vector<int> order;
    
    if (!cfg || cfg->blocks.empty()) {
        return order;
    }
    
    // Simple strategy: emit in block ID order
    // This ensures we emit all blocks, including UNREACHABLE ones
    // (needed for GOSUB/ON GOTO targets)
    for (size_t i = 0; i < cfg->blocks.size(); ++i) {
        if (cfg->blocks[i]) {
            order.push_back(cfg->blocks[i]->id);
        }
    }
    
    return order;
}

bool CFGEmitter::isBlockReachable(int blockId, const ControlFlowGraph* cfg) {
    if (reachabilityCache_.find(blockId) != reachabilityCache_.end()) {
        return reachabilityCache_[blockId];
    }
    
    // If not in cache, assume reachable (conservative)
    return true;
}

// === Label Management ===

std::string CFGEmitter::getBlockLabel(int blockId) {
    return symbolMapper_.getBlockLabel(blockId);
}

void CFGEmitter::registerLabel(int blockId) {
    requiredLabels_.insert(blockId);
}

bool CFGEmitter::isLabelEmitted(int blockId) {
    return emittedLabels_.find(blockId) != emittedLabels_.end();
}

// === Special Block Types ===

bool CFGEmitter::isLoopHeader(const BasicBlock* block, const ControlFlowGraph* cfg) {
    if (!block) return false;
    return block->isLoopHeader;
}

const ForStatement* CFGEmitter::findForStatementInLoop(const BasicBlock* block, const ControlFlowGraph* cfg) {
    if (!block || !cfg) return nullptr;
    
    // Search backwards through predecessors to find the init block
    std::set<int> visited;
    std::queue<int> toVisit;
    toVisit.push(block->id);
    
    while (!toVisit.empty()) {
        int currentId = toVisit.front();
        toVisit.pop();
        
        if (visited.count(currentId)) continue;
        visited.insert(currentId);
        
        if (currentId >= 0 && currentId < static_cast<int>(cfg->blocks.size())) {
            const BasicBlock* currentBlock = cfg->blocks[currentId].get();
            if (currentBlock) {
                // Check if this block has a ForStatement
                for (const Statement* stmt : currentBlock->statements) {
                    if (stmt && stmt->getType() == ASTNodeType::STMT_FOR) {
                        return static_cast<const ForStatement*>(stmt);
                    }
                }
                
                // Add predecessors to search
                for (int predId : currentBlock->predecessors) {
                    if (!visited.count(predId)) {
                        toVisit.push(predId);
                    }
                }
            }
        }
    }
    
    return nullptr;
}

bool CFGEmitter::isExitBlock(const BasicBlock* block, const ControlFlowGraph* cfg) {
    if (!block) return false;
    
    // Check if block has no successors
    std::vector<CFGEdge> outEdges = getOutEdges(block, cfg);
    
    if (outEdges.empty()) {
        return true;
    }
    
    // Check if all edges are RETURN edges
    for (const auto& edge : outEdges) {
        if (edge.type != EdgeType::RETURN) {
            return false;
        }
    }
    
    return true;
}

// === Context Management ===

void CFGEmitter::enterFunction(const std::string& functionName) {
    currentFunction_ = functionName;
    emittedLabels_.clear();
    requiredLabels_.clear();
    reachabilityCache_.clear();
}

void CFGEmitter::exitFunction() {
    currentFunction_.clear();
}

void CFGEmitter::reset() {
    currentFunction_.clear();
    emittedLabels_.clear();
    requiredLabels_.clear();
    reachabilityCache_.clear();
}

// === Helper Methods ===

void CFGEmitter::emitBlockStatements(const BasicBlock* block) {
    if (!block) return;
    
    for (const Statement* stmt : block->statements) {
        if (stmt) {
            astEmitter_.emitStatement(stmt);
        }
    }
}

const ForStatement* CFGEmitter::findForStatementForHeader(const BasicBlock* headerBlock, const ControlFlowGraph* cfg) {
    if (!headerBlock || !cfg) return nullptr;
    
    // The FOR statement is in the init block (predecessor of header)
    for (int predId : headerBlock->predecessors) {
        if (predId >= 0 && predId < static_cast<int>(cfg->blocks.size())) {
            const BasicBlock* predBlock = cfg->blocks[predId].get();
            if (predBlock && predBlock->label.find("For_Init") != std::string::npos) {
                // Found the init block, look for ForStatement
                for (const Statement* stmt : predBlock->statements) {
                    if (stmt && stmt->getType() == ASTNodeType::STMT_FOR) {
                        return static_cast<const ForStatement*>(stmt);
                    }
                }
            }
        }
    }
    return nullptr;
}

std::vector<CFGEdge> CFGEmitter::getOutEdges(const BasicBlock* block, 
                                              const ControlFlowGraph* cfg) {
    std::vector<CFGEdge> result;
    
    if (!block || !cfg) {
        return result;
    }
    
    // Find all edges where sourceBlock == block->id
    for (const auto& edge : cfg->edges) {
        if (edge.sourceBlock == block->id) {
            result.push_back(edge);
        }
    }
    
    return result;
}

void CFGEmitter::computeReachability(const ControlFlowGraph* cfg) {
    if (!cfg) return;
    
    reachabilityCache_.clear();
    
    // Mark all blocks as unreachable initially
    for (const auto& block : cfg->blocks) {
        if (block) {
            reachabilityCache_[block->id] = false;
        }
    }
    
    // DFS from entry block
    std::unordered_set<int> visited;
    dfsReachability(cfg->entryBlock, cfg, visited);
}

void CFGEmitter::dfsReachability(int blockId, 
                                 const ControlFlowGraph* cfg,
                                 std::unordered_set<int>& visited) {
    if (visited.find(blockId) != visited.end()) {
        return;  // Already visited
    }
    
    visited.insert(blockId);
    reachabilityCache_[blockId] = true;
    
    // Find the block
    const BasicBlock* block = nullptr;
    if (blockId >= 0 && blockId < static_cast<int>(cfg->blocks.size())) {
        block = cfg->blocks[blockId].get();
    }
    
    if (!block) return;
    
    // Visit all successors
    std::vector<CFGEdge> outEdges = getOutEdges(block, cfg);
    for (const auto& edge : outEdges) {
        dfsReachability(edge.targetBlock, cfg, visited);
    }
}

std::string CFGEmitter::getEdgeTypeName(EdgeType edgeType) {
    switch (edgeType) {
        case EdgeType::FALLTHROUGH: return "FALLTHROUGH";
        case EdgeType::CONDITIONAL_TRUE: return "CONDITIONAL_TRUE";
        case EdgeType::CONDITIONAL_FALSE: return "CONDITIONAL_FALSE";
        case EdgeType::JUMP: return "JUMP";
        case EdgeType::CALL: return "CALL";
        case EdgeType::RETURN: return "RETURN";
        case EdgeType::EXCEPTION: return "EXCEPTION";
        default: return "UNKNOWN";
    }
}

} // namespace fbc