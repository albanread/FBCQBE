# CFG Builder Refactoring Plan

**Date:** February 1, 2025  
**Status:** Planning Phase  
**Priority:** HIGH - Critical architectural refactoring  
**Impact:** Fixes all nested control flow bugs permanently

---

## Executive Summary

The current CFG builder has a fundamental architectural flaw: it uses a two-phase approach (build blocks, then connect edges) with implicit assumptions about sequential block ordering. This causes failures with nested control structures, particularly REPEAT and DO loops inside IF ELSE branches.

**Solution:** Rewrite the CFG builder using single-pass recursive construction where each control structure is fully wired at creation time, eliminating the need for forward scanning and implicit fallthrough assumptions.

---

## Current Architecture Problems

### Problem 1: Implicit Sequential Ordering

**Current Code Pattern:**
```cpp
// Assumes execution flows to block->id + 1
if (block->successors.empty() && 
    block->id + 1 < static_cast<int>(m_currentCFG->blocks.size())) {
    addFallthroughEdge(block->id, block->id + 1);
}
```

**Why It Fails:**
- In nested structures, block N+1 might be the start of an inner IF block
- The logical flow should jump to an outer loop's incrementor
- Physical block order ≠ logical execution order

**Evidence from Tests:**
```basic
IF condition THEN
    ' Block 5
ELSE
    REPEAT          ' Block 6 (header should be here)
        PRINT x     ' Block 7 (body)
        x = x + 1   ' Still Block 7 or Block 8?
    UNTIL x > 5     ' Block 9 (back to 6?)
END IF              ' Block 10
' Next statement   ' Block 11

' Bug: Code assumes block 7 flows to block 8
' Reality: Block 7 should flow back to block 6
' But block 8 might be the outer merge!
```

### Problem 2: Two-Phase Construction

**Current Approach:**
```cpp
// Phase 1: Build all blocks
void buildBlocks(statements) {
    for (stmt : statements) {
        switch (stmt->type) {
            case WHILE:
                currentBlock->addStatement(stmt);
                break;
            case REPEAT:
                currentBlock->addStatement(stmt);
                break;
        }
    }
}

// Phase 2: Connect edges by scanning
void buildEdges() {
    for (block : blocks) {
        for (stmt : block->statements) {
            if (stmt->type == WHILE) {
                // Scan forward to find matching WEND
                int wendBlock = findMatchingWend(block->id);
                addBackEdge(wendBlock, block->id);
            }
        }
    }
}
```

**Why It Fails:**
1. By Phase 2, structural context is lost
2. Must re-parse to understand nesting
3. Scanning is O(n²) and error-prone
4. Nested structures break the scanning heuristics

**Specific Failure Mode (from our tests):**
```
Test 2: REPEAT in IF ELSE - INFINITE LOOP
Output: "Else branch: 10" repeated forever

Analysis:
- Phase 1: REPEAT statement added to block 6
- Phase 1: PRINT added to block 7  
- Phase 1: Assignment (x = x + 1) added to... where?
- Phase 2: Tries to find UNTIL to create back-edge
- Phase 2: Can't determine which block the assignment went to
- Result: Assignment never executes, infinite loop
```

### Problem 3: Global State Management

**Current Code:**
```cpp
class CFGBuilder {
    std::stack<int> m_loopStack;
    std::stack<int> m_ifStack;
    std::stack<int> m_selectCaseStack;
    BasicBlock* m_currentBlock;  // Global pointer
    bool m_processingNestedStatements;  // Special case flag
};
```

**Why It Fails:**
1. Manual push/pop is error-prone
2. Easy to corrupt stack with irregular exits (GOTO, RETURN)
3. Special flags like `m_processingNestedStatements` indicate leaky abstraction
4. Deep nesting increases risk of synchronization bugs

**Evidence:**
The January 2025 fix added `processNestedStatements()` which sets a special flag. This indicates the architecture wasn't designed for recursion.

### Problem 4: Inconsistent Loop Processing

**Current State:**
```cpp
// WHILE and FOR: Have dedicated processing functions
void processWhileStatement(WhileStatement& stmt) {
    // Creates proper loop structure
    // Jan 2025 fix made this work recursively
}

void processForStatement(ForStatement& stmt) {
    // Creates proper loop structure
    // Jan 2025 fix made this work recursively
}

// REPEAT and DO: Handled in two-phase approach
void buildBlocks() {
    case REPEAT:
        m_currentBlock->addStatement(stmt);  // Just add to block
        break;
}

void buildEdges() {
    case REPEAT:
        // Try to retroactively create structure
        // But nested context already lost!
        break;
}
```

