//
// cfg_builder_core.cpp
// FasterBASIC - Control Flow Graph Builder Core Implementation
//
// Contains constructor, destructor, and main build() entry point.
// Part of modular CFG builder split (February 2026).
//

#include "cfg_builder.h"
#include <algorithm>
#include <sstream>
#include <iostream>
#include <functional>

namespace FasterBASIC {

// =============================================================================
// Constructor/Destructor
// =============================================================================

CFGBuilder::CFGBuilder()
    : m_program(nullptr)
    , m_symbols(nullptr)
    , m_currentBlock(nullptr)
    , m_nextBlockId(0)
    , m_createExitBlock(true)
    , m_mergeBlocks(false)
    , m_blocksCreated(0)
    , m_edgesCreated(0)
    , m_processingNestedStatements(false)
{
}

CFGBuilder::~CFGBuilder() = default;

// =============================================================================
// Main Build Entry Point
// =============================================================================

std::unique_ptr<ProgramCFG> CFGBuilder::build(const Program& program,
                                               const SymbolTable& symbols) {
    m_program = &program;
    m_symbols = &symbols;
    m_programCFG = std::make_unique<ProgramCFG>();
    m_blocksCreated = 0;
    m_edgesCreated = 0;
    m_loopStack.clear();
    
    // Build main program CFG
    m_currentCFG = m_programCFG->mainCFG.get();
    
    // Phase 0: Pre-scan to collect jump targets (main program only)
    std::set<int> jumpTargets = collectJumpTargets(program);
    
    // Phase 1: Build basic blocks (main program + extract functions)
    buildBlocks(program, jumpTargets);
    
    // Phase 2: Build control flow edges for main program
    buildEdges();
    
    // Phase 3: Build edges for each function
    for (auto& pair : m_programCFG->functionCFGs) {
        m_currentCFG = pair.second.get();
        buildEdges();
    }
    
    // Phase 4: Identify loop structures in main
    m_currentCFG = m_programCFG->mainCFG.get();
    identifyLoops();
    
    // Phase 5: Identify loop structures in functions
    for (auto& pair : m_programCFG->functionCFGs) {
        m_currentCFG = pair.second.get();
        identifyLoops();
    }
    
    // Phase 6: Identify subroutines in main
    m_currentCFG = m_programCFG->mainCFG.get();
    identifySubroutines();
    
    // Phase 7: Optimize CFG (optional)
    if (m_mergeBlocks) {
        m_currentCFG = m_programCFG->mainCFG.get();
        optimizeCFG();
        for (auto& pair : m_programCFG->functionCFGs) {
            m_currentCFG = pair.second.get();
            optimizeCFG();
        }
    }
    
    return std::move(m_programCFG);
}

} // namespace FasterBASIC