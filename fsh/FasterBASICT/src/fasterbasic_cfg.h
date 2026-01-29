//
// fasterbasic_cfg.h
// FasterBASIC - Control Flow Graph Builder
//
// Converts validated AST into a Control Flow Graph (CFG) representation.
// The CFG consists of basic blocks connected by control flow edges.
// This is Phase 4 of the compilation pipeline.
//

#ifndef FASTERBASIC_CFG_H
#define FASTERBASIC_CFG_H

#include "fasterbasic_ast.h"
#include "fasterbasic_semantic.h"
#include <string>
#include <vector>
#include <map>
#include <set>
#include <unordered_map>
#include <memory>
#include <sstream>

namespace FasterBASIC {

// Forward declarations
class BasicBlock;
class ControlFlowGraph;

// =============================================================================
// Edge Types
// =============================================================================

enum class EdgeType {
    FALLTHROUGH,    // Sequential execution to next block
    CONDITIONAL,    // Conditional branch (IF, WHILE condition)
    UNCONDITIONAL,  // Unconditional jump (GOTO, end of FOR)
    RETURN,         // Return from GOSUB
    CALL            // GOSUB call
};

inline const char* edgeTypeToString(EdgeType type) {
    switch (type) {
        case EdgeType::FALLTHROUGH: return "FALLTHROUGH";
        case EdgeType::CONDITIONAL: return "CONDITIONAL";
        case EdgeType::UNCONDITIONAL: return "UNCONDITIONAL";
        case EdgeType::RETURN: return "RETURN";
        case EdgeType::CALL: return "CALL";
    }
    return "UNKNOWN";
}

// =============================================================================
// Control Flow Edge
// =============================================================================

struct CFGEdge {
    int sourceBlock;    // Source basic block ID
    int targetBlock;    // Target basic block ID
    EdgeType type;      // Type of edge
    std::string label;  // Optional label (e.g., "true", "false", "I=1 TO 10")
    
    CFGEdge()
        : sourceBlock(-1), targetBlock(-1), type(EdgeType::FALLTHROUGH) {}
    
    CFGEdge(int src, int tgt, EdgeType t, const std::string& lbl = "")
        : sourceBlock(src), targetBlock(tgt), type(t), label(lbl) {}
    
    std::string toString() const {
        std::ostringstream oss;
        oss << "Block " << sourceBlock << " → Block " << targetBlock;
        oss << " [" << edgeTypeToString(type);
        if (!label.empty()) {
            oss << ": " << label;
        }
        oss << "]";
        return oss.str();
    }
};

// =============================================================================
// Basic Block
// =============================================================================

class BasicBlock {
public:
    int id;                                  // Unique block ID
    std::string label;                       // Optional label (e.g., "Loop Header")
    std::vector<const Statement*> statements; // AST statements in this block
    std::vector<int> successors;             // Outgoing edges (block IDs)
    std::vector<int> predecessors;           // Incoming edges (block IDs)
    std::set<int> lineNumbers;               // BASIC line numbers in this block
    std::unordered_map<const Statement*, int> statementLineNumbers; // Maps each statement to its source line number
    
    // Block properties
    bool isLoopHeader;      // Is this a loop header?
    bool isLoopExit;        // Is this a loop exit?
    bool isSubroutine;      // Is this a GOSUB target?
    bool isTerminator;      // Does this block end with END/RETURN?
    
    BasicBlock()
        : id(-1)
        , isLoopHeader(false)
        , isLoopExit(false)
        , isSubroutine(false)
        , isTerminator(false)
    {}
    
    explicit BasicBlock(int blockId)
        : id(blockId)
        , isLoopHeader(false)
        , isLoopExit(false)
        , isSubroutine(false)
        , isTerminator(false)
    {}
    
    // Add a statement to this block
    void addStatement(const Statement* stmt) {
        statements.push_back(stmt);
    }
    
    // Add a statement with its line number to this block
    void addStatement(const Statement* stmt, int lineNum) {
        statements.push_back(stmt);
        if (lineNum > 0) {
            statementLineNumbers[stmt] = lineNum;
        }
    }
    
    // Get the line number for a specific statement
    int getLineNumber(const Statement* stmt) const {
        auto it = statementLineNumbers.find(stmt);
        if (it != statementLineNumbers.end()) {
            return it->second;
        }
        return getFirstLineNumber(); // Fallback to first line in block
    }
    