**Why This Explains Our Test Results:**
- ✅ WHILE/FOR pass: Single-phase processing (fixed Jan 2025)
- ❌ REPEAT/DO fail: Still using two-phase approach
- ❌ Mixed nesting fails: Combining both approaches breaks

---

## Architectural Insights

### The "Dominoes vs. Railroad Tracks" Metaphor

**Current Approach (Dominoes):**
```
Create blocks like dominoes in a line:
[1] [2] [3] [4] [5] [6] [7] [8]

Hope that knocking over block N hits block N+1

Problem: When you insert a nested loop:
[1] [2] [3] [4a] [4b] [4c] [5] [6]
              └─ nested loop ─┘

The numbering changes, fallthrough assumptions break
```

**Better Approach (Railroad Tracks):**
```
Build complete circuits immediately:

buildLoop(incoming):
    header = create("LoopHeader")
    body = create("LoopBody")
    exit = create("LoopExit")
    
    connect(incoming → header)
    connect(header → body [true])
    connect(header → exit [false])
    
    body_end = buildStatements(body)
    connect(body_end → header)  // Back-edge
    
    return exit  // Next statement connects here

Outer code doesn't care about internal complexity.
It only sees: [incoming] → buildLoop() → [exit]
```

### Single-Pass Recursive Construction

**Key Principle:** Each control structure is a **black box** that:
1. Accepts an incoming block
2. Creates its internal structure
3. Returns an outgoing block

```
buildIf(incoming) → outgoing
buildWhile(incoming) → outgoing  
buildFor(incoming) → outgoing
buildRepeat(incoming) → outgoing

// Composition:
block1 = buildIf(entry)
block2 = buildWhile(block1)
block3 = buildFor(block2)
exit = block3
```

**Benefits:**
- No global state needed
- No scanning/searching
- Automatic handling of arbitrary nesting
- Context is explicit (function parameters)
- Back-edges created immediately

### Context Passing Instead of Global Stacks

**Current (Global Stack):**
```cpp
void processExitFor() {
    if (m_loopStack.empty()) error();
    int exitBlock = m_loopStack.top().exitBlock;
    // What if we're in nested FOR inside IF inside WHILE?
    // Which loop does this exit?
}
```

**Better (Context Parameter):**
```cpp
struct LoopContext {
    int headerBlock;
    int exitBlock;
    LoopContext* outer;  // Link to outer loop context
};

BasicBlock* buildStatement(stmt, incoming, LoopContext* currentLoop) {
    if (stmt->type == EXIT_FOR) {
        if (!currentLoop) error();
        connect(incoming, currentLoop->exitBlock);
        return createDeadBlock();  // No fallthrough
    }
    
    if (stmt->type == FOR) {
        LoopContext newContext;
        newContext.outer = currentLoop;  // Link to outer
        return buildFor(stmt, incoming, &newContext);
    }
}
```

**Benefits:**
- Context is explicit, not global
- Natural stack via function call stack
- Impossible to corrupt (falls out of scope on return)
- Easy to find outer loop (follow `outer` pointer)

---

## Refactoring Plan

### Phase 1: Design New Architecture (Week 1)

#### 1.1 Define Core Abstractions

**File:** `fsh/FasterBASICT/src/fasterbasic_cfg_v2.h`

