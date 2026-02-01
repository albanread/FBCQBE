# CFG Builder V2 - Single-Pass Recursive Architecture

**Status:** In Development (February 2025)  
**Purpose:** Replace fragile two-phase CFG builder with robust single-pass recursive approach  
**Priority:** HIGH - Fixes critical nested control flow bugs

---

## Overview

This directory contains the new CFG (Control Flow Graph) builder implementation using a single-pass recursive architecture. This is a complete rewrite to fix fundamental architectural flaws in the original builder.

---

## Why V2 Was Needed

### The Problem

The original CFG builder (`../fasterbasic_cfg.cpp`) had a fatal architectural flaw:

**Two-Phase Construction:**
1. Phase 1: Build all blocks linearly
2. Phase 2: Scan forward to find loop ends and add edges

**Implicit Assumptions:**
- Execution flows from block N to block N+1
- Physical block order = logical execution order

**Result:**
- ✅ Simple programs work
- ❌ Nested control structures fail catastrophically
- ❌ REPEAT/DO loops in IF ELSE branches cause infinite loops
- ❌ Test pass rate: 50% (3 of 6 nested tests)

### Test Results That Proved The Problem

```
✅ PASS: test_nested_while_if.bas    (WHILE fixed Jan 2025)
✅ PASS: test_nested_if_while.bas    (IF in loops works)
✅ PASS: test_nested_for_if.bas      (FOR fixed Jan 2025)
❌ FAIL: test_nested_repeat_if.bas   (INFINITE LOOP)
❌ FAIL: test_nested_do_if.bas       (INFINITE LOOP)
❌ FAIL: test_nested_mixed_controls.bas (INFINITE LOOP)
```

**Root Cause:** January 2025 fix patched WHILE/FOR but didn't address the fundamental architectural issue. REPEAT/DO still used the broken two-phase approach.

---

## The V2 Solution

### Single-Pass Recursive Construction

**Key Principle:** Build complete structures immediately, not in phases.

```cpp
// OLD (Broken):
void buildBlocks() {
    // Add statements to blocks linearly
}
void buildEdges() {
    // Scan to find loop ends (context lost!)
}

// NEW (Fixed):
BasicBlock* buildWhile(incoming) {
    header = create(); body = create(); exit = create();
    wire(incoming → header);
    wire(header → body [true]); wire(header → exit [false]);
    bodyExit = buildStatements(body);  // Recursively!
    wire(bodyExit → header);  // Back-edge created NOW!
    return exit;  // Next statement connects here
}
```

### Architecture Changes

| Aspect | Old V1 | New V2 |
|--------|--------|--------|
| **Phases** | Two (blocks, then edges) | One (together) |
| **Nesting** | Global stacks | Context parameters |
| **Fallthrough** | Implicit (N→N+1) | Explicit wiring |
| **Scanning** | O(n²) forward search | O(1) recursive |
| **Back-edges** | Added later (error-prone) | Added immediately |
| **Context** | Lost between phases | Preserved in call stack |

---

## File Structure

```
cfg_v2/
├── README.md                    ← You are here
├── cfg_builder_v2.h             ← Main interface & context structures
├── cfg_builder_v2.cpp           ← Core recursive builder + IF
├── cfg_while.cpp                ← WHILE loop implementation (TODO)
├── cfg_for.cpp                  ← FOR loop implementation (TODO)
├── cfg_repeat.cpp               ← REPEAT loop implementation (TODO)
├── cfg_do.cpp                   ← DO loop implementation (TODO)
├── cfg_select.cpp               ← SELECT CASE implementation (TODO)
├── cfg_try.cpp                  ← TRY/CATCH implementation (TODO)
└── cfg_terminators.cpp          ← GOTO/RETURN/EXIT implementation (TODO)
```

---

## Implementation Status

### ✅ Complete

- [x] Core architecture design
- [x] Context structures (LoopContext, SelectContext, etc.)
- [x] Main entry point (`build()`)
- [x] Recursive statement builder (`buildStatementRange()`)
- [x] IF statement implementation (`buildIf()`)
- [x] Block/edge management functions
- [x] Terminator stub implementations

### 🚧 In Progress

- [ ] WHILE loop implementation
- [ ] FOR loop implementation
- [ ] REPEAT loop implementation (CRITICAL - fixes infinite loop bug)
- [ ] DO loop implementation (CRITICAL - fixes infinite loop bug)

