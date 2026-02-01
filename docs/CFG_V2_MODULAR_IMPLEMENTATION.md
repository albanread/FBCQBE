# CFG V2 Modular Implementation Plan

**Date:** February 1, 2026  
**Status:** READY TO IMPLEMENT  
**Goal:** Implement v2 single-pass recursive design in the modular file structure

---

## What We Have

### ✅ Excellent Modular Structure (Created)
```
fsh/FasterBASICT/src/cfg/
├── cfg_builder.h                    # Main header
├── cfg_builder_core.cpp             # Constructor, entry points
├── cfg_builder_blocks.cpp           # Block/edge management
├── cfg_builder_utils.cpp            # Utilities
├── cfg_builder_jumptargets.cpp      # Jump target pre-scan
├── cfg_builder_statements.cpp       # Statement dispatcher
├── cfg_builder_jumps.cpp            # GOTO/GOSUB handlers
├── cfg_builder_conditional.cpp      # IF/SELECT CASE
├── cfg_builder_loops.cpp            # FOR/WHILE/REPEAT/DO
├── cfg_builder_exception.cpp        # TRY/CATCH/FINALLY
├── cfg_builder_functions.cpp        # FUNCTION/SUB/DEF
└── cfg_builder_edges.cpp            # Edge building (will be deprecated)
```

### ✅ V2 Design Documents
- `archive/cfg_v2_backup_20260201_120212/cfg_v2/README.md` - Design rationale
- `archive/cfg_v2_backup_20260201_120212/cfg_v2/cfg_builder_v2.h` - V2 API
- `archive/cfg_v2_backup_20260201_120212/cfg_v2/cfg_builder_v2.cpp` - V2 implementation

### ❌ Current Problem
- The modular files contain **OLD broken code** (two-phase)
- We need to replace with **NEW v2 code** (single-pass recursive)

---

## V2 Design Principles

### 1. Single-Pass Recursive Construction
**OLD (Broken):**
```cpp
void buildBlocks() {
    // Phase 1: Create blocks linearly
    for (each statement) {
        currentBlock->add(stmt);
    }
}
void buildEdges() {
    // Phase 2: Scan to find loop ends (context lost!)
    for (each block) {
        if (isLoopEnd) addBackEdge(); // FAILS for nested loops!
    }
}
```

**NEW (Fixed):**
```cpp
BasicBlock* buildWhile(incoming, contexts...) {
    header = create(); body = create(); exit = create();
    wire(incoming → header);
    wire(header → body [true]); wire(header → exit [false]);
    bodyExit = buildStatements(body, contexts...);  // Recursive!
    wire(bodyExit → header);  // Back-edge created NOW!
    return exit;  // Next statement connects here
}
```

### 2. Context Passing (Not Global Stacks)
**OLD (Broken):**
```cpp
std::vector<LoopContext> m_loopStack;  // Global state!
m_loopStack.push_back(ctx);
// ... later, context might be lost
```

**NEW (Fixed):**
```cpp
struct LoopContext {
    int headerBlockId;
    int exitBlockId;
    LoopContext* outerLoop;  // Linked list, passed as parameter
};

BasicBlock* buildFor(incoming, LoopContext* outerLoop, ...) {
    LoopContext thisLoop;
    thisLoop.outerLoop = outerLoop;
    return buildStatements(body, &thisLoop, ...);  // Pass down!
}
```

### 3. Explicit Edge Wiring
**OLD (Broken):**
```cpp
// Assumes block N flows to N+1 (WRONG for nested structures!)
```

**NEW (Fixed):**
```cpp
// Always explicitly wire every edge:
if (!isTerminated(fromBlock)) {
    addEdge(fromBlock->id, toBlock->id);
}
```

### 4. Black-Box Abstraction
Each builder returns only the exit block. Internal structure is hidden.

---

## Implementation Strategy

### Phase 1: Update Header (cfg_builder.h)
**Replace old class with v2 design:**

