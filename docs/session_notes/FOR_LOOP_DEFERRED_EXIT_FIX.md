# FOR Loop Deferred Exit Strategy Fix

## Problem Summary

The FasterBASIC compiler had a "time travel" problem in its CFG (Control Flow Graph) construction for FOR loops. The loop exit block was being created **before** the loop body was processed, which caused it to receive a block ID that was logically out of sequence with nested control structures.

### Symptom

When a FOR loop contained nested control structures (like multi-line IF statements), the block IDs would be:
- Block 10: FOR Loop Check
- Block 11: FOR Loop Body
- Block 12: After FOR (exit block) ← Created early
- Block 13: IF THEN block ← Created during body processing
- Block 14: IF ELSE block
- Block 15: After IF

The CFG builder's `buildEdges` logic would see block 12 followed by block 13 and incorrectly assume a fallthrough edge existed, causing infinite loops or incorrect control flow.

### Root Cause

The exit block was created in `processForStatement` (when the FOR statement was encountered) rather than in `processNextStatement` (when the NEXT statement closed the loop). This violated the principle that **block IDs should reflect the chronological order of code execution**.

## Solution: The "Deferred Exit" Strategy

The fix implements a deferred exit strategy where the loop exit block is only created when the NEXT statement is processed, ensuring all nested blocks receive IDs before the exit block.

### Key Changes

#### 1. Updated `LoopContext` Structure

Added a `pendingExitBlocks` vector to track EXIT FOR statements that need to be wired to the exit block once it's created:

```cpp
struct LoopContext {
    int headerBlock;
    int exitBlock;
    std::string variable;
    std::vector<int> pendingExitBlocks;  // NEW: blocks containing EXIT FOR
};
```

#### 2. Modified `processForStatement`

The FOR statement handler now:
- Creates init, check, and body blocks
- Sets `exitBlock = -1` (placeholder)
- Does **not** create the exit block

```cpp
void CFGBuilder::processForStatement(const ForStatement& stmt, BasicBlock* currentBlock) {
    BasicBlock* initBlock = createNewBlock("FOR Init");
    BasicBlock* loopCheck = createNewBlock("FOR Loop Check");
    loopCheck->isLoopHeader = true;
    BasicBlock* loopBody = createNewBlock("FOR Loop Body");
    
    LoopContext ctx;
    ctx.headerBlock = loopCheck->id;
    ctx.exitBlock = -1;  // Placeholder - will be set by NEXT
    ctx.variable = stmt.variable;
    m_loopStack.push_back(ctx);
    
    m_currentBlock = loopBody;
}
```

#### 3. Enhanced EXIT FOR Handling

EXIT FOR statements now add their block ID to `pendingExitBlocks`:

```cpp
case ASTNodeType::STMT_EXIT:
    const ExitStatement& exitStmt = static_cast<const ExitStatement&>(stmt);
    
    if (exitStmt.exitType == ExitStatement::ExitType::FOR_LOOP) {
        if (!m_loopStack.empty()) {
            // Find innermost FOR loop and add to pending exits
            for (auto it = m_loopStack.rbegin(); it != m_loopStack.rend(); ++it) {
                if (!it->variable.empty()) {  // FOR loops have a variable
                    it->pendingExitBlocks.push_back(currentBlock->id);
                    break;
                }
            }
        }
        BasicBlock* afterExit = createNewBlock("After EXIT FOR");
        m_currentBlock = afterExit;
    }
    break;
```

#### 4. Rewrote `processNextStatement` (The Closer)

This is where the magic happens. The NEXT handler now:

1. **Creates the NEXT/Increment block** (for loop increment code)
2. **Moves the NEXT statement** from loop body to the incrementor block
3. **Wires loop body to incrementor block**
4. **Records back-edge** from incrementor to loop header
5. **Creates the exit block** (finally!)
6. **Wires all pending EXIT FOR blocks** to the new exit block
7. **Updates loop structures** with the real exit block ID
8. **Switches to exit block** for subsequent code