```cpp
class CFGBuilderV2 {
public:
    struct LoopContext {
        int headerBlockId;
        int exitBlockId;
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
    
    // Main entry point
    void build(const std::vector<StatementPtr>& statements);
    
private:
    // Core recursive builder
    // Returns the "exit" block where next statement should connect
    BasicBlock* buildStatementRange(
        const std::vector<StatementPtr>& statements,
        BasicBlock* incoming,
        LoopContext* currentLoop = nullptr,
        SelectContext* currentSelect = nullptr,
        TryContext* currentTry = nullptr
    );
    
    // Individual structure builders
    BasicBlock* buildIf(const IfStatement& stmt, BasicBlock* incoming, 
                       LoopContext* loop, SelectContext* select, TryContext* tryCtx);
    BasicBlock* buildWhile(const WhileStatement& stmt, BasicBlock* incoming,
                          LoopContext* outerLoop, SelectContext* select, TryContext* tryCtx);
    BasicBlock* buildFor(const ForStatement& stmt, BasicBlock* incoming,
                        LoopContext* outerLoop, SelectContext* select, TryContext* tryCtx);
    BasicBlock* buildRepeat(const RepeatStatement& stmt, BasicBlock* incoming,
                           LoopContext* outerLoop, SelectContext* select, TryContext* tryCtx);
    BasicBlock* buildDo(const DoStatement& stmt, BasicBlock* incoming,
                       LoopContext* outerLoop, SelectContext* select, TryContext* tryCtx);
    BasicBlock* buildSelectCase(const SelectCaseStatement& stmt, BasicBlock* incoming,
                               LoopContext* loop, SelectContext* outerSelect, TryContext* tryCtx);
    BasicBlock* buildTryCatch(const TryStatement& stmt, BasicBlock* incoming,
                             LoopContext* loop, SelectContext* select, TryContext* outerTry);
    
    // Terminator handlers (GOTO, RETURN, EXIT FOR, etc.)
    BasicBlock* handleGoto(const GotoStatement& stmt, BasicBlock* incoming);
    BasicBlock* handleReturn(const ReturnStatement& stmt, BasicBlock* incoming);
    BasicBlock* handleExitFor(BasicBlock* incoming, LoopContext* loop);
    BasicBlock* handleExitWhile(BasicBlock* incoming, LoopContext* loop);
    BasicBlock* handleExitDo(BasicBlock* incoming, LoopContext* loop);
    BasicBlock* handleExitSelect(BasicBlock* incoming, SelectContext* select);
    
    // Helper functions
    BasicBlock* createBlock(const std::string& name = "");
    void addEdge(int fromId, int toId, const std::string& label = "");
    BasicBlock* createUnreachableBlock();  // For dead code after terminators
    
    // No more global stacks!
    ControlFlowGraph* m_currentCFG;
};
```

#### 1.2 Define Block Connection Strategy

**Key Insight:** Every `build*()` function follows this contract:

```cpp
// CONTRACT:
// - Takes an incoming block (where control enters)
// - Creates internal structure
// - Wires all internal edges
// - Returns outgoing block (where control exits)
// - If control never exits (e.g., infinite loop, GOTO), returns unreachable block
```

**Example Signatures:**
```cpp
// All follow the same pattern:
BasicBlock* buildX(const XStatement& stmt, 
                   BasicBlock* incoming,
                   ...contexts...) {
    // 1. Create needed blocks
    // 2. Wire incoming to first block
    // 3. Recursively build internal structures
    // 4. Wire everything together
    // 5. Return the exit block
}
```

### Phase 2: Implement Core Structures (Week 1-2)

#### 2.1 Implement IF Statement

**File:** `fsh/FasterBASICT/src/cfg_v2/cfg_if.cpp`

```cpp
BasicBlock* CFGBuilderV2::buildIf(
    const IfStatement& stmt,
    BasicBlock* incoming,
    LoopContext* loop,
    SelectContext* select,
    TryContext* tryCtx
) {
    // 1. Setup blocks
    BasicBlock* conditionBlock = incoming;  // Or split if incoming has statements
    BasicBlock* thenEntry = createBlock("If_Then");
    BasicBlock* elseEntry = stmt.elseStatements.empty() ? nullptr : createBlock("If_Else");
    BasicBlock* mergeBlock = createBlock("If_Merge");
    
    // 2. Add condition check to header
    conditionBlock->addStatement(&stmt);  // The IF condition itself
    
    // 3. Wire condition to branches
    addEdge(conditionBlock->id, thenEntry->id, "true");
    if (elseEntry) {
        addEdge(conditionBlock->id, elseEntry->id, "false");
    } else {
        addEdge(conditionBlock->id, mergeBlock->id, "false");
    }
    
    // 4. Recursively build THEN branch
    BasicBlock* thenExit = buildStatementRange(
        stmt.thenStatements,
        thenEntry,
        loop,
        select,
        tryCtx
    );
    
    // 5. Wire THEN exit to merge (if not terminated)
    if (!thenExit->isTerminated) {
        addEdge(thenExit->id, mergeBlock->id);
    }
    
    // 6. Recursively build ELSE branch (if exists)
    if (elseEntry) {
        BasicBlock* elseExit = buildStatementRange(
            stmt.elseStatements,
            elseEntry,
            loop,
            select,
            tryCtx
        );
        
        if (!elseExit->isTerminated) {
            addEdge(elseExit->id, mergeBlock->id);
        }
    }
    
    // 7. Handle ELSEIF clauses
    // TODO: Similar recursive pattern
    
    // 8. Return merge point
    return mergeBlock;
}
```