```cpp
class CFGBuilder {
public:
    // V2 Context structures (replace global stacks)
    struct LoopContext {
        int headerBlockId;
        int exitBlockId;
        std::string loopType;
        LoopContext* outerLoop;
    };
    
    struct SelectContext {
        int exitBlockId;
        SelectContext* outerSelect;
    };
    
    struct TryContext {
        int catchBlockId;
        int finallyBlockId;
        TryContext* outerTry;
    };
    
    struct SubroutineContext {
        int returnBlockId;
        SubroutineContext* outerSub;
    };

    // Main API (compatible with existing compiler)
    std::unique_ptr<ProgramCFG> build(const Program& program, const SymbolTable& symbols);

private:
    // V2 recursive builder (replaces buildBlocks + buildEdges)
    BasicBlock* buildStatementRange(
        const std::vector<StatementPtr>& statements,
        BasicBlock* incoming,
        LoopContext* loop = nullptr,
        SelectContext* select = nullptr,
        TryContext* tryCtx = nullptr,
        SubroutineContext* sub = nullptr
    );
    
    // Loop builders (single-pass, recursive)
    BasicBlock* buildWhile(const WhileStatement& stmt, BasicBlock* incoming, ...contexts);
    BasicBlock* buildFor(const ForStatement& stmt, BasicBlock* incoming, ...contexts);
    BasicBlock* buildRepeat(const RepeatStatement& stmt, BasicBlock* incoming, ...contexts);
    BasicBlock* buildDo(const DoStatement& stmt, BasicBlock* incoming, ...contexts);
    
    // Conditional builders
    BasicBlock* buildIf(const IfStatement& stmt, BasicBlock* incoming, ...contexts);
    BasicBlock* buildSelectCase(const SelectCaseStatement& stmt, BasicBlock* incoming, ...contexts);
    
    // Exception builder
    BasicBlock* buildTryCatch(const TryStatement& stmt, BasicBlock* incoming, ...contexts);
    
    // Jump handlers
    BasicBlock* handleGoto(const GotoStatement& stmt, BasicBlock* incoming);
    BasicBlock* handleGosub(const GosubStatement& stmt, BasicBlock* incoming, ...contexts);
    BasicBlock* handleReturn(const ReturnStatement& stmt, BasicBlock* incoming, SubroutineContext* sub);
    BasicBlock* handleExit(BasicBlock* incoming, ...contexts);
    
    // Member variables (minimal - no stacks!)
    ProgramCFG* m_programCFG;
    ControlFlowGraph* m_currentCFG;
    int m_nextBlockId;
    std::map<int, int> m_lineNumberToBlock;  // For jump resolution
    std::set<int> m_jumpTargets;  // Pre-scanned jump targets
};
```

### Phase 2: Update Core (cfg_builder_core.cpp)
**Replace with v2 entry point:**