    // Add a line number to this block
    void addLineNumber(int lineNum) {
        if (lineNum > 0) {
            lineNumbers.insert(lineNum);
        }
    }
    
    // Check if block is empty
    bool isEmpty() const {
        return statements.empty();
    }
    
    // Get first line number in block
    int getFirstLineNumber() const {
        if (lineNumbers.empty()) return 0;
        return *lineNumbers.begin();
    }
    
    // Convert to string for debugging
    std::string toString() const {
        std::ostringstream oss;
        oss << "Block " << id;
        if (!label.empty()) {
            oss << " (" << label << ")";
        }
        oss << ":\n";
        
        // Properties
        if (isLoopHeader) oss << "  [Loop Header]\n";
        if (isLoopExit) oss << "  [Loop Exit]\n";
        if (isSubroutine) oss << "  [Subroutine]\n";
        if (isTerminator) oss << "  [Terminator]\n";
        
        // Line numbers
        if (!lineNumbers.empty()) {
            oss << "  Lines: ";
            bool first = true;
            for (int line : lineNumbers) {
                if (!first) oss << ", ";
                oss << line;
                first = false;
            }
            oss << "\n";
        }
        
        // Statements
        oss << "  Statements: " << statements.size() << "\n";
        
        // Successors
        if (!successors.empty()) {
            oss << "  Successors: ";
            for (size_t i = 0; i < successors.size(); ++i) {
                if (i > 0) oss << ", ";
                oss << successors[i];
            }
            oss << "\n";
        }
        
        // Predecessors
        if (!predecessors.empty()) {
            oss << "  Predecessors: ";
            for (size_t i = 0; i < predecessors.size(); ++i) {
                if (i > 0) oss << ", ";
                oss << predecessors[i];
            }
            oss << "\n";
        }
        
        return oss.str();
    }
};

// =============================================================================
// Control Flow Graph (for a single function or main program)
// =============================================================================

class ControlFlowGraph {
public:
    std::string functionName;  // Function/SUB name, or "main" for main program
    std::vector<std::string> parameters;  // Function parameters
    std::vector<VariableType> parameterTypes;  // Parameter types
    VariableType returnType;   // Return type (UNKNOWN for SUBs)
    const DefStatement* defStatement;  // For DEF FN functions - pointer to the statement containing the body expression
    
    std::vector<std::unique_ptr<BasicBlock>> blocks;
    std::vector<CFGEdge> edges;
    int entryBlock;     // Entry point (usually block 0)
    int exitBlock;      // Exit point (usually last block)
    
    // Mapping from BASIC line numbers to block IDs
    std::map<int, int> lineNumberToBlock;
    
    // SELECT CASE structure tracking
    struct SelectCaseInfo {
        int selectBlock;                 // Block that evaluates the SELECT expression
        std::vector<int> testBlocks;     // Blocks that test each CASE condition
        std::vector<int> bodyBlocks;     // Blocks that execute each CASE body
        int elseBlock;                   // ELSE/OTHERWISE block (-1 if none)
        int exitBlock;                   // Block after END SELECT
        const CaseStatement* caseStatement;  // Pointer to AST node
    };
    std::map<int, SelectCaseInfo> selectCaseInfo;  // testBlock ID → SelectCaseInfo
    
    // FOR loop structure tracking
    struct ForLoopBlocks {
        int initBlock;   // Block with FOR statement (initialization)
        int checkBlock;  // Block that checks loop condition
        int bodyBlock;   // Block with loop body statements
        int exitBlock;   // Block after loop (exit target)
        std::string variable;  // Loop variable name (plain, no suffix)
    };
    std::map<int, ForLoopBlocks> forLoopStructure;  // initBlock → ForLoopBlocks
    
    // DO loop structure tracking
    struct DoLoopBlocks {
        int headerBlock;  // Block with DO statement (header)
        int bodyBlock;    // Block with loop body statements
        int exitBlock;    // Block after loop (exit target)
    };
    std::map<int, DoLoopBlocks> doLoopStructure;  // headerBlock → DoLoopBlocks
    