**Key Points:**
- ✅ No scanning forward for END IF
- ✅ No global state
- ✅ Nested IFs handled automatically by recursion
- ✅ All edges created immediately
- ✅ Returns explicit merge point

#### 2.2 Implement WHILE Loop

**File:** `fsh/FasterBASICT/src/cfg_v2/cfg_while.cpp`

```cpp
BasicBlock* CFGBuilderV2::buildWhile(
    const WhileStatement& stmt,
    BasicBlock* incoming,
    LoopContext* outerLoop,
    SelectContext* select,
    TryContext* tryCtx
) {
    // 1. Create loop blocks
    BasicBlock* headerBlock = createBlock("While_Header");
    BasicBlock* bodyEntry = createBlock("While_Body");
    BasicBlock* exitBlock = createBlock("While_Exit");
    
    // 2. Wire entry
    if (!incoming->isTerminated) {
        addEdge(incoming->id, headerBlock->id);
    }
    
    // 3. Add condition to header
    headerBlock->addStatement(&stmt);
    
    // 4. Wire header branches
    addEdge(headerBlock->id, bodyEntry->id, "true");
    addEdge(headerBlock->id, exitBlock->id, "false");
    
    // 5. Create context for this loop
    LoopContext thisLoop;
    thisLoop.headerBlockId = headerBlock->id;
    thisLoop.exitBlockId = exitBlock->id;
    thisLoop.outerLoop = outerLoop;  // Link to outer loop
    
    // 6. Recursively build body
    // This handles nested loops, IFs, etc. automatically!
    BasicBlock* bodyExit = buildStatementRange(
        stmt.bodyStatements,
        bodyEntry,
        &thisLoop,  // Pass our context
        select,
        tryCtx
    );
    
    // 7. Wire back-edge (THE KEY FIX!)
    if (!bodyExit->isTerminated) {
        addEdge(bodyExit->id, headerBlock->id);  // Loop back
    }
    
    // 8. Return exit block
    return exitBlock;
}
```

**This Fixes:**
- ❌ Old: Scanned forward to find WEND, O(n) search
- ✅ New: WEND is implicit (end of stmt.bodyStatements), O(1)
- ❌ Old: Back-edge added in separate phase, easy to miss
- ✅ New: Back-edge added immediately, impossible to forget
- ❌ Old: EXIT WHILE had to search global stack
- ✅ New: EXIT WHILE gets explicit context parameter

#### 2.3 Implement REPEAT Loop

**File:** `fsh/FasterBASICT/src/cfg_v2/cfg_repeat.cpp`

```cpp
BasicBlock* CFGBuilderV2::buildRepeat(
    const RepeatStatement& stmt,
    BasicBlock* incoming,
    LoopContext* outerLoop,
    SelectContext* select,
    TryContext* tryCtx
) {
    // REPEAT is a post-test loop: Body always executes at least once
    
    // 1. Create blocks
    BasicBlock* bodyEntry = createBlock("Repeat_Body");
    BasicBlock* conditionBlock = createBlock("Repeat_Condition");
    BasicBlock* exitBlock = createBlock("Repeat_Exit");
    
    // 2. Wire entry (goes directly to body)
    if (!incoming->isTerminated) {
        addEdge(incoming->id, bodyEntry->id);
    }
    
    // 3. Create loop context
    LoopContext thisLoop;
    thisLoop.headerBlockId = bodyEntry->id;  // For REPEAT, body is "header"
    thisLoop.exitBlockId = exitBlock->id;
    thisLoop.outerLoop = outerLoop;
    
    // 4. Recursively build body
    BasicBlock* bodyExit = buildStatementRange(
        stmt.bodyStatements,
        bodyEntry,
        &thisLoop,
        select,
        tryCtx
    );
    
    // 5. Wire to condition check
    if (!bodyExit->isTerminated) {
        addEdge(bodyExit->id, conditionBlock->id);
    }
    
    // 6. Add UNTIL condition
    conditionBlock->addStatement(&stmt);  // UNTIL condition
    
    // 7. Wire condition: UNTIL means "exit when true, loop when false"
    addEdge(conditionBlock->id, exitBlock->id, "true");
    addEdge(conditionBlock->id, bodyEntry->id, "false");  // Loop back
    
    // 8. Return exit
    return exitBlock;
}
```

**This Fixes the Infinite Loop Bug:**

