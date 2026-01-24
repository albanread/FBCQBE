# FOR Loop CFG Analysis - Critical Bug

## Problem Statement

FOR loops in the CFG are **not generating proper control flow edges**. This causes:
- Loop body executes only once (no back-edge)
- NEXT statement doesn't loop back to FOR
- EXIT FOR works but exposes that the loop structure is broken

## Current CFG Structure (BROKEN)

When the CFG builder processes:
```basic
30 FOR I = 1 TO 100
40   PRINT I
50 NEXT I
60 PRINT "Done"
```

It creates these blocks:
```
Block 0: [Line 30]
  - FOR statement (initialization)
  - Creates: loopHeader, loopBody, loopExit blocks
  - Sets m_currentBlock = loopBody

Block 1: FOR Loop Header (EMPTY)

Block 2: FOR Loop Body [Lines 40, 50, 60]
  - PRINT I
  - NEXT I  
  - PRINT "Done"  ← WRONG! This should be in Block 3

Block 3: After FOR (EMPTY)

Block 4: Exit
```

**Problems:**
1. **No edges from Block 0 → Block 1** (no fallthrough to header)
2. **No edges from Block 1 → Block 2** (condition true → body)
3. **No edges from Block 1 → Block 3** (condition false → exit)
4. **No edges from Block 2 → Block 1** (NEXT back-edge to header)
5. **All statements after NEXT are in the loop body block** (block splitting broken)

## What buildEdges() Does (NOT ENOUGH)

File: `fasterbasic_cfg.cpp` line 660

```cpp
void CFGBuilder::buildEdges() {
    // ...
    case ASTNodeType::STMT_NEXT:
        // Loop back edges handled in buildEdges phase
        break;  // ← DOES NOTHING!
```

**The NEXT case is EMPTY!** No edges are created.

## What Should Happen

### Correct CFG Structure

```
Block 0: Entry [Line 30]
  - FOR I = 1 TO 100 (initialization)
  - Edge: FALLTHROUGH → Block 1

Block 1: FOR Loop Header [condition check]
  - Compare I <= 100
  - Edge: CONDITIONAL (true) → Block 2
  - Edge: CONDITIONAL (false) → Block 3

Block 2: FOR Loop Body [Lines 40, 50]
  - PRINT I
  - NEXT I (increment)
  - Edge: UNCONDITIONAL → Block 1 (BACK-EDGE!)

Block 3: After FOR [Line 60]
  - PRINT "Done"
  - Edge: FALLTHROUGH → Block 4

Block 4: Exit
```

### Key Requirements

1. **FOR statement** (line 30):
   - Initialize loop variable
   - Store END value
   - Store STEP value
   - Create header, body, exit blocks
   - **Add fallthrough edge to header**

2. **Loop Header** (Block 1):
   - Evaluate condition: `I <= END`
   - **Add conditional edge (true) → body**
   - **Add conditional edge (false) → exit**

3. **NEXT statement** (line 50):
   - Increment loop variable: `I = I + STEP`
   - **Add unconditional edge (back-edge) → header**
   - **Create new block for code after NEXT**
   - **Set m_currentBlock = afterBlock**

4. **Block Splitting**:
   - When NEXT is encountered, statements AFTER it should go in a new block
   - Currently ALL statements from FOR to NEXT (and beyond) go in one block

## Root Causes

### 1. processForStatement() Issues

File: `fasterbasic_cfg.cpp` line 350

```cpp
void CFGBuilder::processForStatement(const ForStatement& stmt, BasicBlock* currentBlock) {
    BasicBlock* loopHeader = createNewBlock("FOR Loop Header");
    loopHeader->isLoopHeader = true;
    
    BasicBlock* loopBody = createNewBlock("FOR Loop Body");
    BasicBlock* loopExit = createNewBlock("After FOR");
    loopExit->isLoopExit = true;
    
    LoopContext ctx;
    ctx.headerBlock = loopHeader->id;
    ctx.exitBlock = loopExit->id;
    ctx.variable = stmt.variable;
    m_loopStack.push_back(ctx);
    
    m_currentCFG->forLoopHeaders[loopHeader->id] = loopHeader->id;
    
    m_currentBlock = loopBody;  // ← Sets current block to body
    // ✗ No edges created!
    // ✗ Doesn't finalize currentBlock (no edge from it to header)
}
```

**Missing:**
- Edge from `currentBlock` → `loopHeader` (fallthrough to start loop)
- Edge from `loopHeader` → `loopBody` (condition true)
- Edge from `loopHeader` → `loopExit` (condition false)

### 2. processStatement() - NEXT Case

File: `fasterbasic_cfg.cpp` line 276

```cpp
case ASTNodeType::STMT_NEXT:
    // Loop back edges handled in buildEdges phase
    break;  // ← EMPTY! Does nothing!
```

**Missing:**
- Should create back-edge from current block → loop header
- Should create new block for code after NEXT
- Should pop loop context

### 3. buildEdges() - No FOR Handling