    // TRY/CATCH/FINALLY structure tracking
    struct TryCatchBlocks {
        int tryBlock;              // Block that sets up exception context (setjmp)
        int tryBodyBlock;          // First block of TRY body statements
        int dispatchBlock;         // Block that dispatches to appropriate CATCH
        std::vector<int> catchBlocks;  // One block per CATCH clause
        int finallyBlock;          // FINALLY block (-1 if none)
        int exitBlock;             // Block after END TRY
        bool hasFinally;           // Whether FINALLY is present
        const TryCatchStatement* tryStatement;  // Pointer to AST node
    };
    std::map<int, TryCatchBlocks> tryCatchStructure;  // tryBlock → TryCatchBlocks
    
    // Mapping from loop constructs
    std::map<int, int> forLoopHeaders;      // FOR statement → block ID (deprecated, use forLoopStructure)
    std::map<int, int> whileLoopHeaders;    // WHILE statement → block ID
    std::map<int, int> repeatLoopHeaders;   // REPEAT statement → block ID
    std::map<int, int> doLoopHeaders;       // DO statement → block ID
    
    ControlFlowGraph()
        : returnType(VariableType::UNKNOWN), entryBlock(0), exitBlock(-1), defStatement(nullptr)
    {}
    
    ControlFlowGraph(const std::string& name)
        : functionName(name), returnType(VariableType::UNKNOWN), entryBlock(0), exitBlock(-1), defStatement(nullptr)
    {}
    
    // Create a new basic block
    BasicBlock* createBlock(const std::string& label = "") {
        int id = static_cast<int>(blocks.size());
        auto block = std::make_unique<BasicBlock>(id);
        block->label = label;
        blocks.push_back(std::move(block));
        return blocks.back().get();
    }
    
    // Get block by ID
    BasicBlock* getBlock(int id) {
        if (id >= 0 && id < static_cast<int>(blocks.size())) {
            return blocks[id].get();
        }
        return nullptr;
    }
    
    const BasicBlock* getBlock(int id) const {
        if (id >= 0 && id < static_cast<int>(blocks.size())) {
            return blocks[id].get();
        }
        return nullptr;
    }
    
    // Add an edge between blocks
    void addEdge(int source, int target, EdgeType type, const std::string& label = "") {
        edges.emplace_back(source, target, type, label);
        
        // Update successor/predecessor lists
        auto* srcBlock = getBlock(source);
        auto* tgtBlock = getBlock(target);
        
        if (srcBlock && tgtBlock) {
            srcBlock->successors.push_back(target);
            tgtBlock->predecessors.push_back(source);
        }
    }
    
    // Map a BASIC line number to a block
    void mapLineToBlock(int lineNumber, int blockId) {
        lineNumberToBlock[lineNumber] = blockId;
    }
    
    // Get block containing a line number
    int getBlockForLine(int lineNumber) const {
        auto it = lineNumberToBlock.find(lineNumber);
        if (it != lineNumberToBlock.end()) {
            return it->second;
        }
        return -1;
    }
    
    // Get block for line number, or next available line if target doesn't exist
    int getBlockForLineOrNext(int lineNumber) const {
        // First try exact match
        auto it = lineNumberToBlock.find(lineNumber);
        if (it != lineNumberToBlock.end()) {
            return it->second;
        }
        
        // Find the smallest line number greater than target
        int nextLine = -1;
        int nextBlock = -1;
        
        for (const auto& pair : lineNumberToBlock) {
            if (pair.first > lineNumber) {
                if (nextLine == -1 || pair.first < nextLine) {
                    nextLine = pair.first;
                    nextBlock = pair.second;
                }
            }
        }
        
        return nextBlock;
    }
    
    // Statistics
    int getBlockCount() const { return static_cast<int>(blocks.size()); }
    int getEdgeCount() const { return static_cast<int>(edges.size()); }
    
    int getLoopCount() const {
        return static_cast<int>(forLoopHeaders.size() + 
                               whileLoopHeaders.size() + 
                               repeatLoopHeaders.size() +
                               doLoopHeaders.size());
    }
    