```cpp
std::unique_ptr<ProgramCFG> CFGBuilder::build(const Program& program, const SymbolTable& symbols) {
    m_programCFG = std::make_unique<ProgramCFG>();
    m_currentCFG = m_programCFG->mainCFG.get();
    
    // Phase 0: Pre-scan jump targets (keep this - it's good)
    collectJumpTargets(program);
    
    // Create entry block
    BasicBlock* entryBlock = createBlock("Entry");
    m_currentCFG->entryBlock = entryBlock->id;
    
    // NEW: Single-pass recursive build (no buildBlocks/buildEdges split!)
    BasicBlock* finalBlock = buildFromProgram(program, entryBlock);
    
    // Create exit block
    BasicBlock* exitBlock = createBlock("Exit");
    m_currentCFG->exitBlock = exitBlock->id;
    if (finalBlock && !isTerminated(finalBlock)) {
        addEdge(finalBlock->id, exitBlock->id);
    }
    
    return std::move(m_programCFG);
}

// NEW: Process program lines recursively
BasicBlock* CFGBuilder::buildFromProgram(const Program& program, BasicBlock* incoming) {
    BasicBlock* currentBlock = incoming;
    
    for (const auto& line : program.lines) {
        // Handle jump target landing zones
        if (isJumpTarget(line->lineNumber)) {
            BasicBlock* targetBlock = createBlock("Target_" + std::to_string(line->lineNumber));
            registerLineNumber(line->lineNumber, targetBlock->id);
            if (!isTerminated(currentBlock)) {
                addEdge(currentBlock->id, targetBlock->id);
            }
            currentBlock = targetBlock;
        }
        
        // Process statements recursively
        for (const auto& stmt : line->statements) {
            currentBlock = buildStatement(stmt, currentBlock, nullptr, nullptr, nullptr, nullptr);
        }
    }
    
    return currentBlock;
}

// NEW: Statement dispatcher (routes to specific builders)
BasicBlock* CFGBuilder::buildStatement(
    const Statement* stmt,
    BasicBlock* incoming,
    LoopContext* loop,
    SelectContext* select,
    TryContext* tryCtx,
    SubroutineContext* sub
) {
    switch (stmt->getType()) {
        case ASTNodeType::STMT_WHILE:
            return buildWhile(static_cast<const WhileStatement&>(*stmt), incoming, loop, select, tryCtx, sub);
        case ASTNodeType::STMT_FOR:
            return buildFor(static_cast<const ForStatement&>(*stmt), incoming, loop, select, tryCtx, sub);
        case ASTNodeType::STMT_IF:
            return buildIf(static_cast<const IfStatement&>(*stmt), incoming, loop, select, tryCtx, sub);
        case ASTNodeType::STMT_GOTO:
            return handleGoto(static_cast<const GotoStatement&>(*stmt), incoming);
        // ... etc
        default:
            // Regular statement - add to current block
            addStatementToBlock(incoming, stmt);
            return incoming;
    }
}
```

### Phase 3: Update Loops (cfg_builder_loops.cpp)
**Replace with v2 recursive implementations:**