Current behavior (BROKEN):
```
Phase 1: Add REPEAT to block 6
Phase 1: Add PRINT to block 7
Phase 1: Add assignment to... ??? (gets lost)
Phase 2: Try to find UNTIL, can't find assignment
Result: Assignment never executes, infinite loop
```

New behavior (FIXED):
```
buildRepeat():
  bodyEntry = block 6
  buildStatementRange([PRINT, assignment, ...], bodyEntry)
    - PRINT added to block 6
    - assignment added to block 6
  conditionBlock = block 7
  wire: block 6 -> block 7
  wire: block 7 -> block 6 (back-edge)
Result: All statements in body, back-edge correct
```

#### 2.4 Implement DO Loop (All Variants)

**File:** `fsh/FasterBASICT/src/cfg_v2/cfg_do.cpp`

```cpp
BasicBlock* CFGBuilderV2::buildDo(
    const DoStatement& stmt,
    BasicBlock* incoming,
    LoopContext* outerLoop,
    SelectContext* select,
    TryContext* tryCtx
) {
    // DO has 4 variants:
    // 1. DO WHILE condition ... LOOP (pre-test, while)
    // 2. DO UNTIL condition ... LOOP (pre-test, until)
    // 3. DO ... LOOP WHILE condition (post-test, while)
    // 4. DO ... LOOP UNTIL condition (post-test, until)
    
    if (stmt.isPreTest) {
        // Pre-test: Like WHILE/FOR
        BasicBlock* headerBlock = createBlock("Do_Header");
        BasicBlock* bodyEntry = createBlock("Do_Body");
        BasicBlock* exitBlock = createBlock("Do_Exit");
        
        if (!incoming->isTerminated) {
            addEdge(incoming->id, headerBlock->id);
        }
        
        headerBlock->addStatement(&stmt);
        
        // WHILE: continue if true, exit if false
        // UNTIL: exit if true, continue if false
        if (stmt.isWhileCondition) {
            addEdge(headerBlock->id, bodyEntry->id, "true");
            addEdge(headerBlock->id, exitBlock->id, "false");
        } else {
            addEdge(headerBlock->id, exitBlock->id, "true");
            addEdge(headerBlock->id, bodyEntry->id, "false");
        }
        
        LoopContext thisLoop;
        thisLoop.headerBlockId = headerBlock->id;
        thisLoop.exitBlockId = exitBlock->id;
        thisLoop.outerLoop = outerLoop;
        
        BasicBlock* bodyExit = buildStatementRange(
            stmt.bodyStatements, bodyEntry, &thisLoop, select, tryCtx
        );
        
        if (!bodyExit->isTerminated) {
            addEdge(bodyExit->id, headerBlock->id);
        }
        
        return exitBlock;
        
    } else {
        // Post-test: Like REPEAT
        BasicBlock* bodyEntry = createBlock("Do_Body");
        BasicBlock* conditionBlock = createBlock("Do_Condition");
        BasicBlock* exitBlock = createBlock("Do_Exit");
        
        if (!incoming->isTerminated) {
            addEdge(incoming->id, bodyEntry->id);
        }
        
        LoopContext thisLoop;
        thisLoop.headerBlockId = bodyEntry->id;
        thisLoop.exitBlockId = exitBlock->id;
        thisLoop.outerLoop = outerLoop;
        
        BasicBlock* bodyExit = buildStatementRange(
            stmt.bodyStatements, bodyEntry, &thisLoop, select, tryCtx
        );
        
        if (!bodyExit->isTerminated) {
            addEdge(bodyExit->id, conditionBlock->id);
        }
        
        conditionBlock->addStatement(&stmt);
        
        // LOOP WHILE: continue if true, exit if false
        // LOOP UNTIL: exit if true, continue if false
        if (stmt.isWhileCondition) {
            addEdge(conditionBlock->id, bodyEntry->id, "true");
            addEdge(conditionBlock->id, exitBlock->id, "false");
        } else {
            addEdge(conditionBlock->id, exitBlock->id, "true");
            addEdge(conditionBlock->id, bodyEntry->id, "false");
        }
        
        return exitBlock;
    }
}
```

### Phase 3: Handle Special Cases (Week 2)

#### 3.1 Terminators (GOTO, RETURN, EXIT)

