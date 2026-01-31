# Control Flow Graph in FasterBASIC

## Overview

The Control Flow Graph (CFG) is a fundamental data structure that represents program execution paths. Originally designed for Lua code generation, the CFG has been significantly enhanced to support low-level QBE IL generation, making it more precise and rigorous.

**Status**: ✅ Fully operational for all control structures (FOR, WHILE, REPEAT, DO/LOOP, IF/ELSE, SELECT CASE, GOTO)

---

## Why CFG Matters

### Original Design: Lua/LuaJIT Backend
- High-level dynamic language target
- Built-in control flow primitives
- Runtime handles many details implicitly
- Forgiving of imprecise CFG structure

### Current State: QBE Backend
- Low-level SSA-based IL target
- Explicit representation of ALL control flow
- Every jump, branch, and edge must be defined
- Unforgiving: imprecise CFG = incorrect code

### Benefits
The improvements made for QBE benefit **all backends**:
- ✅ More accurate control flow representation
- ✅ Better optimization opportunities
- ✅ Easier to add new backends
- ✅ More maintainable codebase
- ✅ Better foundation for program analysis

---

## CFG Structure

### Basic Blocks

A basic block is a sequence of statements with:
- **Single entry point** (one predecessor or entry)
- **Single exit point** (one successor or multiple conditional)
- **No internal control flow** (straight-line code)

```basic
10 X = 5
20 Y = X + 10
30 PRINT Y
```

Becomes one basic block:
```
Block 0: [Lines 10, 20, 30]
  - X = 5
  - Y = X + 10
  - PRINT Y
  - Edge: FALLTHROUGH → Block 1 (exit)
```

### Edges

Edges represent control flow between blocks:

| Edge Type | Description | Example |
|-----------|-------------|---------|
| **Fallthrough** | Sequential execution | Line 10 → Line 20 |
| **Conditional** | Branch (true/false) | IF condition THEN... |
| **Unconditional** | Jump | GOTO, NEXT (back-edge) |
| **Call/Return** | Function invocation | GOSUB/RETURN |

---

## Control Structures

### IF/THEN/ELSE

```basic
IF condition THEN
    statement1
ELSE
    statement2
END IF
statement3
```

CFG:
```
Block 0: [condition evaluation]
  - Evaluate condition
  - Edge: CONDITIONAL (true) → Block 1
  - Edge: CONDITIONAL (false) → Block 2

Block 1: [THEN branch]
  - statement1
  - Edge: UNCONDITIONAL → Block 3

Block 2: [ELSE branch]
  - statement2
  - Edge: FALLTHROUGH → Block 3

Block 3: [after IF]
  - statement3
```

### FOR/NEXT Loops

```basic
FOR I = 1 TO 10 STEP 2
    PRINT I
NEXT I
```

CFG:
```
Block 0: [initialization]
  - I = 1
  - Store END value (10)
  - Store STEP value (2)
  - Edge: FALLTHROUGH → Block 1

Block 1: [loop header - condition check]
  - Check: I <= 10
  - Edge: CONDITIONAL (true) → Block 2
  - Edge: CONDITIONAL (false) → Block 3

Block 2: [loop body]
  - PRINT I
  - I = I + 2
  - Edge: UNCONDITIONAL → Block 1 (BACK-EDGE)

Block 3: [after loop]
  - (subsequent code)
```

**Key features**:
- Nested loops: Maintained via loop stack
- EXIT FOR: Jumps to loop exit block
- STEP handling: Positive/negative steps

### WHILE/WEND Loops

```basic
WHILE condition
    statement
WEND
```

CFG:
```
Block 0: [before loop]
  - Edge: FALLTHROUGH → Block 1

Block 1: [loop header]
  - Evaluate condition
  - Edge: CONDITIONAL (true) → Block 2
  - Edge: CONDITIONAL (false) → Block 3

Block 2: [loop body]
  - statement
  - Edge: UNCONDITIONAL → Block 1 (BACK-EDGE)

Block 3: [after loop]
```