### 📋 TODO

- [ ] SELECT CASE implementation
- [ ] TRY/CATCH implementation
- [ ] GOSUB/RETURN implementation
- [ ] EXIT statements (FOR/WHILE/DO/SELECT)
- [ ] CONTINUE statement
- [ ] ON GOTO/GOSUB implementation
- [ ] Label resolution
- [ ] Integration with main compiler
- [ ] Feature flag for testing
- [ ] Comprehensive testing

---

## Key Design Principles

### 1. Explicit Over Implicit

❌ **Don't:** Assume block N flows to block N+1  
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

### 5. Black Box Abstraction

Each `build*()` function is a black box:
- **Input:** Incoming block + context
- **Output:** Exit block
- **Guarantee:** All internal structure fully wired

---

## Example: WHILE Loop Implementation

This is the template all loop builders should follow:

```cpp
BasicBlock* CFGBuilderV2::buildWhile(
    const WhileStatement& stmt,
    BasicBlock* incoming,
    LoopContext* outerLoop,
    SelectContext* select,
    TryContext* tryCtx,
    SubroutineContext* sub
) {
    // 1. Create blocks
    BasicBlock* headerBlock = createBlock("While_Header");
    BasicBlock* bodyEntry = createBlock("While_Body");
    BasicBlock* exitBlock = createBlock("While_Exit");
    
    // 2. Wire entry
    if (!isTerminated(incoming)) {
        addUnconditionalEdge(incoming->id, headerBlock->id);
    }
    
    // 3. Add condition to header
    addStatementToBlock(headerBlock, &stmt, getLineNumber(&stmt));
    headerBlock->isLoopHeader = true;
    
    // 4. Wire header branches
    addConditionalEdge(headerBlock->id, bodyEntry->id, "true");
    addConditionalEdge(headerBlock->id, exitBlock->id, "false");
    
    // 5. Create loop context
    LoopContext thisLoop;
    thisLoop.headerBlockId = headerBlock->id;
    thisLoop.exitBlockId = exitBlock->id;
    thisLoop.loopType = "WHILE";
    thisLoop.outerLoop = outerLoop;
    
    // 6. Recursively build body (KEY!)
    // This handles nested loops/IFs/etc. automatically!
    BasicBlock* bodyExit = buildStatementRange(
        stmt.bodyStatements,
        bodyEntry,
        &thisLoop,  // Pass our context
        select,
        tryCtx,
        sub
    );
    
    // 7. Wire back-edge (THE FIX!)
    if (!isTerminated(bodyExit)) {
        addUnconditionalEdge(bodyExit->id, headerBlock->id);
    }
    
    // 8. Return exit block
    return exitBlock;
}
```

**Why This Works:**
- ✅ No scanning forward for WEND
- ✅ Back-edge created immediately
- ✅ Nested structures handled by recursion
- ✅ Context passed explicitly
- ✅ Exit block returned to caller

---

## Testing Strategy

### Phase 1: Unit Testing (Week 3)

Test each control structure individually:

```bash
# Enable V2 builder
export USE_CFG_V2=1

# Test IF (already implemented)
./qbe_basic --use-cfg-v2 -o /tmp/test tests/conditionals/test_if.bas
/tmp/test

# Test WHILE (after implementation)
./qbe_basic --use-cfg-v2 -o /tmp/test tests/loops/test_while_basic.bas
/tmp/test
```

### Phase 2: Nested Testing (Week 3-4)

Run the comprehensive nested test suite:

```bash
USE_CFG_V2=1 ./scripts/test_nested_control_flow.sh

# Expected outcome after full implementation:
# ✅ 6/6 tests PASS (was 3/6 with old builder)
```

### Phase 3: Full Regression (Week 4)

```bash
USE_CFG_V2=1 ./run_tests.sh

# Should have zero regressions
```

---

## Migration Plan

### Week 1 (Current)
- ✅ Design architecture
- ✅ Implement core + IF
- 🚧 Implement WHILE, FOR

### Week 2
- Implement REPEAT (fixes infinite loop bug!)
- Implement DO (fixes infinite loop bug!)
- Implement SELECT, TRY

### Week 3
- Unit testing
- Nested testing
- Bug fixes

