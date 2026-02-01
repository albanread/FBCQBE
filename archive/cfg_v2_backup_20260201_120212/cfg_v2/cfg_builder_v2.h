//
// cfg_builder_v2.h
// FasterBASIC - Control Flow Graph Builder V2
//
// ARCHITECTURAL REDESIGN (February 2025)
// 
// This is a complete rewrite of the CFG builder using single-pass recursive
// construction instead of the old two-phase approach (build blocks, then edges).
//
// KEY PRINCIPLES:
// 1. Single-pass construction: Create blocks and edges together
// 2. Recursive composition: Each control structure is self-contained
// 3. Explicit wiring: No implicit fallthrough assumptions
// 4. Context passing: No global state or stacks
// 5. Black-box abstraction: Functions return entry/exit blocks only
//
// WHY THIS FIX IS NEEDED:
// The old builder assumed block N flows to block N+1. This breaks with nested
// structures where the "next" block physically might be internal to a nested
// structure, but logically execution should go elsewhere.
//
// EXAMPLE OF THE FIX:
// Old approach (BROKEN):
//   Phase 1: Create all blocks linearly [1][2][3][4][5]
//   Phase 2: Scan forward to find loop ends, add back-edges
//   Problem: By Phase 2, context is lost, scanning fails
//
// New approach (FIXED):
//   buildWhile(incoming) {
//     header = create(); body = create(); exit = create();
//     wire(incoming -> header);
//     wire(header -> body [true]); wire(header -> exit [false]);
//     bodyExit = buildStatements(body);
//     wire(bodyExit -> header);  // Back-edge created immediately!
//     return exit;  // Next statement connects here
//   }
//

#ifndef FASTERBASIC_CFG_BUILDER_V2_H
#define FASTERBASIC_CFG_BUILDER_V2_H

#include "../fasterbasic_ast.h"
#include "../fasterbasic_cfg.h"
#include <string>
#include <vector>
#include <map>
#include <memory>

namespace FasterBASIC {

// Forward declarations
class BasicBlock;
class ControlFlowGraph;

// =============================================================================
// CFGBuilderV2 - Single-Pass Recursive CFG Construction
// =============================================================================

class CFGBuilderV2 {
public:
    // Context structures for nested control flow
    // These replace the global stacks from the old implementation
    
    // Loop context: Tracks loop header/exit for CONTINUE/EXIT statements
    struct LoopContext {
        int headerBlockId;           // Loop header (for CONTINUE)
        int exitBlockId;             // Loop exit (for EXIT FOR/WHILE/DO)
        std::string loopType;        // "FOR", "WHILE", "DO", "REPEAT"
        LoopContext* outerLoop;      // Link to enclosing loop (nullptr if outermost)
        
        LoopContext()
            : headerBlockId(-1), exitBlockId(-1), outerLoop(nullptr) {}
    };
    
    // SELECT CASE context: Tracks exit point for EXIT SELECT
    struct SelectContext {
        int exitBlockId;             // Block to jump to on EXIT SELECT
        SelectContext* outerSelect;  // Link to enclosing SELECT (nullptr if outermost)
        
        SelectContext()
            : exitBlockId(-1), outerSelect(nullptr) {}
    };
    
    // TRY/CATCH context: Tracks catch/finally blocks for exception handling
    struct TryContext {
        int catchBlockId;            // Catch block (for THROW)
        int finallyBlockId;          // Finally block (always executed)
        TryContext* outerTry;        // Link to enclosing TRY (nullptr if outermost)
        
        TryContext()
            : catchBlockId(-1), finallyBlockId(-1), outerTry(nullptr) {}
    };
    
    // Subroutine context: Tracks GOSUB call sites for RETURN
    struct SubroutineContext {
        int returnBlockId;           // Block to return to
        SubroutineContext* outerSub; // Link to enclosing GOSUB (nullptr if outermost)
        
        SubroutineContext()
            : returnBlockId(-1), outerSub(nullptr) {}
    };

public:
    CFGBuilderV2();
    ~CFGBuilderV2();
    
    // Main entry point: Build CFG from validated AST
    // Returns: Complete CFG with all blocks and edges wired
    ControlFlowGraph* build(const std::vector<StatementPtr>& statements);
    
    // Adapter: Build CFG from Program structure (flattens ProgramLines)
    // This allows CFG v2 to work with the existing Program AST structure
    // Note: The Program should already have loop bodies populated by the parser
    ControlFlowGraph* buildFromProgram(const Program& program);
    
    // Get the constructed CFG (transfers ownership)
    ControlFlowGraph* takeCFG();

private:
    // =============================================================================
    // Core Recursive Builder
    // =============================================================================
    