### REPEAT/UNTIL Loops

```basic
REPEAT
    statement
UNTIL condition
```

CFG:
```
Block 0: [before loop]
  - Edge: FALLTHROUGH → Block 1

Block 1: [loop body + condition]
  - statement
  - Evaluate condition
  - Edge: CONDITIONAL (true) → Block 2 (exit)
  - Edge: CONDITIONAL (false) → Block 1 (BACK-EDGE)

Block 2: [after loop]
```

**Note**: Post-test loop — body executes at least once.

### DO/LOOP Variants

```basic
' Pre-test
DO WHILE condition
    statement
LOOP

' Post-test
DO
    statement
LOOP WHILE condition
```

Both variants supported with appropriate condition placement.

### SELECT CASE

```basic
SELECT CASE value
    CASE 1
        statement1
    CASE 2
        statement2
    CASE ELSE
        statement3
END SELECT
```

CFG:
```
Block 0: [evaluate selector]
  - Compute value
  - Edge: CONDITIONAL (value=1) → Block 1
  - Edge: CONDITIONAL (value=2) → Block 2
  - Edge: FALLTHROUGH → Block 3 (ELSE)

Block 1: [CASE 1]
  - statement1
  - Edge: UNCONDITIONAL → Block 4

Block 2: [CASE 2]
  - statement2
  - Edge: UNCONDITIONAL → Block 4

Block 3: [CASE ELSE]
  - statement3
  - Edge: FALLTHROUGH → Block 4

Block 4: [after SELECT]
```

### GOTO/GOSUB

```basic
10 GOTO 100
20 GOSUB 200
30 RETURN
```

CFG handles unstructured control flow:
- **GOTO**: Unconditional edge to target
- **GOSUB**: Call edge with return address
- **RETURN**: Return edge to caller

---

## Implementation Details

### Two-Phase Construction

**Phase 1: Build Blocks**
```cpp
void CFGBuilder::buildCFG(Program& program) {
    // Create basic blocks
    // Process statements into blocks
    // Track loop/function contexts
}
```

**Phase 2: Build Edges**
```cpp
void CFGBuilder::buildEdges() {
    // Connect blocks with edges
    // Handle loop back-edges
    // Add conditional branches
}
```

This separation ensures all blocks exist before creating edges.

### Loop Context Stack

```cpp
struct LoopContext {
    int headerBlock;     // Loop condition check
    int exitBlock;       // After loop
    std::string variable; // FOR loop variable
    std::string type;    // FOR, WHILE, REPEAT, DO
};

std::vector<LoopContext> m_loopStack;
```

Tracks nested loops for:
- EXIT FOR/WHILE/etc. statements
- NEXT/WEND/UNTIL matching
- Proper back-edge creation

### Function Context

```cpp
struct FunctionContext {
    std::string name;
    int entryBlock;
    int exitBlock;
    std::vector<std::string> parameters;
    VariableType returnType;
};
```

Separate CFG per function with isolated block numbering.

---

## Code Generation

### QBE IL Emission

The CFG drives QBE IL generation:

```cpp
void QBECodeGenerator::emitBlock(BasicBlock* block) {
    emit("@" + getBlockLabel(block->id) + "\n");
    
    // Emit statements
    for (auto* stmt : block->statements) {
        emitStatement(stmt);
    }
    
    // Emit control flow
    if (block->successors.size() == 0) {
        // No successors - explicit jump or return
    } else if (block->successors.size() == 1) {
        emit("    jmp @" + getBlockLabel(block->successors[0]) + "\n");
    } else if (block->successors.size() == 2) {
        // Conditional branch
        emit("    jnz " + m_lastCondition + ", @" + 
             getBlockLabel(block->successors[0]) + ", @" + 
             getBlockLabel(block->successors[1]) + "\n");
    }
}
```

### Block Label Mapping

```cpp
std::string getBlockLabel(int blockId) {
    return "block_" + std::to_string(blockId);
}

std::string getLineLabel(int lineNumber) {
    return "line_" + std::to_string(lineNumber);
}
```

