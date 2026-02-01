//
// cfg_builder_utils.cpp
// FasterBASIC - Control Flow Graph Builder Utility Functions
//
// Contains utility functions: report generation and type inference.
// Part of modular CFG builder split (February 2026).
//

#include "cfg_builder.h"
#include <algorithm>
#include <sstream>
#include <iostream>
#include <functional>

namespace FasterBASIC {

// =============================================================================
// Report Generation
// =============================================================================

std::string CFGBuilder::generateReport(const ControlFlowGraph& cfg) const {
    std::ostringstream oss;
    
    oss << "=== CFG BUILDER REPORT ===\n\n";
    
    // Build statistics
    oss << "Build Statistics:\n";
    oss << "  Blocks Created: " << m_blocksCreated << "\n";
    oss << "  Edges Created: " << m_edgesCreated << "\n";
    oss << "  Loop Headers: " << cfg.getLoopCount() << "\n";
    oss << "\n";
    
    // CFG summary
    oss << "CFG Summary:\n";
    oss << "  Total Blocks: " << cfg.getBlockCount() << "\n";
    oss << "  Total Edges: " << cfg.getEdgeCount() << "\n";
    oss << "  Entry Block: " << cfg.entryBlock << "\n";
    oss << "  Exit Block: " << cfg.exitBlock << "\n";
    oss << "\n";
    
    // Block types
    int loopHeaders = 0;
    int loopExits = 0;
    int subroutines = 0;
    int terminators = 0;
    
    for (const auto& block : cfg.blocks) {
        if (block->isLoopHeader) loopHeaders++;
        if (block->isLoopExit) loopExits++;
        if (block->isSubroutine) subroutines++;
        if (block->isTerminator) terminators++;
    }
    
    oss << "Block Analysis:\n";
    oss << "  Loop Headers: " << loopHeaders << "\n";
    oss << "  Loop Exits: " << loopExits << "\n";
    oss << "  Subroutines: " << subroutines << "\n";
    oss << "  Terminators: " << terminators << "\n";
    oss << "\n";
    
    // Edge types
    int fallthroughEdges = 0;
    int conditionalEdges = 0;
    int unconditionalEdges = 0;
    int callEdges = 0;
    int returnEdges = 0;
    
    for (const auto& edge : cfg.edges) {
        switch (edge.type) {
            case EdgeType::FALLTHROUGH: fallthroughEdges++; break;
            case EdgeType::CONDITIONAL: conditionalEdges++; break;
            case EdgeType::UNCONDITIONAL: unconditionalEdges++; break;
            case EdgeType::CALL: callEdges++; break;
            case EdgeType::RETURN: returnEdges++; break;
        }
    }
    
    oss << "Edge Analysis:\n";
    oss << "  Fallthrough: " << fallthroughEdges << "\n";
    oss << "  Conditional: " << conditionalEdges << "\n";
    oss << "  Unconditional: " << unconditionalEdges << "\n";
    oss << "  Call: " << callEdges << "\n";
    oss << "  Return: " << returnEdges << "\n";
    oss << "\n";
    
    // Full CFG details
    oss << cfg.toString();
    
    oss << "=== END CFG BUILDER REPORT ===\n";
    
    return oss.str();
}

// =============================================================================
// Type Inference
// =============================================================================

// Helper function to infer type from variable name (suffix-based)
// For 64-bit systems (ARM64/x86-64), DOUBLE is the natural numeric type
VariableType CFGBuilder::inferTypeFromName(const std::string& name) {
    if (name.empty()) return VariableType::DOUBLE;  // Default for 64-bit systems
    
    char lastChar = name.back();
    switch (lastChar) {
        case '%': return VariableType::INT;      // Integer (32/64-bit on modern systems)
        case '!': return VariableType::FLOAT;    // Single-precision (32-bit float)
        case '#': return VariableType::DOUBLE;   // Double-precision (64-bit float)
        case '$': return VariableType::STRING;
        default: return VariableType::DOUBLE;    // Default: DOUBLE for 64-bit systems (ARM64/x86-64)
    }
}

} // namespace FasterBASIC