### Week 4
- Full regression testing
- Enable by default with feature flag
- Monitor for issues

### Week 5-6
- Deprecate old builder
- Remove feature flag
- Documentation

---

## Context Structures

### LoopContext

Tracks current loop for EXIT/CONTINUE statements:

```cpp
struct LoopContext {
    int headerBlockId;      // For CONTINUE
    int exitBlockId;        // For EXIT FOR/WHILE/DO
    std::string loopType;   // "FOR", "WHILE", "DO", "REPEAT"
    LoopContext* outerLoop; // Link to enclosing loop
};
```

**Usage:**
```cpp
LoopContext thisLoop;
thisLoop.headerBlockId = header->id;
thisLoop.exitBlockId = exit->id;
thisLoop.loopType = "WHILE";
thisLoop.outerLoop = outerLoop;  // Link to outer

bodyExit = buildStatementRange(body, bodyEntry, &thisLoop, ...);
```

### SelectContext

Tracks SELECT CASE for EXIT SELECT:

```cpp
struct SelectContext {
    int exitBlockId;           // For EXIT SELECT
    SelectContext* outerSelect; // Link to enclosing SELECT
};
```

### TryContext

Tracks TRY/CATCH for THROW:

```cpp
struct TryContext {
    int catchBlockId;      // For THROW
    int finallyBlockId;    // For FINALLY
    TryContext* outerTry;  // Link to enclosing TRY
};
```

---

## Common Patterns

### Pattern 1: Pre-Test Loop (WHILE, FOR, DO WHILE)

```
Entry → Header (condition) → Body → Header (back-edge)
           ↓                           ↑
         Exit ←─────────────────────────┘
```

### Pattern 2: Post-Test Loop (REPEAT, DO...LOOP UNTIL)

```
Entry → Body → Condition → Exit
         ↑          ↓
         └──────────┘ (back-edge)
```

### Pattern 3: Conditional (IF)

```
Entry → Condition → Then → Merge
           ↓                  ↑
         Else ───────────────→┘
```

---

## Debugging

### Enable Debug Mode

```cpp
CFGBuilderV2 builder;
builder.setDebugMode(true);  // TODO: Add this method
cfg = builder.build(statements);
```

**Output:**
```
[CFGv2] Starting CFG construction...
[CFGv2] Created block 0 (Entry)
[CFGv2] Building IF statement
[CFGv2] Created block 1 (If_Then)
[CFGv2] Created block 2 (If_Merge)
[CFGv2] Added edge: Block 0 -> Block 1 [true]
...
```

### Dump CFG Structure

```cpp
builder.dumpCFG("After IF");
```

**Output:**
```
=== CFG Dump (After IF) ===
Total Blocks: 5
Total Edges: 6

Block 0 (Entry)
  Statements: 1
  Successors: 1 2

Block 1 (If_Then)
  Statements: 2
  Successors: 3
...
```

---

## Contributing

### Adding a New Control Structure

1. **Add to header:** Declare `buildXXX()` in `cfg_builder_v2.h`
2. **Create file:** `cfg_xxx.cpp` with implementation
3. **Follow pattern:** Use WHILE implementation as template
4. **Test:** Create unit tests before integration
5. **Document:** Add to this README

### Code Style

- Use existing naming conventions
- Add debug logging for major operations
- Document complex logic
- Keep functions focused (single responsibility)

---

## Expected Outcomes

### Before V2
- 50% nested test pass rate (3/6)
- Infinite loops with REPEAT/DO in IF ELSE
- O(n²) compilation time (scanning)

### After V2
- 100% nested test pass rate (6/6)
- No infinite loop bugs
- O(n) compilation time (single pass)
- Cleaner, maintainable code

---

## References

- **Planning Document:** `../../docs/CFG_REFACTORING_PLAN.md`
- **Test Results:** `../../docs/session_notes/NESTED_TEST_RESULTS_2025_02_01.md`
- **Test Suite:** `../../tests/loops/test_nested_*.bas`
- **Old Implementation:** `../fasterbasic_cfg.{h,cpp}`

---

**Status:** Foundation complete, loop implementations in progress  
**Next Steps:** Implement WHILE, FOR, REPEAT, DO loops  
**Goal:** 100% nested test pass rate by Week 4