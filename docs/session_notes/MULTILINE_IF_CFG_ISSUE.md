# Multi-line IF Statement CFG Issue

## Problem Summary

We implemented support for multi-line IF...THEN...ELSE...END IF statements in the FasterBASIC compiler's Control Flow Graph (CFG) builder. The implementation creates separate blocks for THEN, ELSE, and "After IF" branches. While simple test cases work correctly, complex nested structures (like IF inside FOR loops) create incorrect CFG successor edges, causing infinite loops at runtime.

## Symptoms

1. **Simple test case works**: A single FOR loop with an IF containing EXIT FOR works correctly
2. **Complex test fails**: Multiple sequential tests with FOR loops containing multi-line IFs cause infinite loops
3. **Block ordering issue**: CFG blocks are numbered non-sequentially when IF blocks are created inside other control structures
4. **Incorrect fallthrough**: "After FOR" blocks incorrectly fall through to IF THEN blocks instead of continuing to subsequent code

## Example That Fails

```basic
REM Test 3: EXIT FOR with condition
LET FOUND% = 0
FOR K% = 1 TO 100
    IF K% * K% > 50 THEN
        FOUND% = K%
        EXIT FOR
    END IF
NEXT K%
PRINT "After loop"
PRINT "Test 4 starts here"
```

**What happens**: Test 3 executes correctly (prints "After loop") but then jumps BACK to execute Test 3 again in an infinite loop instead of continuing to Test 4.

## CFG Dump for Failing Test

Here's the relevant portion of the CFG for the failing test (test_exit_statements.bas, Test 3):

```
Block 9 (FOR Init)
  Statements:
    [20] FOR (line 260)
  Successors: 10

Block 10 (FOR Loop Check) [LOOP HEADER]
  Statements: (none)
  Successors: 11, 12

Block 11 (FOR Loop Body)
  Lines: 270
  Statements:
    [21] IF (line 270) - then:2 else:0
  Successors: 13, 14

Block 12 (After FOR) [LOOP EXIT]
  Lines: 320, 330, 340, 350, 360, 370, 380
  Statements:
    [22] PRINT (line 320)
    [23] IF (line 330) - then:2 else:0
    [24] PRINT (line 340)
    [25] PRINT (line 350)
    [26] LET/ASSIGNMENT (line 370)
  Successors: 13           <--- PROBLEM: Should be 16 (next test), not 13!

Block 13 (IF THEN)
  Statements:
    [27] LET/ASSIGNMENT
    [28] EXIT
  Successors: 48

Block 14 (IF ELSE)
  Statements: (none)
  Successors: 15, 15       <--- Also wrong: duplicate successor

Block 15 (After IF)
  Lines: 310
  Statements:
    [29] NEXT (line 310)
  Successors: 10

Block 16 (DO Loop Header) [LOOP HEADER]  <--- This is where Block 12 should point
  Statements:
    [30] DO (line 380)
  Successors: 17, 18
```

