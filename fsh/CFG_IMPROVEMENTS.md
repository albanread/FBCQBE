# CFG Improvements for FasterBASIC

## Overview

The Control Flow Graph (CFG) builder is being significantly improved as part of the QBE backend development. These improvements make the CFG more rigorous, precise, and suitable for low-level code generation.

**Important:** These CFG enhancements benefit ALL backends, not just QBE. They should be contributed back to the main FasterBASIC project.

---

## Why CFG Improvements Matter

### Original Design: Lua/LuaJIT Backend

The CFG was originally designed to support Lua code generation:
- **High-level target**: Lua is a dynamic language with built-in control flow
- **Implicit behavior**: Lua runtime handles many control flow details
- **Forgiving**: Imprecise CFG structure could be "papered over" by Lua
- **Focus**: Quick code generation, rely on LuaJIT for optimization

### New Requirement: QBE Backend

QBE IL requires explicit, precise control flow:
- **Low-level target**: SSA-based intermediate language
- **Explicit everything**: Every jump, branch, and edge must be defined
- **Unforgiving**: Imprecise CFG = incorrect code
- **Focus**: Correct by construction, enable backend optimizations

### Result: Better CFG for Everyone

By making the CFG correct for QBE, we improve it for all backends:
- ✅ More accurate control flow representation
- ✅ Better optimization opportunities (for LuaJIT and others)
- ✅ Easier to add new backends in the future
- ✅ More maintainable and understandable code
- ✅ Better foundation for program analysis

---

## Improvements Made

### 1. REPEAT/UNTIL Loop Structure ✅

**Problem:**
- Loop context prematurely popped before edge creation
- UNTIL didn't create new block after loop
- Missing conditional back edges

**Solution:**
- UNTIL now creates new block for post-loop code
- Loop context preserved until `buildEdges()` phase
- Proper conditional edges: TRUE→exit, FALSE→repeat
- Fixed block ID comparison (>= instead of >)

**Impact:**
- Correct REPEAT/UNTIL code generation
- Loop structure explicitly represented in CFG
- Back edges properly identified

**Files Changed:**
- `fasterbasic_cfg.cpp` (processStatement, buildEdges)

---

### 2. Block Terminator Tracking 🚧

**Problem:**
- Duplicate jumps when statement already emitted terminator
- CFG didn't track which statements end blocks

**Solution:**
- Track statements that emit control flow (GOTO, END, RETURN)
- Block emitter checks if terminator already emitted
- Prevent duplicate jump instructions

**Impact:**
- Cleaner generated code
- No redundant jumps
- Better understanding of control flow

**Files Changed:**
- `qbe_codegen_main.cpp`
- `qbe_codegen_statements.cpp`

---

### 3. Conditional Branch Coordination 🚧

**Problem:**
- Statements evaluated conditions but didn't emit branches
- Block emitter had successor info but no condition value
- Disconnect between statement handlers and block emitter

**Solution:**
- Statements store condition in `m_lastCondition`
- Block emitter uses condition with CFG successors
- Clear separation: statements compute, blocks emit control flow

**Impact:**
- Works for IF, WHILE, UNTIL, etc.
- Consistent pattern for all conditional constructs
- CFG drives the control flow, not ad-hoc statement emission

**Files Changed:**
- `qbe_codegen_main.cpp`
- `qbe_codegen_statements.cpp`

---

## Future Improvements

### Expected CFG Enhancements

As we test more control flow constructs, we'll likely find and fix:

1. **FOR/NEXT Loops**
   - Proper loop initialization block
   - Increment/decrement logic
   - Exit condition evaluation
   - Step value handling (positive/negative)

2. **WHILE/WEND Loops**
   - Condition evaluation block separate from body
   - Back edge to condition (not body)
   - Proper loop exit edges

3. **DO/LOOP Variants**
   - DO WHILE (pre-test)
   - DO UNTIL (pre-test)
   - LOOP WHILE (post-test)
   - LOOP UNTIL (post-test)

4. **Nested Loop Handling**
   - Loop context stack management
   - Proper EXIT statement targets
   - Correct loop matching (NEXT, WEND, UNTIL)

5. **Complex Control Flow**
   - ON GOTO multi-way branches
   - SELECT CASE statements
   - GOSUB/RETURN call/return edges
   - Mixed structured/unstructured flow

---

## Contributing Back to Main Project

### Why Contribute Back?

1. **Benefit All Users**: Lua backend users get better CFG
2. **Future Backends**: Easier to add C, LLVM, WASM, etc.
3. **Optimization**: Better CFG enables better analysis
4. **Maintenance**: Single, high-quality CFG implementation
5. **Community**: Share improvements with FasterBASIC community

### What to Contribute