    // Build a range of statements starting from 'incoming' block
    // Returns: The "exit" block where control flows after executing all statements
    // 
    // This is the heart of the new architecture. It processes statements one by one,
    // and when it encounters a control structure, it calls the appropriate builder
    // function which recursively handles the nested structure.
    //
    // Context parameters are optional - pass nullptr if not in that context
    BasicBlock* buildStatementRange(
        const std::vector<StatementPtr>& statements,
        BasicBlock* incoming,
        LoopContext* currentLoop = nullptr,
        SelectContext* currentSelect = nullptr,
        TryContext* currentTry = nullptr,
        SubroutineContext* currentSub = nullptr
    );
    
    // =============================================================================
    // Control Structure Builders
    // =============================================================================
    // Each builder follows this contract:
    // - Accepts an incoming block (where control enters)
    // - Creates all necessary internal blocks
    // - Wires all edges (including back-edges for loops)
    // - Recursively processes nested statements
    // - Returns the exit block (where control leaves)
    //
    // If a structure never exits (e.g., infinite loop, GOTO), it returns an
    // unreachable block so subsequent statements can still be added (even if dead).
    // =============================================================================
    
    // IF...THEN...ELSE...END IF
    BasicBlock* buildIf(
        const IfStatement& stmt,
        BasicBlock* incoming,
        LoopContext* loop,
        SelectContext* select,
        TryContext* tryCtx,
        SubroutineContext* sub
    );
    
    // WHILE...WEND (pre-test loop)
    BasicBlock* buildWhile(
        const WhileStatement& stmt,
        BasicBlock* incoming,
        LoopContext* outerLoop,
        SelectContext* select,
        TryContext* tryCtx,
        SubroutineContext* sub
    );
    
    // FOR...NEXT (counted loop with optional STEP)
    BasicBlock* buildFor(
        const ForStatement& stmt,
        BasicBlock* incoming,
        LoopContext* outerLoop,
        SelectContext* select,
        TryContext* tryCtx,
        SubroutineContext* sub
    );
    
    // REPEAT...UNTIL (post-test loop)
    BasicBlock* buildRepeat(
        const RepeatStatement& stmt,
        BasicBlock* incoming,
        LoopContext* outerLoop,
        SelectContext* select,
        TryContext* tryCtx,
        SubroutineContext* sub
    );
    
    // DO...LOOP variants (WHILE/UNTIL, pre-test/post-test)
    BasicBlock* buildDo(
        const DoStatement& stmt,
        BasicBlock* incoming,
        LoopContext* outerLoop,
        SelectContext* select,
        TryContext* tryCtx,
        SubroutineContext* sub
    );
    
    // SELECT CASE...END SELECT
    BasicBlock* buildSelectCase(
        const SelectCaseStatement& stmt,
        BasicBlock* incoming,
        LoopContext* loop,
        SelectContext* outerSelect,
        TryContext* tryCtx,
        SubroutineContext* sub
    );
    
    // TRY...CATCH...FINALLY...END TRY
    BasicBlock* buildTryCatch(
        const TryStatement& stmt,
        BasicBlock* incoming,
        LoopContext* loop,
        SelectContext* select,
        TryContext* outerTry,
        SubroutineContext* sub
    );
    
    // =============================================================================
    // Terminator Handlers
    // =============================================================================
    // These handle statements that change control flow and don't return
    // (GOTO, RETURN, EXIT, etc.)
    // =============================================================================
    
    // GOTO line_number
    BasicBlock* handleGoto(
        const GotoStatement& stmt,
        BasicBlock* incoming
    );
    
    // GOSUB line_number (call subroutine)
    BasicBlock* handleGosub(
        const GosubStatement& stmt,
        BasicBlock* incoming,
        LoopContext* loop,
        SelectContext* select,
        TryContext* tryCtx,
        SubroutineContext* outerSub
    );
    
    // RETURN (from GOSUB)
    BasicBlock* handleReturn(
        const ReturnStatement& stmt,
        BasicBlock* incoming,
        SubroutineContext* sub
    );
    
    // ON...GOTO (computed goto with fallthrough)
    BasicBlock* handleOnGoto(
        const OnGotoStatement& stmt,
        BasicBlock* incoming
    );
    
    // ON...GOSUB (computed gosub with fallthrough)
    BasicBlock* handleOnGosub(
        const OnGosubStatement& stmt,
        BasicBlock* incoming,
        LoopContext* loop,
        SelectContext* select,
        TryContext* tryCtx,
        SubroutineContext* outerSub
    );
    
    // EXIT FOR (exit current FOR loop)
    BasicBlock* handleExitFor(
        BasicBlock* incoming,
        LoopContext* loop
    );
    
    // EXIT WHILE (exit current WHILE loop)
    BasicBlock* handleExitWhile(
        BasicBlock* incoming,
        LoopContext* loop
    );
    
    // EXIT DO (exit current DO loop)
    BasicBlock* handleExitDo(
        BasicBlock* incoming,
        LoopContext* loop
    );
    
