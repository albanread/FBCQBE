# ON CALL Implementation Summary

**Date:** February 4, 2025  
**Status:** ✅ IMPLEMENTED AND TESTED

## Overview

This document describes the implementation of the BASIC `ON CALL` statement in the FasterBASIC → QBE compiler. This feature allows computed dispatch to named SUB procedures based on a selector value.

## Syntax

```basic
ON <expression> CALL Sub1, Sub2, Sub3, ...
```

**Behavior:**
- Evaluates `<expression>` to an integer (1-based index)
- If value is 1, calls Sub1
- If value is 2, calls Sub2
- If value is 3, calls Sub3
- If value is 0, negative, or > N, falls through (no call made)
- Always continues to next statement after SUB returns or fallthrough

## Comparison with Related Statements

| Statement | Purpose | Target Type | Arguments | Return Behavior |
|-----------|---------|-------------|-----------|-----------------|
| `ON x GOTO` | Jump to line number | Line numbers | N/A | Does not return |
| `ON x GOSUB` | Call line-numbered subroutine | Line numbers | N/A | Returns to next line |
| `ON x CALL` | Call named SUB | SUB names | No (simple version) | Returns to next line |
| `CALL SubName()` | Direct SUB call | SUB name | Yes | Returns to next line |

**Key Differences:**
- `ON GOTO/GOSUB` use line numbers (e.g., 100, 200, 300)
- `ON CALL` uses named SUBs (e.g., ProcessOption, HandleInput, ShowMenu)
- `ON CALL` is more structured and maintainable than line-numbered alternatives

## Implementation Architecture

### 1. AST Node (Already Existed)

**File:** `fsh/FasterBASICT/src/fasterbasic_ast.h`

```cpp
class OnCallStatement : public Statement {
public:
    ExpressionPtr selector;                  // Expression to evaluate (1-based)
    std::vector<std::string> functionNames;  // List of SUB names to call
    
    void addTarget(const std::string& name);
    ASTNodeType getType() const override { return ASTNodeType::STMT_ON_CALL; }
};
```

**Current Limitation:** Only supports SUB names without arguments. Each SUB must have same signature (currently no arguments).

### 2. Parser Support (Already Existed)

**File:** `fsh/FasterBASICT/src/fasterbasic_parser.cpp`

The parser already recognized the `ON <expr> CALL` syntax and populated the AST node.

### 3. CFG Builder Handler (NEW)

**File:** `fsh/FasterBASICT/src/cfg/cfg_builder_jumps.cpp`

**Function:** `BasicBlock* CFGBuilder::handleOnCall(const OnCallStatement&, ...)`

**CFG Structure Created:**
```
[Incoming Block]
  └─> [OnCall_Continue Block]
      (with edges labeled: call_sub:SubName:case_N)
```

**Edge Labels:**
- Format: `"call_sub:<SubName>:case_<N>"`
- Example: `"call_sub:PrintMenu:case_2"`
- Special: `"call_default"` for fallthrough

**Key Implementation Details:**
- Creates conditional edges for each SUB target
- All paths lead to a single continuation block
- Edge labels encode both the SUB name and case number
- CFG structure allows emitter to generate switch dispatch

### 4. Dispatch Integration (NEW)

**Files Modified:**
- `fsh/FasterBASICT/src/cfg/cfg_builder_core.cpp` - Added `OnCallStatement` dispatch
- `fsh/FasterBASICT/src/cfg/cfg_builder_functions.cpp` - Added `OnCallStatement` dispatch

**Pattern:**
```cpp
if (auto* onCallStmt = dynamic_cast<const OnCallStatement*>(stmt.get())) {
    currentBlock = handleOnCall(*onCallStmt, currentBlock, ...);
    continue;
}
```

### 5. Code Generation (NEW)

**File:** `fsh/FasterBASICT/src/codegen_v2/cfg_emitter.cpp`

**Function:** `void CFGEmitter::emitOnCallTerminator(const OnCallStatement*, ...)`

**Generated QBE Pattern:**

```qbe
# Evaluate and normalize selector
%selector =d loadd %var_choice_DOUBLE
%selector =w dtosi %selector          # Convert to integer
%selector =w sub %selector, 1         # 1-based → 0-based

# Generate comparison chain
%cmp1 =w ceqw %selector, 0
jnz %cmp1, @on_call_trampoline_0_case_0, @switch_next_0

@switch_next_0
%cmp2 =w ceqw %selector, 1
jnz %cmp2, @on_call_trampoline_0_case_1, @switch_next_1

@switch_next_1
%cmp3 =w ceqw %selector, 2
jnz %cmp3, @on_call_trampoline_0_case_2, @continue_block

# Trampolines (one per SUB)
@on_call_trampoline_0_case_0
    call $sub_Sub1()                   # Note: $sub_ prefix
    jmp @continue_block

@on_call_trampoline_0_case_1
    call $sub_Sub2()
    jmp @continue_block

@on_call_trampoline_0_case_2
    call $sub_Sub3()
    jmp @continue_block

# Continuation point
@continue_block
    # Next statement here
```