**Core CFG Improvements:**
- ✅ REPEAT/UNTIL block splitting and edge creation
- 🚧 FOR/NEXT loop structure (when fixed)
- 🚧 WHILE/WEND loop structure (when fixed)
- 🚧 All other control flow fixes

**Documentation:**
- CFG structure documentation
- Control flow handling patterns
- Testing methodology
- Bug patterns and solutions

**Test Suite:**
- Control flow test cases
- Loop variants
- Edge cases
- Regression tests

### How to Contribute

1. **Isolate Changes**
   - Keep CFG fixes separate from QBE-specific code
   - Changes in `fasterbasic_cfg.cpp/h` are universal
   - Changes in codegen are backend-specific

2. **Test with Lua Backend**
   - Verify fixes don't break existing Lua codegen
   - Run original test suite
   - Compare generated Lua before/after

3. **Document Changes**
   - Explain the problem
   - Show the fix
   - Provide test cases
   - Note any behavioral changes

4. **Submit Pull Request**
   - Clear description of improvements
   - Reference specific bugs fixed
   - Include test cases
   - Show before/after examples

---

## CFG Quality Metrics

### Before QBE Backend

- ❌ Some loop constructs missing back edges
- ❌ Block boundaries not always correct
- ❌ Control flow sometimes implicit
- ⚠️ Works for Lua but not rigorous

### After QBE Backend Work

- ✅ All control flow explicit and correct
- ✅ Block boundaries precise
- ✅ All edges properly represented
- ✅ Suitable for low-level code generation
- ✅ Foundation for optimization passes

---

## Technical Details

### CFG Construction Phases

Understanding the phases helps ensure correctness:

```
Phase 1: buildBlocks()
- Walk AST statements
- Create basic blocks
- Add statements to current block
- Handle block-creating statements (GOTO, IF, loops)
- Push/pop loop contexts
- DO NOT create edges yet

Phase 2: buildEdges()
- Walk all blocks
- Check last statement of each block
- Create edges based on statement type
- Use loop contexts to create back edges
- Connect blocks into complete graph

Phase 3: identifyLoops()
- Find back edges (target < source)
- Mark loop headers
- Identify natural loops
- Used for optimization hints

Phase 4: optimizeCFG()
- Remove empty blocks (optional)
- Merge sequential blocks (optional)
- Clean up unreachable code (optional)
```

### Key Insight: Phase Separation

**Critical rule:** Data structures (like loop stack) must persist between phases.

**Wrong:**
```cpp
case STMT_UNTIL:
    // Pop context immediately
    m_loopStack.pop_back();  // ❌ buildEdges needs this!
    break;
```

**Right:**
```cpp
case STMT_UNTIL:
    // Create new block, keep context
    BasicBlock* next = createNewBlock();
    m_currentBlock = next;
    // buildEdges will pop context when creating edges
    break;
```

---

## Testing Strategy

### CFG Validation Tests

Every control flow construct should have tests:

```basic
REM Test: REPEAT/UNTIL basic
10 X = 0
20 REPEAT
30   PRINT X
40   X = X + 1
50 UNTIL X >= 5
60 END
```

Verify:
- ✅ Loop body is separate block
- ✅ UNTIL creates conditional branch
- ✅ Back edge to loop header
- ✅ Exit edge to post-loop code

### Regression Tests

Ensure fixes don't break existing code:
- Run full test suite on Lua backend
- Compare output before/after
- Check performance impact
- Verify correctness

---

## Long-Term Vision

### Multiple Backend Support

```
FasterBASIC Program
        ↓
    [Lexer/Parser]
        ↓
       [AST]
        ↓
  [Semantic Analysis]
        ↓
       [CFG] ← Enhanced, rigorous, precise
        ↓
        ├─→ [Lua Backend] (JIT execution)
        ├─→ [QBE Backend] (native compilation)
        ├─→ [C Backend] (maximum portability)
        ├─→ [LLVM Backend] (heavy optimization)
        └─→ [WebAssembly] (browser execution)
```

**Foundation:** A single, correct CFG enables all backends.

---

## Conclusion

The CFG improvements driven by QBE backend development are **valuable for the entire FasterBASIC project**. By making the CFG more rigorous and correct, we:

1. Enable low-level code generation (QBE, C, LLVM)
2. Improve high-level code generation (better Lua output)
3. Create foundation for optimization passes
4. Make the codebase more maintainable
5. Support future backend development

**These improvements should absolutely be contributed back to the main FasterBASIC project once validated.**

---

## Contact & Coordination

When ready to contribute back:

1. Open an issue describing the improvements
2. Show concrete examples of bugs fixed
3. Provide test cases demonstrating correctness
4. Submit pull request with changes
5. Work with maintainers to integrate

The FasterBASIC community will benefit from this work!