    // EXIT SELECT (exit current SELECT CASE)
    BasicBlock* handleExitSelect(
        BasicBlock* incoming,
        SelectContext* select
    );
    
    // CONTINUE (jump to loop header - if supported)
    BasicBlock* handleContinue(
        BasicBlock* incoming,
        LoopContext* loop
    );
    
    // END (program termination)
    BasicBlock* handleEnd(
        const EndStatement& stmt,
        BasicBlock* incoming
    );
    
    // THROW error_code (exception handling)
    BasicBlock* handleThrow(
        const ThrowStatement& stmt,
        BasicBlock* incoming,
        TryContext* tryCtx
    );
    
    // =============================================================================
    // Block and Edge Management
    // =============================================================================
    
    // Create a new basic block
    BasicBlock* createBlock(const std::string& label = "");
    
    // Create an unreachable block (for dead code after terminators)
    // This allows subsequent statements to be added even if they're unreachable
    BasicBlock* createUnreachableBlock();
    
    // Add an edge between two blocks
    void addEdge(int fromBlockId, int toBlockId, const std::string& label = "");
    
    // Add a conditional edge (for IF, WHILE conditions)
    void addConditionalEdge(int fromBlockId, int toBlockId, const std::string& condition);
    
    // Add an unconditional edge (for GOTO, loop back-edges)
    void addUnconditionalEdge(int fromBlockId, int toBlockId);
    
    // Mark a block as terminated (no fallthrough)
    void markTerminated(BasicBlock* block);
    
    // Check if a block is terminated
    bool isTerminated(const BasicBlock* block) const;
    
    // =============================================================================
    // Label and Line Number Resolution
    // =============================================================================
    
    // Resolve a BASIC line number to a block ID
    // (Used for GOTO, GOSUB, ON GOTO, etc.)
    int resolveLineNumberToBlock(int lineNumber);
    
    // Register a block as the target for a specific line number
    void registerLineNumberBlock(int lineNumber, int blockId);
    
    // Register a label as the target for a specific block
    void registerLabel(const std::string& label, int blockId);
    
    // Resolve a label to a block ID
    int resolveLabelToBlock(const std::string& label);
    
    // =============================================================================
    // Helper Functions
    // =============================================================================
    
    // Add a statement to a block with line number tracking
    void addStatementToBlock(BasicBlock* block, const Statement* stmt, int lineNumber = -1);
    
    // Find the innermost loop context of a specific type
    LoopContext* findLoopContext(LoopContext* ctx, const std::string& loopType);
    
    // Split a block if it already contains statements
    // Returns: The new block to continue building in
    BasicBlock* splitBlockIfNeeded(BasicBlock* block);
    
    // Get the current line number for a statement
    int getLineNumber(const Statement* stmt);
    
    // Debug: Dump the current CFG structure
    void dumpCFG(const std::string& phase = "") const;
    
    // =============================================================================
    // Jump Target Collection (Phase 0)
    // =============================================================================
    
    // Collect all GOTO/GOSUB targets from statement list
    void collectJumpTargets(const std::vector<StatementPtr>& statements);
    
    // Collect all GOTO/GOSUB targets from Program structure
    void collectJumpTargetsFromProgram(const Program& program);
    
    // Recursively collect jump targets from a single statement
    void collectJumpTargetsFromStatement(const Statement* stmt);
    
    // Check if a line number is a jump target
    bool isJumpTarget(int lineNumber) const;

private:
    // =============================================================================
    // Internal State
    // =============================================================================
    
    ControlFlowGraph* m_cfg;                          // The CFG being constructed
    int m_nextBlockId;                                // Next available block ID
    
    // Line number and label mappings
    std::map<int, int> m_lineNumberToBlock;           // Maps BASIC line numbers to blocks
    std::map<std::string, int> m_labelToBlock;        // Maps labels to blocks
    
    // Deferred edge resolution (for forward references)
    struct DeferredEdge {
        int sourceBlockId;
        int targetLineNumber;
        std::string label;
    };
    std::vector<DeferredEdge> m_deferredEdges;        // Edges to resolve later
    
    // Statistics and debugging
    int m_totalBlocksCreated;
    int m_totalEdgesCreated;
    bool m_debugMode;                                 // Enable verbose logging
    
    // Program structure tracking
    BasicBlock* m_entryBlock;                         // Program entry point
    BasicBlock* m_exitBlock;                          // Program exit point
    
    // Unreachable code tracking
    std::vector<BasicBlock*> m_unreachableBlocks;     // Dead code blocks
    
    // Jump target tracking (Phase 0)
    std::set<int> m_jumpTargets;                      // Line numbers that are GOTO/GOSUB targets
};

} // namespace FasterBASIC

#endif // FASTERBASIC_CFG_BUILDER_V2_H