```cpp
BasicBlock* CFGBuilder::buildWhile(
    const WhileStatement& stmt,
    BasicBlock* incoming,
    LoopContext* outerLoop,
    SelectContext* select,
    TryContext* tryCtx,
    SubroutineContext* sub
) {
    // 1. Create structure
    BasicBlock* headerBlock = createBlock("While_Header");
    BasicBlock* bodyEntry = createBlock("While_Body");
    BasicBlock* exitBlock = createBlock("While_Exit");
    
    // 2. Wire entry
    if (!isTerminated(incoming)) {
        addEdge(incoming->id, headerBlock->id);
    }
    
    // 3. Add WHILE statement to header
    addStatementToBlock(headerBlock, &stmt);
    headerBlock->isLoopHeader = true;
    
    // 4. Wire condition branches
    addConditionalEdge(headerBlock->id, bodyEntry->id, "true");
    addConditionalEdge(headerBlock->id, exitBlock->id, "false");
    
    // 5. Create loop context
    LoopContext thisLoop;
    thisLoop.headerBlockId = headerBlock->id;
    thisLoop.exitBlockId = exitBlock->id;
    thisLoop.loopType = "WHILE";
    thisLoop.outerLoop = outerLoop;
    
    // 6. Recursively build body (KEY: context passed down!)
    BasicBlock* bodyExit = buildStatementRange(
        stmt.body,  // NEW: Parser should populate this!
        bodyEntry,
        &thisLoop,  // Pass our context
        select,
        tryCtx,
        sub
    );
    
    // 7. Wire back-edge (NOW, not in separate phase!)
    if (!isTerminated(bodyExit)) {
        addEdge(bodyExit->id, headerBlock->id);
    }
    
    // 8. Return exit (next statement connects here)
    return exitBlock;
}

BasicBlock* CFGBuilder::buildFor(
    const ForStatement& stmt,
    BasicBlock* incoming,
    LoopContext* outerLoop,
    SelectContext* select,
    TryContext* tryCtx,
    SubroutineContext* sub
) {
    // Similar structure to WHILE:
    // 1. Create: init, check, body, increment, exit
    BasicBlock* initBlock = createBlock("For_Init");
    BasicBlock* checkBlock = createBlock("For_Check");
    BasicBlock* bodyBlock = createBlock("For_Body");
    BasicBlock* incrBlock = createBlock("For_Incr");
    BasicBlock* exitBlock = createBlock("For_Exit");
    
    // 2. Wire: incoming → init → check → body → incr → check
    //                                  ↓ (false)
    //                                exit
    
    // 3. Create context
    LoopContext thisLoop;
    thisLoop.headerBlockId = checkBlock->id;
    thisLoop.exitBlockId = exitBlock->id;
    thisLoop.loopType = "FOR";
    thisLoop.outerLoop = outerLoop;
    
    // 4. Recursively build body
    BasicBlock* bodyExit = buildStatementRange(stmt.body, bodyBlock, &thisLoop, select, tryCtx, sub);
    
    // 5. Wire edges
    addEdge(bodyExit->id, incrBlock->id);
    addEdge(incrBlock->id, checkBlock->id);  // Back-edge!
    
    return exitBlock;
}

BasicBlock* CFGBuilder::buildRepeat(
    const RepeatStatement& stmt,
    BasicBlock* incoming,
    LoopContext* outerLoop,
    SelectContext* select,
    TryContext* tryCtx,
    SubroutineContext* sub
) {
    // REPEAT is post-test: body executes at least once
    BasicBlock* bodyBlock = createBlock("Repeat_Body");
    BasicBlock* checkBlock = createBlock("Repeat_Check");
    BasicBlock* exitBlock = createBlock("Repeat_Exit");
    
    // Wire entry
    if (!isTerminated(incoming)) {
        addEdge(incoming->id, bodyBlock->id);
    }
    
    // Create context
    LoopContext thisLoop;
    thisLoop.headerBlockId = bodyBlock->id;  // Body is header for post-test
    thisLoop.exitBlockId = exitBlock->id;
    thisLoop.loopType = "REPEAT";
    thisLoop.outerLoop = outerLoop;
    
    // Build body recursively
    BasicBlock* bodyExit = buildStatementRange(stmt.body, bodyBlock, &thisLoop, select, tryCtx, sub);
    
    // Wire: body → check → (false) body | (true) exit
    if (!isTerminated(bodyExit)) {
        addEdge(bodyExit->id, checkBlock->id);
    }
    addConditionalEdge(checkBlock->id, bodyBlock->id, "false");  // Back-edge
    addConditionalEdge(checkBlock->id, exitBlock->id, "true");
    
    return exitBlock;
}

BasicBlock* CFGBuilder::buildDo(
    const DoStatement& stmt,
    BasicBlock* incoming,
    LoopContext* outerLoop,
    SelectContext* select,
    TryContext* tryCtx,
    SubroutineContext* sub
) {
    // DO has variants: DO WHILE/UNTIL (pre-test) or DO...LOOP WHILE/UNTIL (post-test)
    // Similar to WHILE/REPEAT depending on variant
    // (Implementation depends on stmt.preTest flag)
    
    if (stmt.preTest) {
        // Like WHILE
        // ...
    } else {
        // Like REPEAT
        // ...
    }
    
    return exitBlock;
}
```

### Phase 4: Update Conditional (cfg_builder_conditional.cpp)
**Use v2 IF implementation:**

