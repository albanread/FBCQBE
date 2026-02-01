//
// cfg_builder_blocks.cpp
// FasterBASIC - Control Flow Graph Builder Block Management
//
// Contains block creation and edge management functions.
// Part of modular CFG builder split (February 2026).
//

#include "cfg_builder.h"
#include <algorithm>
#include <sstream>
#include <iostream>
#include <functional>

namespace FasterBASIC {

// =============================================================================
// Block Management
// =============================================================================

BasicBlock* CFGBuilder::createNewBlock(const std::string& label) {
    auto* block = m_currentCFG->createBlock(label);
    m_blocksCreated++;
    return block;
}

void CFGBuilder::finalizeBlock(BasicBlock* block) {
    // Any finalization needed for a block
}

// =============================================================================
// Edge Creation Helpers
// =============================================================================

void CFGBuilder::addFallthroughEdge(int source, int target) {
    m_currentCFG->addEdge(source, target, EdgeType::FALLTHROUGH);
    m_edgesCreated++;
}

void CFGBuilder::addConditionalEdge(int source, int target, const std::string& label) {
    m_currentCFG->addEdge(source, target, EdgeType::CONDITIONAL, label);
    m_edgesCreated++;
}

void CFGBuilder::addUnconditionalEdge(int source, int target) {
    m_currentCFG->addEdge(source, target, EdgeType::UNCONDITIONAL);
    m_edgesCreated++;
}

void CFGBuilder::addCallEdge(int source, int target) {
    m_currentCFG->addEdge(source, target, EdgeType::CALL);
    m_edgesCreated++;
}

void CFGBuilder::addReturnEdge(int source, int target) {
    m_currentCFG->addEdge(source, target, EdgeType::RETURN);
    m_edgesCreated++;
}

} // namespace FasterBASIC