```cpp
BasicBlock* CFGBuilderV2::handleGoto(
    const GotoStatement& stmt,
    BasicBlock* incoming
) {
    // 1. Add statement to incoming block
    incoming->addStatement(&stmt);
    
    // 2. Wire to target (resolve label later)
    int targetId = resolveLabelToBlockId(stmt.targetLabel);
    addEdge(incoming->id, targetId);
    
    // 3. Mark as terminated
    incoming->isTerminated = true;
    
    // 4. Return unreachable block (subsequent statements go here)
    return createUnreachableBlock();
}

BasicBlock* CFGBuilderV2::handleExitFor(
    BasicBlock* incoming,
    LoopContext* loop
) {
    if (!loop) {
        error("EXIT FOR outside of FOR loop");
    }
    
    // Jump to loop exit
    addEdge(incoming->id, loop->exitBlockId);
    incoming->isTerminated = true;
    
    return createUnreachableBlock();
}
```

#### 3.2 FOR Loop with EXIT FOR

```cpp
BasicBlock* CFGBuilderV2::buildFor(
    const ForStatement& stmt,
    BasicBlock* incoming,
    LoopContext* outerLoop,
    SelectContext* select,
    TryContext* tryCtx
) {
    // Setup blocks
    BasicBlock* initBlock = createBlock("For_Init");
    BasicBlock* headerBlock = createBlock("For_Header");
    BasicBlock* bodyEntry = createBlock("For_Body");
    BasicBlock* incrementBlock = createBlock("For_Increment");
    BasicBlock* exitBlock = createBlock("For_Exit");
    
    // Wire entry and initialization
    if (!incoming->isTerminated) {
        addEdge(incoming->id, initBlock->id);
    }
    initBlock->addStatement(&stmt);  // var = start
    addEdge(initBlock->id, headerBlock->id);
    
    // Condition check
    headerBlock->addStatement(&stmt);  // check condition
    addEdge(headerBlock->id, bodyEntry->id, "true");
    addEdge(headerBlock->id, exitBlock->id, "false");
    
    // Create loop context with exit
    LoopContext thisLoop;
    thisLoop.headerBlockId = headerBlock->id;
    thisLoop.exitBlockId = exitBlock->id;  // EXIT FOR jumps here
    thisLoop.outerLoop = outerLoop;
    
    // Build body (may contain EXIT FOR)
    BasicBlock* bodyExit = buildStatementRange(
        stmt.bodyStatements,
        bodyEntry,
        &thisLoop,  // EXIT FOR can access this
        select,
        tryCtx
    );
    
    // Wire to increment
    if (!bodyExit->isTerminated) {
        addEdge(bodyExit->id, incrementBlock->id);
    }
    
    // Increment and loop back
    incrementBlock->addStatement(&stmt);  // var = var + step
    addEdge(incrementBlock->id, headerBlock->id);
    
    return exitBlock;
}
```

#### 3.3 SELECT CASE

```cpp
BasicBlock* CFGBuilderV2::buildSelectCase(
    const SelectCaseStatement& stmt,
    BasicBlock* incoming,
    LoopContext* loop,
    SelectContext* outerSelect,
    TryContext* tryCtx
) {
    // SELECT creates a dispatch structure
    BasicBlock* dispatchBlock = incoming;
    BasicBlock* mergeBlock = createBlock("Select_Merge");
    
    dispatchBlock->addStatement(&stmt);
    
    // Create context for EXIT SELECT
    SelectContext thisSelect;
    thisSelect.exitBlockId = mergeBlock->id;
    thisSelect.outerSelect = outerSelect;
    
    // Build each CASE branch
    for (const auto& caseClause : stmt.cases) {
        BasicBlock* caseEntry = createBlock("Case_Branch");
        addEdge(dispatchBlock->id, caseEntry->id, caseClause.condition);
        
        BasicBlock* caseExit = buildStatementRange(
            caseClause.statements,
            caseEntry,
            loop,
            &thisSelect,  // EXIT SELECT can access this
            tryCtx
        );
        
        if (!caseExit->isTerminated) {
            addEdge(caseExit->id, mergeBlock->id);
        }
    }
    
    // CASE ELSE (default)
    if (stmt.hasDefaultCase) {
        BasicBlock* defaultEntry = createBlock("Case_Default");
        addEdge(dispatchBlock->id, defaultEntry->id, "default");
        
        BasicBlock* defaultExit = buildStatementRange(
            stmt.defaultStatements,
            defaultEntry,
            loop,
            &thisSelect,
            tryCtx
        );
        
        if (!defaultExit->isTerminated) {
            addEdge(defaultExit->id, mergeBlock->id);
        }
    }
    
    return mergeBlock;
}
```