```cpp
BasicBlock* CFGBuilder::buildIf(
    const IfStatement& stmt,
    BasicBlock* incoming,
    LoopContext* loop,
    SelectContext* select,
    TryContext* tryCtx,
    SubroutineContext* sub
) {
    if (stmt.hasGoto) {
        // Single-line IF THEN GOTO
        BasicBlock* thenTarget = resolveGotoTarget(stmt.gotoLine);
        BasicBlock* nextBlock = createBlock("After_IfGoto");
        
        addConditionalEdge(incoming->id, thenTarget->id, "true");
        addConditionalEdge(incoming->id, nextBlock->id, "false");
        
        return nextBlock;
    }
    
    // Multi-line IF
    BasicBlock* thenBlock = createBlock("If_Then");
    BasicBlock* elseBlock = createBlock("If_Else");
    BasicBlock* mergeBlock = createBlock("If_Merge");
    
    // Wire condition
    addConditionalEdge(incoming->id, thenBlock->id, "true");
    addConditionalEdge(incoming->id, elseBlock->id, "false");
    
    // Recursively build THEN branch
    BasicBlock* thenExit = buildStatementRange(stmt.thenBlock, thenBlock, loop, select, tryCtx, sub);
    
    // Recursively build ELSE branch
    BasicBlock* elseExit = buildStatementRange(stmt.elseBlock, elseBlock, loop, select, tryCtx, sub);
    
    // Wire to merge
    if (!isTerminated(thenExit)) {
        addEdge(thenExit->id, mergeBlock->id);
    }
    if (!isTerminated(elseExit)) {
        addEdge(elseExit->id, mergeBlock->id);
    }
    
    return mergeBlock;
}
```

### Phase 5: Update Jumps (cfg_builder_jumps.cpp)
**Handle EXIT using contexts:**

```cpp
BasicBlock* CFGBuilder::handleExit(
    const ExitStatement& stmt,
    BasicBlock* incoming,
    LoopContext* loop,
    SelectContext* select
) {
    switch (stmt.exitType) {
        case ExitType::FOR_LOOP:
        case ExitType::WHILE_LOOP:
        case ExitType::DO_LOOP: {
            if (!loop) {
                // Error: EXIT outside loop
                return incoming;
            }
            // Jump to loop exit
            addEdge(incoming->id, loop->exitBlockId);
            markTerminated(incoming);
            
            // Create unreachable block for subsequent statements
            return createBlock("After_Exit");
        }
        
        case ExitType::SELECT: {
            if (!select) {
                // Error: EXIT SELECT outside SELECT
                return incoming;
            }
            addEdge(incoming->id, select->exitBlockId);
            markTerminated(incoming);
            return createBlock("After_ExitSelect");
        }
    }
}
```

---

## Key Differences From Old Code

| Aspect | OLD | NEW V2 |
|--------|-----|--------|
| **Phases** | buildBlocks() + buildEdges() | buildStatementRange() (single pass) |
| **Loop Back-edges** | Added in buildEdges() | Added immediately in buildWhile/For/etc |
| **Context** | Global m_loopStack | LoopContext* parameter |
| **Nesting** | Breaks with nested loops | Recursion handles naturally |
| **Edge Wiring** | Implicit fallthrough | Explicit addEdge() calls |
| **Statement Bodies** | Scanned later | Recursively built immediately |

---

## Implementation Order

1. **Update cfg_builder.h** - Add v2 context structures, update API
2. **Update cfg_builder_core.cpp** - Single-pass entry point
3. **Update cfg_builder_statements.cpp** - buildStatementRange() recursive dispatcher
4. **Update cfg_builder_loops.cpp** - buildWhile/For/Repeat/Do with recursion
5. **Update cfg_builder_conditional.cpp** - buildIf with recursion
6. **Update cfg_builder_jumps.cpp** - handleExit with context parameters
7. **Update cfg_builder_exception.cpp** - buildTryCatch with recursion
8. **Delete cfg_builder_edges.cpp** - No longer needed! (single-pass eliminates this)
9. **Test** - Run nested loop test suite
10. **Verify** - All 6 nested tests should pass

---

## Success Criteria

✅ All nested loop tests pass:
- test_nested_while_if.bas
- test_nested_if_while.bas
- test_nested_for_if.bas
- **test_nested_repeat_if.bas** (currently fails)
- **test_nested_do_if.bas** (currently fails)
- **test_nested_mixed_controls.bas** (currently fails)

✅ No infinite loops in nested structures
✅ Proper back-edges created immediately
✅ Context preserved through recursion
✅ Clean modular code structure

---

## Next Steps

Start with **cfg_builder.h** - add v2 context structures and update the class interface to match v2 design.