    // Check if a GOTO from sourceLineNumber to targetLineNumber creates a back edge
    bool isBackEdge(int sourceLineNumber, int targetLineNumber) const {
        int sourceBlock = getBlockForLine(sourceLineNumber);
        int targetBlock = getBlockForLine(targetLineNumber);
        
        if (sourceBlock >= 0 && targetBlock >= 0) {
            // Simple heuristic: back edge if target block has lower ID (earlier in program)
            // or if target block is marked as a loop header
            const BasicBlock* target = getBlock(targetBlock);
            return (targetBlock < sourceBlock) || (target && target->isLoopHeader);
        }
        
        return false;
    }
    
    // Convert to string for debugging
    std::string toString() const {
        std::ostringstream oss;
        
        oss << "=== CONTROL FLOW GRAPH ===\n\n";
        
        oss << "Summary:\n";
        oss << "  Basic Blocks: " << getBlockCount() << "\n";
        oss << "  Edges: " << getEdgeCount() << "\n";
        oss << "  Loops: " << getLoopCount() << "\n";
        oss << "  Entry Block: " << entryBlock << "\n";
        oss << "  Exit Block: " << exitBlock << "\n";
        oss << "\n";
        
        // List all blocks
        oss << "Basic Blocks:\n";
        for (const auto& block : blocks) {
            oss << block->toString() << "\n";
        }
        
        // List all edges
        if (!edges.empty()) {
            oss << "Edges:\n";
            for (const auto& edge : edges) {
                oss << "  " << edge.toString() << "\n";
            }
            oss << "\n";
        }
        
        // Line number mapping
        if (!lineNumberToBlock.empty()) {
            oss << "Line Number Mapping:\n";
            for (const auto& [line, block] : lineNumberToBlock) {
                oss << "  Line " << line << " → Block " << block << "\n";
            }
            oss << "\n";
        }
        
        oss << "=== END CONTROL FLOW GRAPH ===\n";
        
        return oss.str();
    }
};

// =============================================================================
// Program CFG (manages main program + all functions)
// =============================================================================

class ProgramCFG {
public:
std::unique_ptr<ControlFlowGraph> mainCFG;  // Main program CFG
std::unordered_map<std::string, std::unique_ptr<ControlFlowGraph>> functionCFGs;  // Function CFGs by name
    
ProgramCFG() : mainCFG(std::make_unique<ControlFlowGraph>("main")) {}
    
// Get or create a function CFG
ControlFlowGraph* getFunctionCFG(const std::string& name) {
    auto it = functionCFGs.find(name);
    if (it != functionCFGs.end()) {
        return it->second.get();
    }
    return nullptr;
}

const ControlFlowGraph* getFunctionCFG(const std::string& name) const {
    auto it = functionCFGs.find(name);
    if (it != functionCFGs.end()) {
        return it->second.get();
    }
    return nullptr;
}
    
// Create a new function CFG
ControlFlowGraph* createFunctionCFG(const std::string& name) {
    auto cfg = std::make_unique<ControlFlowGraph>(name);
    auto* ptr = cfg.get();
    functionCFGs[name] = std::move(cfg);
    return ptr;
}
    
// Get all function names
std::vector<std::string> getFunctionNames() const {
    std::vector<std::string> names;
    for (const auto& pair : functionCFGs) {
        names.push_back(pair.first);
    }
    return names;
}
    
int getBlockCount() const {
    int count = mainCFG ? static_cast<int>(mainCFG->blocks.size()) : 0;
    for (const auto& pair : functionCFGs) {
        count += static_cast<int>(pair.second->blocks.size());
    }
    return count;
}
};

// =============================================================================
// CFG Builder
// =============================================================================

class CFGBuilder {
public:
    CFGBuilder();
    ~CFGBuilder();
    
    // Main entry point - build CFG from AST (returns ProgramCFG with main + functions)
    std::unique_ptr<ProgramCFG> build(const Program& program, 
                                      const SymbolTable& symbols);
    
    // Configuration
    void setCreateExitBlock(bool create) { m_createExitBlock = create; }
    void setMergeSequentialBlocks(bool merge) { m_mergeBlocks = merge; }
    
    // Get build statistics
    int getBlocksCreated() const { return m_blocksCreated; }
    int getEdgesCreated() const { return m_edgesCreated; }
    
    // Report generation
    std::string generateReport(const ControlFlowGraph& cfg) const;
    
private:
    // CFG construction
    std::set<int> collectJumpTargets(const Program& program);
    void buildBlocks(const Program& program, const std::set<int>& jumpTargets);
    void buildEdges();
    void identifyLoops();
    void identifySubroutines();
    void optimizeCFG();
    