### Phase 4: Migration Strategy (Week 3)

#### 4.1 Parallel Implementation

**Approach:** Keep old CFG builder, build new one alongside

```
fsh/FasterBASICT/src/
├── fasterbasic_cfg.cpp          (OLD - keep for now)
├── fasterbasic_cfg.h            (OLD - keep for now)
└── cfg_v2/
    ├── cfg_builder_v2.cpp       (NEW - main builder)
    ├── cfg_builder_v2.h         (NEW - interface)
    ├── cfg_if.cpp               (NEW - IF implementation)
    ├── cfg_while.cpp            (NEW - WHILE implementation)
    ├── cfg_for.cpp              (NEW - FOR implementation)
    ├── cfg_repeat.cpp           (NEW - REPEAT implementation)
    ├── cfg_do.cpp               (NEW - DO implementation)
    ├── cfg_select.cpp           (NEW - SELECT implementation)
    ├── cfg_try.cpp              (NEW - TRY/CATCH implementation)
    └── cfg_terminators.cpp      (NEW - GOTO/RETURN/EXIT)
```

#### 4.2 Feature Flag

```cpp
// In compiler options
struct CompilerOptions {
    bool useNewCFGBuilder = false;  // Start false
};

// In compilation entry point
if (options.useNewCFGBuilder) {
    CFGBuilderV2 builder;
    builder.build(statements);
} else {
    CFGBuilder builder;  // Old implementation
    builder.buildBlocks(statements);
    builder.buildEdges();
}
```

#### 4.3 Testing Strategy

**Week 3: Test new builder with nested test suite**

```bash
# Test with new CFG builder
./qbe_basic --use-new-cfg -o /tmp/test tests/loops/test_nested_repeat_if.bas
/tmp/test
# Expected: PASS (infinite loop fixed!)

# Run all nested tests
USE_NEW_CFG=1 ./scripts/test_nested_control_flow.sh
# Expected: 6/6 PASS
```

**Week 4: Test with full test suite**

```bash
# Run all tests with new CFG builder
./run_tests.sh --new-cfg
# Expected: All existing tests still pass
#           + Fixed tests now pass
```

### Phase 5: Validation & Deployment (Week 4)

#### 5.1 Validation Checklist

- [ ] All nested control flow tests pass (6/6)
- [ ] All existing tests still pass (no regressions)
- [ ] Performance acceptable (compile time < 10% increase)
- [ ] Memory usage acceptable (< 20% increase)
- [ ] CFG traces show correct structure
- [ ] Code coverage > 90% for new builder

#### 5.2 Documentation Updates

- [ ] Update `docs/design/ControlFlowGraph.md`
- [ ] Document new architecture in comments
- [ ] Add examples of each control structure
- [ ] Update developer guide

#### 5.3 Gradual Rollout

**Week 4:**
- Enable new CFG builder by default
- Keep old builder available via flag
- Monitor for issues

**Week 5:**
- If stable, deprecate old builder
- Remove feature flag
- Delete old implementation

**Week 6:**
- Final cleanup
- Performance optimization
- Documentation finalization

---

## Expected Outcomes

### Before Refactoring

**Test Results:**
- ✅ test_nested_while_if.bas - PASS (3/6 tests)
- ✅ test_nested_if_while.bas - PASS
- ✅ test_nested_for_if.bas - PASS
- ❌ test_nested_repeat_if.bas - FAIL (infinite loop)
- ❌ test_nested_do_if.bas - FAIL (infinite loop)
- ❌ test_nested_mixed_controls.bas - FAIL (infinite loop)

**Pass Rate:** 50%

### After Refactoring

**Test Results:**
- ✅ test_nested_while_if.bas - PASS (6/6 tests)
- ✅ test_nested_if_while.bas - PASS
- ✅ test_nested_for_if.bas - PASS
- ✅ test_nested_repeat_if.bas - PASS ← FIXED
- ✅ test_nested_do_if.bas - PASS ← FIXED
- ✅ test_nested_mixed_controls.bas - PASS ← FIXED

**Pass Rate:** 100%

### Code Quality Improvements

**Before:**
```cpp
// Complex, error-prone
void buildBlocks() { /* 500+ lines */ }
void buildEdges() { /* 300+ lines, lots of scanning */ }
```

**After:**
```cpp
// Simple, composable
BasicBlock* buildIf(stmt, incoming, contexts) { /* 50 lines */ }
BasicBlock* buildWhile(stmt, incoming, contexts) { /* 40 lines */ }
BasicBlock* buildFor(stmt, incoming, contexts) { /* 60 lines */ }
// Each structure is self-contained and testable
```