---

## Key Improvements Made

### 1. REPEAT/UNTIL Fixed ✅

**Problem**: Missing back-edges, premature loop context pop

**Solution**:
- UNTIL creates new block after loop
- Loop context preserved until buildEdges()
- Proper conditional edges (true→exit, false→repeat)
- Fixed block ID comparison (>= instead of >)

### 2. FOR/NEXT Fixed ✅

**Problem**: No back-edges, no condition evaluation

**Solution**:
- Separate initialization, header, body, exit blocks
- Condition check in header block
- NEXT creates back-edge to header
- Proper loop increment/decrement

### 3. Block Terminator Tracking ✅

**Problem**: Duplicate jumps from statements and block emitter

**Solution**:
- Track statements that emit terminators (GOTO, END, RETURN)
- Block emitter checks before adding jumps
- Prevents redundant control flow

### 4. Conditional Branch Coordination ✅

**Problem**: Disconnect between condition evaluation and branch emission

**Solution**:
- Statements store condition in `m_lastCondition`
- Block emitter uses condition with CFG edges
- Consistent pattern across all conditionals

---

## Testing

### Test Programs

All control structures verified:
- [test_for_loop.bas](test_for_loop.bas) - FOR/NEXT variants
- [test_while.bas](fsh/test_while.bas) - WHILE/WEND
- [test_repeat.bas](fsh/test_repeat.bas) - REPEAT/UNTIL
- [test_do_loop.bas](fsh/test_do_loop.bas) - DO/LOOP variants
- [test_select.bas](fsh/test_select.bas) - SELECT CASE
- [test_goto.bas](fsh/test_goto.bas) - GOTO/GOSUB

### Verification

```bash
# Compile and run
./fsh/fbc_qbe test_for_loop.bas && ./test_for_loop

# Generate CFG dump (if enabled)
./fsh/fbc_qbe --dump-cfg test_program.bas
```

---

## Performance Characteristics

### CFG Construction
- **Time**: O(n) where n = number of statements
- **Space**: O(b + e) where b = blocks, e = edges
- Typically: blocks ≈ statements / 5

### CFG Analysis
- **Dominance**: O(b × e) — used for optimization
- **Loop detection**: O(b) — identifies natural loops
- **Reachability**: O(b + e) — finds dead code

---

## Future Enhancements

### Planned
1. **Dominance tree** - For advanced optimization
2. **Loop invariant detection** - Move calculations outside loops
3. **Dead code elimination** - Remove unreachable blocks
4. **Common subexpression elimination** - Reuse computed values

### Under Consideration
- SSA construction in CFG (currently in QBE)
- Interprocedural CFG (call graphs)
- Exception handling edges

---

## Related Documentation

- [README.md](README.md) - Project overview with control flow status
- [COERCION_STRATEGY.md](COERCION_STRATEGY.md) - Type handling
- [UserDefinedTypes.md](UserDefinedTypes.md) - UDT implementation

---

## Technical Reference

### Files

**Core CFG**:
- `fasterbasic_cfg.h` - CFG data structures
- `fasterbasic_cfg.cpp` - CFG builder implementation

**Code Generation**:
- `qbe_codegen_main.cpp` - Block emission
- `qbe_codegen_statements.cpp` - Statement handlers
- `qbe_codegen_expressions.cpp` - Expression evaluation

### Key Classes

```cpp
class BasicBlock {
    int id;
    std::vector<Statement*> statements;
    std::vector<int> successors;
    std::vector<int> predecessors;
    bool isLoopHeader;
    bool isLoopExit;
};

class ControlFlowGraph {
    std::vector<BasicBlock*> blocks;
    std::map<int, BasicBlock*> blockMap;
    std::map<std::string, ControlFlowGraph*> functionCFGs;
    VariableType returnType;
};

class CFGBuilder {
    void buildCFG(Program& program);
    void buildEdges();
    BasicBlock* createNewBlock(const std::string& name);
    void addEdge(int from, int to, const std::string& label);
};
```