    // Statement processing
    void processStatement(const Statement& stmt, BasicBlock* currentBlock, int lineNumber = 0);
    void processIfStatement(const IfStatement& stmt, BasicBlock* currentBlock);
    void processForStatement(const ForStatement& stmt, BasicBlock* currentBlock);
    void processForInStatement(const ForInStatement& stmt, BasicBlock* currentBlock);
    void processWhileStatement(const WhileStatement& stmt, BasicBlock* currentBlock);
    void processRepeatStatement(const RepeatStatement& stmt, BasicBlock* currentBlock);
    void processDoStatement(const DoStatement& stmt, BasicBlock* currentBlock);
    void processCaseStatement(const CaseStatement& stmt, BasicBlock* currentBlock);
    void processFunctionStatement(const FunctionStatement& stmt, BasicBlock* currentBlock);
    void processSubStatement(const SubStatement& stmt, BasicBlock* currentBlock);
    void processDefStatement(const DefStatement& stmt, BasicBlock* currentBlock);
    void processGotoStatement(const GotoStatement& stmt, BasicBlock* currentBlock);
    void processGosubStatement(const GosubStatement& stmt, BasicBlock* currentBlock);
    void processOnGotoStatement(const OnGotoStatement& stmt, BasicBlock* currentBlock);
    void processOnGosubStatement(const OnGosubStatement& stmt, BasicBlock* currentBlock);
    void processLabelStatement(const LabelStatement& stmt, BasicBlock* currentBlock);
    void processTryCatchStatement(const TryCatchStatement& stmt, BasicBlock* currentBlock);
    
    // Helper functions
    VariableType inferTypeFromName(const std::string& name);
    
    // Block management
    BasicBlock* createNewBlock(const std::string& label = "");
    void finalizeBlock(BasicBlock* block);
    
    // Edge creation helpers
    void addFallthroughEdge(int source, int target);
    void addConditionalEdge(int source, int target, const std::string& condition);
    void addUnconditionalEdge(int source, int target);
    void addCallEdge(int source, int target);
    void addReturnEdge(int source, int target);
    
    // Data
    std::unique_ptr<ProgramCFG> m_programCFG;
    ControlFlowGraph* m_currentCFG;  // Points to the CFG currently being built
    const Program* m_program;
    const SymbolTable* m_symbols;
    
    // Current building state
    BasicBlock* m_currentBlock;
    int m_nextBlockId;
    
    // Control flow tracking
    struct LoopContext {
        int headerBlock;
        int exitBlock;
        std::string variable;  // For FOR loops
    };
    std::vector<LoopContext> m_loopStack;
    
    // Map from NEXT statement block ID to loop header block ID (for back-edges)
    std::map<int, int> m_nextToHeaderMap;
    
    struct SelectCaseContext {
        int selectBlock;                 // Block that evaluates the SELECT expression
        std::vector<int> testBlocks;     // Blocks that test each CASE condition
        std::vector<int> bodyBlocks;     // Blocks that execute each CASE body
        int elseBlock;                   // ELSE/OTHERWISE block (-1 if none)
        int exitBlock;                   // Block after END SELECT
        const CaseStatement* caseStatement;  // Pointer to AST node
    };
    std::vector<SelectCaseContext> m_selectCaseStack;
    
    // TRY/CATCH tracking
    struct TryCatchContext {
        int tryBlock;              // Block that sets up exception context
        int tryBodyBlock;          // First block of TRY body
        int dispatchBlock;         // Exception dispatcher block
        std::vector<int> catchBlocks;  // CATCH handler blocks
        int finallyBlock;          // FINALLY block (-1 if none)
        int exitBlock;             // Block after END TRY
        bool hasFinally;           // Whether FINALLY is present
        const TryCatchStatement* tryStatement;
    };
    std::vector<TryCatchContext> m_tryCatchStack;
    
    // Configuration
    bool m_createExitBlock;
    bool m_mergeBlocks;
    
    // Statistics
    int m_blocksCreated;
    int m_edgesCreated;
};

} // namespace FasterBASIC

#endif // FASTERBASIC_CFG_H