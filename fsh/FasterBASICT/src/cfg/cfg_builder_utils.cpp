//
// cfg_builder_utils.cpp
// FasterBASIC - Control Flow Graph Builder Utilities (V2)
//
// Contains utility functions like dumpCFG for debugging.
// Part of modular CFG builder split (February 2026).
//
// V2 IMPLEMENTATION: Single-pass recursive construction with immediate edge wiring
//

#include "cfg_builder.h"
#include <iostream>
#include <sstream>

namespace FasterBASIC {

// =============================================================================
// CFG Dump (Debug Visualization)
// =============================================================================

void CFGBuilder::dumpCFG(const std::string& phase) const {
    if (!m_cfg) {
        std::cout << "[CFG] No CFG to dump" << std::endl;
        return;
    }
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "    CFG DUMP";
    if (!phase.empty()) {
        std::cout << " (" << phase << ")";
    }
    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Function: " << (m_cfg->functionName.empty() ? "main" : m_cfg->functionName) << std::endl;
    std::cout << "Total Blocks: " << m_cfg->blocks.size() << std::endl;
    std::cout << "Total Edges: " << m_cfg->edges.size() << std::endl;
    std::cout << "Entry Block: " << m_cfg->entryBlock << std::endl;
    std::cout << "Exit Block: " << m_cfg->exitBlock << std::endl;
    std::cout << "Blocks Created: " << m_totalBlocksCreated << std::endl;
    std::cout << "Edges Created: " << m_totalEdgesCreated << std::endl;
    
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
            std::cout << "║ Lines: ";
            bool first = true;
            for (int line : block->lineNumbers) {
                if (!first) std::cout << ", ";
                std::cout << line;
                first = false;
            }
            std::cout << std::endl;
        }
        
        // Statements
        std::cout << "║ Statements: " << block->statements.size() << std::endl;
        for (size_t i = 0; i < block->statements.size(); i++) {
            std::cout << "║   [" << i << "] ";
            const Statement* stmt = block->statements[i];
            if (stmt) {
                // Print statement type - use typeid as we don't have getType()
                std::string typeName = typeid(*stmt).name();
                // Demangle if possible (platform-specific)
                std::cout << typeName;
            } else {
                std::cout << "NULL";
            }
            std::cout << std::endl;
        }
        
        // Predecessors
        std::cout << "║ Predecessors: ";
        if (block->predecessors.empty()) {
            std::cout << "none";
        } else {
            for (size_t i = 0; i < block->predecessors.size(); i++) {
                if (i > 0) std::cout << ", ";
                std::cout << block->predecessors[i];
            }
        }
        std::cout << std::endl;
        
        // Successors
        std::cout << "║ Successors: ";
        if (block->successors.empty()) {
            std::cout << "none";
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
                  << " -> Block " << edge.targetBlock;
        
        // Edge type
        std::cout << " [";
        switch (edge.type) {
            case EdgeType::FALLTHROUGH:
                std::cout << "FALLTHROUGH";
                break;
            case EdgeType::CONDITIONAL_TRUE:
                std::cout << "CONDITIONAL_TRUE";
                break;
            case EdgeType::CONDITIONAL_FALSE:
                std::cout << "CONDITIONAL_FALSE";
                break;
            case EdgeType::JUMP:
                std::cout << "JUMP";
                break;
            case EdgeType::CALL:
                std::cout << "CALL";
                break;
            case EdgeType::RETURN:
                std::cout << "RETURN";
                break;
            case EdgeType::EXCEPTION:
                std::cout << "EXCEPTION";
                break;
            default:
                std::cout << "UNKNOWN";
        }
        std::cout << "]";
        
        // Edge label
        if (!edge.label.empty()) {
            std::cout << " \"" << edge.label << "\"";
        }
        
        std::cout << std::endl;
    }
    
    // Dump jump targets
    if (!m_jumpTargets.empty()) {
        std::cout << "\n----------------------------------------" << std::endl;
        std::cout << "JUMP TARGETS:" << std::endl;
        std::cout << "----------------------------------------" << std::endl;
        for (int target : m_jumpTargets) {
            std::cout << "Line " << target;
            auto it = m_lineNumberToBlock.find(target);
            if (it != m_lineNumberToBlock.end()) {
                std::cout << " -> Block " << it->second;
            } else {
                std::cout << " -> (unmapped)";
            }
            std::cout << std::endl;
        }
    }
    
    // Dump deferred edges
    if (!m_deferredEdges.empty()) {
        std::cout << "\n----------------------------------------" << std::endl;
        std::cout << "DEFERRED EDGES:" << std::endl;
        std::cout << "----------------------------------------" << std::endl;
        for (const auto& edge : m_deferredEdges) {
            std::cout << "Block " << edge.sourceBlockId 
                      << " -> Line " << edge.targetLineNumber;
            if (!edge.label.empty()) {
                std::cout << " [" << edge.label << "]";
            }
            std::cout << std::endl;
        }
    }
    
    // Dump unreachable blocks
    if (!m_unreachableBlocks.empty()) {
        std::cout << "\n----------------------------------------" << std::endl;
        std::cout << "UNREACHABLE BLOCKS:" << std::endl;
        std::cout << "----------------------------------------" << std::endl;
        for (const BasicBlock* block : m_unreachableBlocks) {
            if (block) {
                std::cout << "Block " << block->id;
                if (!block->label.empty()) {
                    std::cout << " (" << block->label << ")";
                }
                std::cout << std::endl;
            }
        }
    }
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "    END CFG DUMP" << std::endl;
    std::cout << "========================================\n" << std::endl;
}

} // namespace FasterBASIC