# CFG Bugs Found During QBE IL Generation

## Overview

The FasterBASIC CFG builder was originally designed to support Lua code generation, which is a high-level dynamic language. Lua's runtime handles many control flow details implicitly. Now that we're generating low-level SSA-based QBE IL, we're discovering CFG construction bugs that were previously hidden.

**Key Insight:** QBE IL requires **explicit** representation of ALL control flow. Every jump, branch, and edge must be precisely defined. Lua could "paper over" imprecise CFG structure.

---

## Bug #1: REPEAT/UNTIL Missing Back Edges

### Issue
REPEAT/UNTIL loops were not creating proper conditional back edges.

### Symptoms
- Block containing UNTIL had only 1 successor (fallthrough)
- No conditional branch generated
- Loop would execute once and exit

### Root Cause
**Multiple issues:**

1. **Loop Context Premature Pop**
   - `processStatement()` was popping loop context when seeing UNTIL
   - `buildEdges()` runs AFTER all statements are processed
   - By the time buildEdges tried to create edges, loop stack was empty
   
2. **Wrong Block ID Comparison**
   - buildEdges searched for loop context with `block->id > ctx.headerBlock`
   - Failed when UNTIL was in the SAME block as loop body (block 1)
   - Comparison `1 > 1` is FALSE

3. **UNTIL Not Creating New Block**
   - UNTIL statement didn't end current block
   - Subsequent statements were added to loop body block
   - CFG structure didn't match actual control flow

### The Fix

**File:** `fasterbasic_cfg.cpp`

**Change 1: UNTIL creates new block**
```cpp
case ASTNodeType::STMT_UNTIL:
    // UNTIL ends the loop body and starts a new block for code after the loop
    {
        BasicBlock* nextBlock = createNewBlock("After REPEAT");
        m_currentBlock = nextBlock;
        // Don't pop loop context - buildEdges() needs it
    }
    break;
```

**Change 2: buildEdges creates proper edges**
```cpp
case ASTNodeType::STMT_UNTIL: {
    // Find matching REPEAT loop context
    LoopContext* loopCtx = nullptr;
    for (auto& ctx : m_loopStack) {
        // CHANGED: >= instead of > to handle UNTIL in header block
        if (block->id >= ctx.headerBlock) {
            loopCtx = &ctx;
            break;
        }
    }
    
    if (loopCtx) {
        // TRUE: exit loop (go to next block)
        addConditionalEdge(block->id, block->id + 1, "true");
        // FALSE: repeat (go back to loop header)
        addConditionalEdge(block->id, loopCtx->headerBlock, "false");
        
        // NOW pop the loop context
        // (remove from vector)
    }
    break;
}
```

### Result
✅ REPEAT/UNTIL now generates correct loop structure:
```qbe
@block_1  # Loop body
    # ... statements ...
    %cond =w csge %var_X_INT, %end
    jnz %cond, @block_2, @block_1  # exit or repeat

@block_2  # After loop
```

---

## Expected Future Bugs

Based on the Lua→QBE transition, we expect to find similar issues in:

### 1. FOR/NEXT Loops
**Potential Issues:**
- Loop increment/decrement logic
- Step value handling (positive vs negative)
- Exit condition evaluation (inclusive vs exclusive)
- NEXT without matching FOR

**Why:** Lua FOR loops are built-in syntax. QBE needs explicit:
- Initialize loop variable
- Check condition
- Increment/decrement
- Conditional back edge

### 2. WHILE/WEND Loops
**Potential Issues:**
- Condition evaluation placement
- Back edge to condition check (not loop body)
- WEND not creating proper back edge

**Why:** Lua WHILE is built-in. QBE needs:
- Separate condition check block
- Branch to body or exit
- Unconditional jump back to condition

### 3. DO/LOOP Variants
**Potential Issues:**
- DO WHILE vs DO UNTIL (condition sense)
- LOOP WHILE vs LOOP UNTIL (post-test loops)
- Condition at top vs bottom

**Why:** Multiple variants with subtle differences in when condition is tested.

### 4. Nested Loop Handling
**Potential Issues:**
- Loop stack not properly maintained
- NEXT/WEND/UNTIL matching wrong loop
- EXIT statements jumping to wrong block

**Why:** Nested loops need proper context tracking. Lua could handle implicitly.

### 5. IF/THEN/ELSE with GOTO
**Potential Issues:**
- Mixing structured IF with unstructured GOTO
- Block splitting at wrong points
- Dangling blocks with no predecessors

**Why:** BASIC allows arbitrary mixing. CFG must handle all cases.

### 6. ON GOTO / ON GOSUB
**Potential Issues:**
- Multi-way branch not creating all edges
- Computed GOTO destinations
- Out-of-range selector handling

**Why:** Needs N-way branch in CFG. Lua could use table dispatch.