**Code Generation Strategy:**
1. Evaluate selector once, cache in register
2. Generate linear comparison chain (like ON GOTO)
3. Each match jumps to a trampoline
4. Trampolines call the SUB with proper name prefix (`$sub_`)
5. All trampolines jump to common continuation point

### 6. SUB Name Resolution (FIXED)

**Issue:** SUBs are compiled as `$sub_<Name>` but initial implementation called `$<Name>`

**Fix:** Add `"sub_"` prefix when emitting CALL instruction:
```cpp
builder_.emitCall("", "", "sub_" + subNames[i], "");
```

This matches the naming convention used by the SUB code generator.

## Test Results

### ✅ Basic Functionality Test

```basic
SUB PrintOne()
  PRINT "Called PrintOne"
END SUB

SUB PrintTwo()
  PRINT "Called PrintTwo"
END SUB

LET choice = 2
ON choice CALL PrintOne, PrintTwo
PRINT "Back in main"
```

**Output:**
```
Called PrintTwo
Back in main
```

### ✅ Comprehensive Edge Cases Test

All test cases passed:
- ✅ Choice = 1: Calls first SUB
- ✅ Choice = 2: Calls middle SUB
- ✅ Choice = 3: Calls last SUB
- ✅ Choice = 0: Falls through (no call)
- ✅ Choice = 5 (out of range): Falls through
- ✅ Choice = -1: Falls through
- ✅ Execution continues after SUB returns

## Performance Characteristics

### Time Complexity

Same as ON GOTO/GOSUB:
- **Best case:** O(1) - first SUB selected
- **Worst case:** O(N) - last SUB or out-of-range
- **Average:** O(N/2) - linear search with short-circuit

### Instruction Overhead

| Operation | Instructions |
|-----------|-------------|
| Selector evaluation | 3 (load + convert + subtract) |
| Per comparison | 2 (ceqw + jnz) |
| Per trampoline | 2 (call + jmp) |
| **Total for N SUBs, match case K** | ~3 + 2×K + 2 |

**Example:** For 5 SUBs, match case 3:
- Selector: 3 instructions
- Comparisons: 2×3 = 6 instructions
- Trampoline: 2 instructions
- **Total: 11 instructions** (very efficient)

### Comparison with Direct CALL

```basic
' ON CALL (dynamic dispatch)
ON choice CALL Sub1, Sub2, Sub3
' Cost: ~11 instructions for case 3

' Equivalent IF chain
IF choice = 1 THEN CALL Sub1()
IF choice = 2 THEN CALL Sub2()
IF choice = 3 THEN CALL Sub3()
' Cost: Similar (~12 instructions)

' Advantage: ON CALL is more concise and readable
```

## Use Cases

### ✅ Perfect For:

1. **Menu Systems**
```basic
PRINT "1) New Game"
PRINT "2) Load Game"
PRINT "3) Options"
INPUT choice
ON choice CALL NewGame, LoadGame, ShowOptions
```

2. **State Machine Handlers**
```basic
' Call appropriate handler for current state
ON gameState CALL StateMenu, StatePlay, StatePause, StateOver
```

3. **Command Dispatch**
```basic
' Map command number to handler
LET cmdNum = ParseCommand(input$)
ON cmdNum CALL CmdNew, CmdOpen, CmdSave, CmdClose
```

4. **Event Handlers**
```basic
' Dispatch to event-specific handler
ON eventType CALL OnClick, OnKeyPress, OnTimer
```

### ⚠️ Limitations:

1. **No Arguments** (current simple implementation)
   ```basic
   ' NOT YET SUPPORTED:
   ON x CALL Sub1(a, b), Sub2(c), Sub3()
   ```
   
   **Workaround:** Use global variables or call wrapper SUBs

2. **Same Signature Required**
   - All SUBs in the list must have same signature (currently: no parameters)

3. **SUBs Only, Not FUNCTIONs**
   - Can only call SUBs (procedures), not FUNCTIONs (with return values)
   - Use `CALL FuncName()` directly if you need to ignore function return value

## Future Enhancements

### 1. Argument Support (Medium Priority)

Allow passing arguments to each SUB:

```basic
' Proposed syntax:
ON choice CALL ProcessInt(x), ProcessFloat(y), ProcessString(s$)
```

**Implementation Complexity:** Medium
- Parser needs to parse argument lists per target
- AST node needs `std::vector<std::vector<ExpressionPtr>>` for arguments
- Code generator needs to evaluate and pass different arguments per case