File: `fasterbasic_cfg.cpp` line 660

The `buildEdges()` function walks all blocks and creates edges based on the last statement.
**It has no case for STMT_FOR or STMT_NEXT!**

This means:
- FOR blocks get no outgoing edges
- NEXT blocks get no back-edge

### 4. Block Splitting Problem

The CFG builder doesn't create a new block after NEXT, so all statements after NEXT
end up in the same block as the loop body. This is why line 60 ("Done") is in Block 2
instead of Block 3.

## Solution Design

### Option 1: Handle in processStatement() (RECOMMENDED)

**In processForStatement():**
```cpp
void CFGBuilder::processForStatement(const ForStatement& stmt, BasicBlock* currentBlock) {
    // 1. Finalize current block
    finalizeBlock(currentBlock);
    
    // 2. Create loop structure
    BasicBlock* loopHeader = createNewBlock("FOR Loop Header");
    loopHeader->isLoopHeader = true;
    
    BasicBlock* loopBody = createNewBlock("FOR Loop Body");
    
    BasicBlock* loopExit = createNewBlock("After FOR");
    loopExit->isLoopExit = true;
    
    // 3. Add edge: currentBlock → loopHeader
    addFallthroughEdge(currentBlock->id, loopHeader->id);
    
    // 4. Add conditional edges from header
    addConditionalEdge(loopHeader->id, loopBody->id, "I <= END");
    addConditionalEdge(loopHeader->id, loopExit->id, "I > END");
    
    // 5. Track loop context
    LoopContext ctx;
    ctx.headerBlock = loopHeader->id;
    ctx.exitBlock = loopExit->id;
    ctx.variable = stmt.variable;
    m_loopStack.push_back(ctx);
    
    // 6. Continue building in loop body
    m_currentBlock = loopBody;
}
```

**Add processNextStatement():**
```cpp
void CFGBuilder::processNextStatement(const NextStatement& stmt, BasicBlock* currentBlock) {
    // 1. Find matching FOR loop context
    if (m_loopStack.empty()) {
        // Error: NEXT without FOR
        return;
    }
    
    LoopContext& loopCtx = m_loopStack.back();
    
    // 2. Add back-edge: currentBlock → loopHeader
    addUnconditionalEdge(currentBlock->id, loopCtx.headerBlock);
    
    // 3. Create new block for code after loop
    BasicBlock* afterBlock = m_currentCFG->getBlock(loopCtx.exitBlock);
    if (!afterBlock) {
        afterBlock = createNewBlock("After FOR");
    }
    
    // 4. Pop loop context
    m_loopStack.pop_back();
    
    // 5. Continue in after block
    m_currentBlock = afterBlock;
}
```

**Update processStatement() switch:**
```cpp
case ASTNodeType::STMT_NEXT:
    processNextStatement(static_cast<const NextStatement&>(stmt), currentBlock);
    break;
```

### Option 2: Handle in buildEdges()

Walk through blocks in `buildEdges()` and:
1. If block contains FOR statement, create header edges
2. If block contains NEXT statement, create back-edge

**Problem:** Harder to implement because blocks are already created and we'd need
to find the matching FOR/NEXT blocks.

## Recommended Fix Priority

1. **HIGH**: Implement `processNextStatement()` to create back-edge and split blocks
2. **HIGH**: Update `processForStatement()` to create initial edges
3. **MEDIUM**: Add validation to ensure FOR/NEXT matching
4. **MEDIUM**: Handle nested FOR loops correctly
5. **LOW**: Optimize empty loop header blocks

## Testing Strategy

1. **Simple loop:**
   ```basic
   FOR I = 1 TO 5
     PRINT I
   NEXT I
   PRINT "Done"
   ```
   Should print: 1, 2, 3, 4, 5, Done

2. **EXIT FOR:**
   ```basic
   FOR I = 1 TO 10
     PRINT I
     IF I = 5 THEN EXIT FOR
   NEXT I
   PRINT "Done"
   ```
   Should print: 1, 2, 3, 4, 5, Done

3. **Nested loops:**
   ```basic
   FOR I = 1 TO 3
     FOR J = 1 TO 2
       PRINT I; ","; J
     NEXT J
   NEXT I
   ```

4. **Step values:**
   ```basic
   FOR I = 10 TO 1 STEP -2
     PRINT I
   NEXT I
   ```

## Related Issues

- WHILE/WEND loops - **WORKING** (have proper edges)
- REPEAT/UNTIL loops - **WORKING** (have proper edges)
- DO/LOOP loops - **WORKING** (have proper edges)
- FOR loops - **BROKEN** (no edges!)

## Conclusion

FOR loops are fundamentally broken in the CFG builder. The structure exists but
edges are never created. This is a **critical bug** that must be fixed before
FOR loops can work correctly.

The fix is straightforward but requires careful implementation to handle:
- Edge creation in processForStatement()
- Back-edge creation in processNextStatement() (NEW function needed)
- Block splitting after NEXT
- Loop context management