**Key observation**: Block 12 (created as the FOR loop's exit block) points to Block 13 (the IF THEN block created later). Block order is: 9, 10, 11, 12, 13, 14, 15, 16...

Blocks 13, 14, 15 were created AFTER Block 12 but are numbered sequentially after it. This causes Block 12's fallthrough logic to incorrectly target Block 13.

## Relevant CFG Code

### 1. processIfStatement (creates THEN/ELSE/AFTER blocks)

```cpp
void CFGBuilder::processIfStatement(const IfStatement& stmt, BasicBlock* currentBlock) {
    // IF creates conditional branch
    
    if (stmt.hasGoto) {
        // IF ... THEN GOTO creates two-way branch
        BasicBlock* nextBlock = createNewBlock();
        m_currentBlock = nextBlock;
    } else if (stmt.isMultiLine) {
        // Multi-line IF...END IF: Create separate blocks for THEN/ELSE branches
        BasicBlock* thenBlock = createNewBlock("IF THEN");
        BasicBlock* elseBlock = createNewBlock("IF ELSE");
        BasicBlock* afterIfBlock = createNewBlock("After IF");

        // Store the IF block info so buildEdges can create the proper conditional branch
        IfContext ifCtx;
        ifCtx.ifBlock = currentBlock->id;
        ifCtx.thenBlock = thenBlock->id;
        ifCtx.elseBlock = elseBlock->id;
        ifCtx.afterIfBlock = afterIfBlock->id;
        m_ifStack.push_back(ifCtx);
        
        // Process THEN statements into thenBlock
        m_currentBlock = thenBlock;
        if (!stmt.thenStatements.empty()) {
            processNestedStatements(stmt.thenStatements, thenBlock, stmt.location.line);
        }
        
        // Process ELSE statements into elseBlock
        m_currentBlock = elseBlock;
        if (!stmt.elseStatements.empty()) {
            processNestedStatements(stmt.elseStatements, elseBlock, stmt.location.line);
        }
        
        // Continue with afterIfBlock
        m_currentBlock = afterIfBlock;
    } else {
        // Single-line IF: handled differently
    }
}
```

### 2. buildEdges (adds successor edges for IF statements)

```cpp
case ASTNodeType::STMT_IF: {
    // Conditional branch
    const auto& ifStmt = static_cast<const IfStatement&>(*lastStmt);
    if (ifStmt.hasGoto) {
        // IF ... THEN GOTO handling
        int targetBlock = m_currentCFG->getBlockForLineOrNext(ifStmt.gotoLine);
        if (targetBlock >= 0) {
            addConditionalEdge(block->id, targetBlock, "true");
        }
        if (block->id + 1 < static_cast<int>(m_currentCFG->blocks.size())) {
            addConditionalEdge(block->id, block->id + 1, "false");
        }
    } else if (ifStmt.isMultiLine) {
        // Multi-line IF...END IF
        // Find the IfContext for this IF statement
        IfContext* ifCtx = nullptr;
        for (auto& ctx : m_ifStack) {
            if (ctx.ifBlock == block->id) {
                ifCtx = &ctx;
                break;
            }
        }
        
        if (ifCtx) {
            int thenBlockId = ifCtx->thenBlock;
            int elseBlockId = ifCtx->elseBlock;
            int afterIfBlockId = ifCtx->afterIfBlock;
            
            // Conditional branch from IF block to THEN/ELSE
            addConditionalEdge(block->id, thenBlockId, "true");
            addConditionalEdge(block->id, elseBlockId, "false");
            
            // Check if THEN block ends with a terminator
            const BasicBlock* thenBlock = m_currentCFG->getBlock(thenBlockId);
            bool thenHasTerminator = false;
            if (thenBlock && !thenBlock->statements.empty()) {
                ASTNodeType thenLastType = thenBlock->statements.back()->getType();
                thenHasTerminator = (thenLastType == ASTNodeType::STMT_GOTO ||
                                    thenLastType == ASTNodeType::STMT_RETURN ||
                                    thenLastType == ASTNodeType::STMT_EXIT ||
                                    thenLastType == ASTNodeType::STMT_END);
            }
            if (!thenHasTerminator) {
                addFallthroughEdge(thenBlockId, afterIfBlockId);
            }
            
            // Check if ELSE block ends with a terminator
            const BasicBlock* elseBlock = m_currentCFG->getBlock(elseBlockId);
            bool elseHasTerminator = false;
            if (elseBlock && !elseBlock->statements.empty()) {
                ASTNodeType elseLastType = elseBlock->statements.back()->getType();
                elseHasTerminator = (elseLastType == ASTNodeType::STMT_GOTO ||
                                    elseLastType == ASTNodeType::STMT_RETURN ||
                                    elseLastType == ASTNodeType::STMT_EXIT ||
                                    elseLastType == ASTNodeType::STMT_END);
            }
            if (!elseHasTerminator) {
                addFallthroughEdge(elseBlockId, afterIfBlockId);
            }
        }
    }
    break;
}
```

### 3. processForStatement (creates FOR loop blocks)

```cpp
void CFGBuilder::processForStatement(const ForStatement& stmt, BasicBlock* currentBlock) {
    // FOR creates: init block (with FOR statement), check block, body block, exit block
    
    // Init block contains the FOR statement (initialization)
    BasicBlock* initBlock = createNewBlock("FOR Init");
    
    // Create edge from current block to init block
    if (currentBlock->id != initBlock->id) {
        addFallthroughEdge(currentBlock->id, initBlock->id);
    }
    
    // Move the FOR statement to the init block
    if (!currentBlock->statements.empty() && currentBlock->statements.back() == &stmt) {
        int lineNum = 0;
        auto it = currentBlock->statementLineNumbers.find(&stmt);
        if (it != currentBlock->statementLineNumbers.end()) {
            lineNum = it->second;
        }
        
        currentBlock->statements.pop_back();
        currentBlock->statementLineNumbers.erase(&stmt);
        initBlock->addStatement(&stmt, lineNum);
    }
    
    // Check block evaluates the loop condition
    BasicBlock* loopCheck = createNewBlock("FOR Loop Check");
    loopCheck->isLoopHeader = true;
    
    BasicBlock* loopBody = createNewBlock("FOR Loop Body");
    BasicBlock* loopExit = createNewBlock("After FOR");
    loopExit->isLoopExit = true;
    
    // Add edges
    addFallthroughEdge(initBlock->id, loopCheck->id);
    addConditionalEdge(loopCheck->id, loopBody->id, "true");
    addConditionalEdge(loopCheck->id, loopExit->id, "false");
    
    // Track loop context
    LoopContext ctx;
    ctx.headerBlock = loopCheck->id;
    ctx.exitBlock = loopExit->id;
    ctx.varName = stmt.variable;
    m_loopStack.push_back(ctx);
    
    // Continue building in loop body
    m_currentBlock = loopBody;
}
```

## Questions

1. **Block ordering strategy**: Should we maintain sequential block numbering, or is it acceptable for nested structure blocks to be created out-of-order?

2. **Fallthrough edges**: How should we handle fallthrough edges when the "next sequential block" isn't actually the next block in control flow? For example, Block 12 (After FOR) is followed by Block 13 (IF THEN from inside the loop), but Block 12 should never flow to Block 13.

3. **IfContext cleanup**: Should we pop IfContext entries after processing them in buildEdges? Currently they accumulate in m_ifStack.

4. **Alternative approach**: Should we create all nested blocks BEFORE the parent's exit block to maintain sequential numbering? For example, when processing a FOR loop, we could recursively process all statements to create nested IF blocks before creating the "After FOR" block.

5. **Duplicate successors**: Why does Block 14 (empty ELSE block) show duplicate successors `[15, 15]`? The buildEdges code should only add one fallthrough edge.

## What Works

- Simple multi-line IF statements (not inside loops)
- Single-line IF statements  
- EXIT FOR outside of IF blocks
- FOR loops without nested control structures

## What Fails

- Multi-line IF inside FOR loops with EXIT FOR
- Multiple sequential FOR loops, each containing multi-line IFs
- Complex nested control structures (IF inside FOR inside another FOR)

## Architecture Notes

The CFG is built in two phases:
1. **buildBlocks**: Iterates through statements, creates blocks, adds statements to blocks, calls processXXXStatement for control structures
2. **buildEdges**: Iterates through blocks, examines their statements, adds successor edges based on control flow

The issue seems to be that processIfStatement creates blocks during buildBlocks (phase 1), but those blocks end up numbered after blocks that were created earlier in the same nesting level. Then in buildEdges (phase 2), fallthrough logic incorrectly assumes sequential blocks represent sequential control flow.

## Possible Solutions to Consider

1. **Defer block creation**: Don't create THEN/ELSE/AFTER blocks in processIfStatement. Instead, just mark the IF statement somehow, and create the blocks later in a separate pass before buildEdges.

2. **Reorder blocks**: After buildBlocks completes, renumber all blocks to match control flow order before running buildEdges.

3. **Explicit successors only**: Remove all automatic fallthrough edge logic. Require every block's successors to be explicitly set during processXXXStatement or buildEdges.

4. **Track parent context**: When creating nested blocks, store the parent's exit block so we know where to continue after the nested structure completes.

5. **Different IF handling**: Don't create separate blocks for multi-line IF. Instead, emit inline like single-line IF does, with labels for THEN/ELSE/ENDIF.

## Request for Advice

Which approach would be most robust? Are there standard CFG construction patterns for handling nested control structures that we should follow?

The current two-phase approach (buildBlocks then buildEdges) seems to work well for most constructs, but multi-line IF breaks the assumption that block IDs reflect control flow order.