### 2. Mixed Target Types (Low Priority)

Allow mixing SUBs and FUNCTIONs:

```basic
ON choice CALL Sub1, Func2, Sub3   ' Ignore Func2's return value
```

### 3. Jump Table Optimization (Low Priority)

For large N (20+ targets), generate jump table instead of linear search:

```qbe
data $call_table = { l @trampoline_0, l @trampoline_1, ... }
%offset =l mul %selector, 8
%target =l loadl $call_table[%offset]
jmp %target
```

**Benefit:** O(1) constant time instead of O(N)

## Files Modified/Created

### Modified Files:

1. **CFG Builder:**
   - `fsh/FasterBASICT/src/cfg/cfg_builder.h` - Added `handleOnCall` declaration
   - `fsh/FasterBASICT/src/cfg/cfg_builder_jumps.cpp` - Implemented `handleOnCall`
   - `fsh/FasterBASICT/src/cfg/cfg_builder_core.cpp` - Added dispatch
   - `fsh/FasterBASICT/src/cfg/cfg_builder_functions.cpp` - Added dispatch

2. **Code Generator:**
   - `fsh/FasterBASICT/src/codegen_v2/cfg_emitter.h` - Added `emitOnCallTerminator` declaration
   - `fsh/FasterBASICT/src/codegen_v2/cfg_emitter.cpp` - Implemented terminator, added detection

### Existing (Used As-Is):

- `fsh/FasterBASICT/src/fasterbasic_ast.h` - `OnCallStatement` class
- `fsh/FasterBASICT/src/fasterbasic_parser.cpp` - ON CALL parsing

## Example Programs

### Menu System

```basic
100 REM Restaurant ordering system
110
120 SUB OrderPizza()
130   PRINT "Pizza ordered!"
140 END SUB
150
160 SUB OrderBurger()
170   PRINT "Burger ordered!"
180 END SUB
190
200 SUB OrderSalad()
210   PRINT "Salad ordered!"
220 END SUB
230
240 SUB ShowBill()
250   PRINT "Total: $15.00"
260 END SUB
270
280 REM Main loop
290 CLS
300 PRINT "=== RESTAURANT MENU ==="
310 PRINT "1) Pizza"
320 PRINT "2) Burger"
330 PRINT "3) Salad"
340 PRINT "4) Show Bill"
350 PRINT "5) Exit"
360 INPUT "Select option: ", choice
370 IF choice = 5 THEN END
380 IF choice < 1 OR choice > 4 THEN GOTO 290
390 ON choice CALL OrderPizza, OrderBurger, OrderSalad, ShowBill
400 PRINT ""
410 INPUT "Press Enter to continue...", dummy$
420 GOTO 290
```

### Game State Machine

```basic
100 REM Game with multiple states
110 LET STATE_MENU = 1
120 LET STATE_PLAY = 2
130 LET STATE_PAUSE = 3
140 LET currentState = STATE_MENU
150
160 SUB HandleMenu()
170   PRINT "In menu state"
180   ' Menu logic here
190   LET currentState = STATE_PLAY
200 END SUB
210
220 SUB HandlePlay()
230   PRINT "Playing game"
240   ' Game logic here
250 END SUB
260
270 SUB HandlePause()
280   PRINT "Game paused"
290   ' Pause logic here
300 END SUB
310
320 REM Main game loop
330 ON currentState CALL HandleMenu, HandlePlay, HandlePause
340 GOTO 330
```

## Comparison with Other Languages

| Language | Equivalent Feature | Syntax |
|----------|-------------------|--------|
| **FasterBASIC** | ON CALL | `ON x CALL Sub1, Sub2, Sub3` |
| **C** | Function pointer array | `handlers[x]();` |
| **C++** | std::function array | `handlers[x]();` |
| **Python** | List of functions | `handlers[x]()` |
| **JavaScript** | Array of functions | `handlers[x]()` |
| **Go** | Slice of functions | `handlers[x]()` |

**FasterBASIC Advantage:** More readable than function pointer arrays for beginners, while providing similar functionality.

## Conclusion

The `ON CALL` statement successfully extends the ON GOTO/GOSUB family to support modern structured programming with named SUBs. It provides:

✅ **Clean syntax** - Easy to read and understand  
✅ **Efficient execution** - O(N/2) average, O(1) best case  
✅ **Type safety** - SUB names checked at compile time  
✅ **Maintainability** - Named SUBs easier to refactor than line numbers  
✅ **Full edge case handling** - Out-of-range values fall through safely  

The implementation integrates seamlessly with the existing ON GOTO/GOSUB infrastructure and follows the same efficient code generation patterns.

---

**Implementation by:** FasterBASIC Development Team  
**Compiler Version:** v2 (CFG-aware code generation)  
**Last Updated:** February 4, 2025