```cpp
void CFGBuilder::processNextStatement(const NextStatement& stmt) {
    // Find matching loop context
    LoopContext* matchingLoop = ...;
    
    // 1. Create the NEXT block (incrementor)
    BasicBlock* nextBlock = createNewBlock("FOR Next/Increment");
    
    // 2. Move NEXT statement from current block to NEXT block
    if (!currentBlock->statements.empty() && currentBlock->statements.back() == &stmt) {
        int lineNum = currentBlock->statementLineNumbers[&stmt];
        currentBlock->statements.pop_back();
        currentBlock->statementLineNumbers.erase(&stmt);
        nextBlock->addStatement(&stmt, lineNum);
    }
    
    // 3. Current block flows into NEXT block
    if (!currentBlock->isTerminator && currentBlock->successors.empty()) {
        addFallthroughEdge(currentBlock->id, nextBlock->id);
    }
    
    // 4. Record back-edge from NEXT to loop header
    m_nextToHeaderMap[nextBlock->id] = matchingLoop->headerBlock;
    
    // 5. NOW create the exit block
    BasicBlock* loopExit = createNewBlock("After FOR");
    loopExit->isLoopExit = true;
    matchingLoop->exitBlock = loopExit->id;
    
    // 6. Wire all pending EXIT FOR blocks
    for (int exitBlockId : matchingLoop->pendingExitBlocks) {
        addFallthroughEdge(exitBlockId, loopExit->id);
    }
    
    // 7. Update FOR loop structure
    for (auto& pair : m_currentCFG->forLoopStructure) {
        if (pair.second.checkBlock == matchingLoop->headerBlock) {
            pair.second.exitBlock = loopExit->id;
            break;
        }
    }
    
    // 8. Switch to exit block
    m_currentBlock = loopExit;
    
    // 9. Pop loop context
    m_loopStack.erase(...);
}
```

#### 5. Added Safety Check in `buildEdges`

Added a guard to prevent trying to wire the false branch if exit block hasn't been created yet:

```cpp
// FOR check block: conditional branch
addConditionalEdge(block->id, forBlocks.bodyBlock, "true");
if (forBlocks.exitBlock >= 0) {  // Safety check
    addConditionalEdge(block->id, forBlocks.exitBlock, "false");
}
```

## Results

### Before Fix

**CFG Structure:**
```
Block 10: FOR Loop Check
Block 11: FOR Loop Body
Block 12: After FOR (exit)  ← Created too early
Block 13: IF THEN            ← Nested structure
Block 14: After IF
```

**Problem:** Block 12 → Block 13 phantom fallthrough edge

### After Fix

**CFG Structure:**
```
Block 10: FOR Loop Check
Block 11: FOR Loop Body
Block 12: IF THEN            ← Nested structure
Block 13: After IF
Block 14: FOR Next/Increment ← NEW: incrementor block
Block 15: After FOR (exit)   ← Created at NEXT
```

**Benefit:** Block IDs now reflect chronological order, no phantom edges

### QBE IL Comparison

**Before (broken):**
```qbe
@block_3
    # Loop body
    jnz %condition, @then, @else
@then
    jmp @block_5  # EXIT FOR
@else
    # Increment code here (wrong!)
    jmp @block_4
@block_4
    # Empty block, no back-edge!
```

**After (fixed):**
```qbe
@block_3
    # Loop body
    jnz %condition, @then, @else
@then
    jmp @block_5  # EXIT FOR
@else
    jmp @endif

@block_4
    # Increment code here (correct!)
    %t =l add %var_I, %step
    %var_I =l copy %t
    jmp @block_2  # Back-edge to loop header

@block_5
    # After FOR (exit)
```

## Test Results

The fix resolves the infinite loop in `tests/loops/test_exit_statements.bas`:

```
✓ PASS: EXIT FOR at iteration 5
✓ PASS: Normal FOR = 15
✓ PASS: Found K = 8
✓ PASS: EXIT DO WHILE at 7
✓ PASS: EXIT DO UNTIL at 3
✓ PASS: Nested EXIT FOR (inner only)
✓ PASS: Exit at first iteration
✓ PASS: First EXIT triggered at 5
✓ PASS: Safety exit at 6
✓ PASS: EXIT FOR with STEP = 4

=== All EXIT Statements Tests PASSED ===
```

## Key Principles Established

1. **Block IDs Must Reflect Chronological Order**: Don't create blocks that logically belong "after" code until that code has been processed.

2. **Defer Exit Block Creation**: For loop structures, create the exit block when the loop **closes** (NEXT), not when it **opens** (FOR).

3. **Explicit Edge Wiring**: Never rely on `block->id + 1` for control flow. Always explicitly wire edges.

4. **Pending Exits Pattern**: Use a pending list to track edges that can't be wired immediately (because the target doesn't exist yet).

5. **Statement Placement Matters**: Ensure statements end up in the correct blocks (NEXT statement goes in the incrementor block, not the loop body).

## Files Modified

- `FBCQBE/fsh/FasterBASICT/src/fasterbasic_cfg.h` - Added `pendingExitBlocks` to `LoopContext`
- `FBCQBE/fsh/FasterBASICT/src/fasterbasic_cfg.cpp` - Implemented deferred exit strategy

## Related Documentation

- See `MULTILINE_IF_CFG_ISSUE.md` for the related multi-line IF block ordering fix
- See `CFG_FIX_SESSION_COMPLETE.md` for the overall CFG refactoring journey

## Credits

Solution identified and implemented based on analysis of the "time travel" problem in CFG block ordering.
Date: January 30, 2025