### 7. GOSUB/RETURN
**Potential Issues:**
- Return stack management
- RETURN without matching GOSUB
- Multiple GOSUB targets with shared RETURN

**Why:** Lua doesn't have GOSUB. This is essentially manual call stack in BASIC.

### 8. SELECT CASE
**Potential Issues:**
- Multiple condition checks
- Fall-through behavior
- Default case handling

**Why:** Needs complex multi-way branching. May generate as chain of IFs.

---

## Debugging Strategy

### Symptoms of CFG Bugs

1. **Missing Branches**
   - Block has successors in CFG but no branch instruction emitted
   - Check: `block->successors.size()` vs actual jumps in generated code

2. **Wrong Branch Targets**
   - Branch goes to wrong block
   - Check: successor[0] and successor[1] vs generated labels

3. **Dead Code**
   - Blocks with no predecessors (unreachable)
   - Check: `block->predecessors.empty()` for non-entry blocks

4. **Infinite Loops**
   - Back edge without exit condition
   - Check: loop blocks have conditional exit edge

5. **Missing Blocks**
   - Statements that should split blocks don't
   - Check: terminators (GOTO, RETURN, END) create new blocks

### Debugging Process

1. **Enable Verbose Output**
   ```bash
   ./fbc_qbe --verbose test.bas
   ```
   Look at "CFG blocks: N"

2. **Add Debug Comments**
   Temporarily add to qbe_codegen_main.cpp:
   ```cpp
   emitComment("Block " + std::to_string(block->id) + 
               " successors: " + std::to_string(block->successors.size()));
   ```

3. **Check CFG Structure**
   Print CFG in build phase (if verbose):
   ```cpp
   std::cout << m_cfg->toString() << std::endl;
   ```

4. **Verify Edge Creation**
   Add debug in buildEdges():
   ```cpp
   std::cerr << "Adding edge: " << block->id << " -> " << target << std::endl;
   ```

5. **Compare BASIC → Blocks**
   - Which line numbers in which blocks?
   - Does block splitting make sense?
   - Are loop boundaries correct?

---

## Lessons Learned

### 1. High-Level → Low-Level Translation is Hard
Lua codegen could rely on Lua's runtime. QBE codegen must make everything explicit.

### 2. CFG Must Be Precise
In low-level IR, CFG **IS** the program structure. Imprecise CFG = wrong code.

### 3. Test Control Flow Extensively
Every loop type, every conditional variant, every combination must be tested.

### 4. Phase Separation Matters
- buildBlocks() creates blocks and adds statements
- buildEdges() creates edges based on statements
- Order matters! Loop contexts must survive between phases

### 5. Block Terminators Are Critical
Statements that change control flow MUST:
- End current block
- Create successor relationships
- Let buildEdges create edges

---

## Testing Checklist

Use these test cases to find more CFG bugs:

### Loops
- [ ] Simple FOR loop
- [ ] FOR with negative step
- [ ] FOR with variable step
- [ ] Nested FOR loops
- [ ] WHILE loop
- [ ] DO WHILE loop (pre-test)
- [ ] DO UNTIL loop
- [ ] LOOP WHILE (post-test)
- [ ] REPEAT/UNTIL ✅ (FIXED)
- [ ] EXIT FOR
- [ ] EXIT WHILE
- [ ] EXIT DO

### Conditionals
- [ ] Simple IF/THEN/ELSE/END IF ✅ (WORKING)
- [ ] IF with GOTO ✅ (WORKING)
- [ ] Nested IF statements
- [ ] IF without ELSE
- [ ] ELSE IF chains
- [ ] SELECT CASE

### Jumps
- [ ] GOTO forward
- [ ] GOTO backward (loop)
- [ ] GOTO into IF block
- [ ] GOTO out of loop
- [ ] ON GOTO with multiple targets
- [ ] GOSUB/RETURN
- [ ] Nested GOSUB

### Edge Cases
- [ ] Empty loop body
- [ ] Multiple GOTOs in sequence
- [ ] GOTO to non-existent line
- [ ] RETURN without GOSUB
- [ ] NEXT without FOR
- [ ] Mixed structured/unstructured control flow

---

## References

- QBE IL Specification: `qbe/doc/il.txt`
- CFG Builder Source: `FasterBASICT/src/fasterbasic_cfg.cpp`
- Code Generator: `FasterBASICT/src/codegen/qbe_codegen_main.cpp`
- Test Suite: `test_*.bas` files

---

## Contributing

When you find a CFG bug:

1. Create a minimal test case (`test_bug_name.bas`)
2. Document the expected vs actual CFG structure
3. Identify the root cause in `fasterbasic_cfg.cpp`
4. Fix the bug
5. Verify fix with generated QBE IL
6. Add test case to suite
7. Document in this file

Remember: **Every CFG bug found and fixed makes the compiler more robust!**