### Performance

**Compilation Time:**
- Old: O(n²) due to forward scanning
- New: O(n) single pass
- Expected: 10-20% faster for large programs

**Memory:**
- Old: Global stacks + block array
- New: Call stack (recursion) + block array
- Expected: Similar or slightly more (context structs)

---

## Risk Assessment

### High Risks

**Risk:** Bugs in new implementation  
**Mitigation:** 
- Parallel implementation with feature flag
- Extensive testing before switch
- Keep old builder as fallback

**Risk:** Performance regression  
**Mitigation:**
- Benchmark before/after
- Profile hot paths
- Optimize recursive calls if needed

### Medium Risks

**Risk:** Breaking existing features  
**Mitigation:**
- Run full test suite continuously
- Test all control structures
- Validate CFG structure matches

**Risk:** Increased complexity from recursion  
**Mitigation:**
- Clear documentation
- Simple, consistent patterns
- Helper functions for common operations

### Low Risks

**Risk:** Stack overflow from deep nesting  
**Mitigation:**
- BASIC programs rarely nest > 10 levels
- Modern systems have large stacks
- Could add depth limit if needed

---

## Success Metrics

### Must Have (Week 4)

- ✅ All 6 nested control flow tests pass
- ✅ Zero regressions in existing tests
- ✅ Code compiles without warnings
- ✅ Documentation updated

### Should Have (Week 5)

- ✅ Compile time < old builder + 10%
- ✅ Memory < old builder + 20%
- ✅ Code coverage > 90%
- ✅ All CFG traces show correct structure

### Nice to Have (Week 6)

- ✅ Performance improvements in some cases
- ✅ Cleaner, more maintainable code
- ✅ Easier to add new control structures
- ✅ Better error messages

---

## Lessons Learned from Current Bug

### What Went Wrong

1. **Band-aid Fix:** January 2025 fixed WHILE/FOR but not REPEAT/DO
2. **Architecture:** Two-phase approach is fundamentally broken
3. **Testing:** Didn't have comprehensive nesting tests until now

### What We Learned

1. **Tests First:** Comprehensive test suite found all the bugs
2. **Architecture Matters:** Can't fix bad architecture with patches
3. **Recursion:** Natural fit for nested structures

### What to Do Differently

1. **Single-Pass:** Design for recursion from start
2. **Explicit Wiring:** No implicit assumptions
3. **Context Passing:** No global state
4. **Test Coverage:** Test all nesting combinations

---

## Key Design Principles for New Builder

### 1. Explicit Over Implicit

❌ **Don't:** Assume block N+1 follows block N  
✅ **Do:** Explicitly wire every edge

### 2. Local Over Global

❌ **Don't:** Use global stacks for context  
✅ **Do:** Pass context as function parameters

### 3. Single-Pass Over Two-Phase

❌ **Don't:** Build blocks, then scan to add edges  
✅ **Do:** Create structure and edges together

### 4. Recursive Over Iterative

❌ **Don't:** Try to flatten nested structures  
✅ **Do:** Embrace recursion for nested structures

### 5. Composition Over Monolithic

❌ **Don't:** One giant function for all control flow  
✅ **Do:** Small functions that compose

### 6. Black Box Over Leaky Abstraction

❌ **Don't:** Caller needs to know internal structure  
✅ **Do:** Each builder returns only entry/exit blocks

---

## Conclusion

The current CFG builder's two-phase architecture with implicit fallthrough assumptions is fundamentally incompatible with nested control structures. The January 2025 fix was a band-aid that worked for WHILE/FOR but failed for REPEAT/DO because those loops were still using the broken two-phase approach.

The solution is a complete architectural refactoring to single-pass recursive construction where:
1. Each control structure is fully wired at creation time
2. Context is passed explicitly via function parameters
3. No global state or forward scanning
4. Natural handling of arbitrary nesting depth

**Implementation Timeline:** 4-6 weeks  
**Risk Level:** Medium (mitigated by parallel implementation)  
**Expected Outcome:** 100% nested test pass rate, cleaner code, better maintainability

The comprehensive test suite we created will validate the refactoring and prevent future regressions.

---

**Plan Created:** February 1, 2025  
**Status:** Ready for Implementation  
**Priority:** HIGH  
**Estimated Effort:** 4-6 weeks  
**Expected Benefit:** Eliminates entire class of CFG bugs permanently