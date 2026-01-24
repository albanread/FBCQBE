# FUNCTION/SUB Implementation Status

## What We Accomplished (Session Summary)

### 1. WHILE...WEND Loop Support ✅
- **CFG Support**: Proper loop header, body, and exit blocks
- **Edge Creation**: Conditional edges for condition, back edges to header
- **Nested Loops**: Fixed innermost loop matching with reverse search
- **Code Generation**: Proper condition evaluation and jnz emission
- **Testing**: Simple and nested WHILE loops working perfectly

### 2. SELECT CASE Support ✅
- **CFG Support**: Test blocks for each CASE, body blocks, ELSE block, exit block
- **Multi-way Branching**: Cascading comparisons with proper edges
- **Code Generation**: Comparison emission in test blocks, conditional jumps
- **Empty Block Handling**: Special handling before fallthrough logic
- **Testing**: All CASE values and ELSE clause working correctly

### 3. CFG Bug Fixes ✅
- Fixed REPEAT/UNTIL loop context management
- Fixed WHILE/WEND nested loop header detection (nesting counter)
- Fixed SELECT CASE test blocks being intercepted by empty block logic
- All improvements strengthen the entire compiler

---

## FUNCTION/SUB Implementation (In Progress)

### Architecture Decisions ✅

**Each function gets its own CFG** - This is the standard compiler approach:
```
ProgramCFG {
  mainCFG: ControlFlowGraph          // Main program
  functionCFGs: Map<name, CFG>       // Each function has its own CFG
}
```

**Why separate CFGs?**
- Encapsulation - Each function's control flow is independent
- Optimization - Can optimize each function separately
- Local reasoning - Don't need whole program analysis
- Standard practice - LLVM, GCC, Java, .NET all use this approach

### QBE Structure

```qbe
# Main program
export function w $main() {
@entry
    %result =d call $Square(d 5.0)
    %max =w call $Max(w 10, w 20)
    ret 0
}

# User-defined function
function d $Square(d %X) {
@entry
    %t0 =d mul %X, %X
    %Square =d copy %t0    # Return value (assigned to function name)
    ret %Square
}

# User-defined function with control flow
function w $Max(w %A, w %B) {
@entry
    %cond =w csgtw %A, %B
    jnz %cond, @then, @else
@then
    %Max =w copy %A
    jmp @exit
@else
    %Max =w copy %B
    jmp @exit
@exit
    ret %Max
}
```

---

## What's Been Done

### 1. CFG Structure Updates ✅

**File**: `fasterbasic_cfg.h`

**Added**:
```cpp
class ControlFlowGraph {
    std::string functionName;
    std::vector<std::string> parameters;
    std::vector<VariableType> parameterTypes;
    VariableType returnType;
    // ... existing block/edge data
};

class ProgramCFG {
    std::unique_ptr<ControlFlowGraph> mainCFG;
    std::unordered_map<std::string, std::unique_ptr<ControlFlowGraph>> functionCFGs;
    
    ControlFlowGraph* getFunctionCFG(const std::string& name);
    ControlFlowGraph* createFunctionCFG(const std::string& name);
    std::vector<std::string> getFunctionNames() const;
};
```

### 2. CFGBuilder Updates ✅

**File**: `fasterbasic_cfg.h` / `fasterbasic_cfg.cpp`

**Changed**:
- `build()` now returns `std::unique_ptr<ProgramCFG>` instead of single CFG
- Added `m_programCFG` and `m_currentCFG` (pointer to CFG being built)
- Added `processFunctionStatement()` and `processSubStatement()`

**Implementation**:
```cpp
void CFGBuilder::processFunctionStatement(const FunctionStatement& stmt, BasicBlock* currentBlock) {
    // 1. Create new CFG for function
    ControlFlowGraph* funcCFG = m_programCFG->createFunctionCFG(stmt.functionName);
    
    // 2. Store metadata (parameters, return type)
    funcCFG->functionName = stmt.functionName;
    funcCFG->parameters = stmt.parameters;
    funcCFG->parameterTypes = /* convert from TokenType */;
    funcCFG->returnType = /* extract from statement */;
    
    // 3. Save context and switch to function CFG
    ControlFlowGraph* savedCFG = m_currentCFG;
    BasicBlock* savedBlock = m_currentBlock;
    m_currentCFG = funcCFG;
    
    // 4. Build function CFG
    BasicBlock* entryBlock = createNewBlock("Function Entry");
    funcCFG->entryBlock = entryBlock->id;
    m_currentBlock = entryBlock;
    
    // 5. Process function body
    for (const auto& bodyStmt : stmt.body) {
        processStatement(*bodyStmt, m_currentBlock, 0);
    }
    
    // 6. Create exit block
    BasicBlock* exitBlock = createNewBlock("Function Exit");
    funcCFG->exitBlock = exitBlock->id;
    
    // 7. Restore context
    m_currentCFG = savedCFG;
    m_currentBlock = savedBlock;
}
```

**All m_cfg references updated to m_currentCFG** ✅

### 3. QBE Code Generator Updates ⚠️ PARTIAL

**File**: `fasterbasic_qbe_codegen.h` / `qbe_codegen_main.cpp`

**Changed**:
- `generate()` accepts `ProgramCFG` instead of single `ControlFlowGraph`
- Added `m_programCFG` member
- Added `emitFunction(const std::string& functionName)` declaration

**Implementation Started**:
```cpp
std::string QBECodeGenerator::generate(const ProgramCFG& programCFG, ...) {
    m_programCFG = &programCFG;
    
    // Emit main
    m_cfg = m_programCFG->mainCFG.get();
    emitMainFunction();
    
    // Emit user functions
    for (const auto& funcName : m_programCFG->getFunctionNames()) {
        m_cfg = m_programCFG->getFunctionCFG(funcName);
        emitFunction(funcName);  // ⚠️ NOT YET IMPLEMENTED
    }
    
    emitDataSection();
    return m_output.str();
}
```

---

## What Still Needs to Be Done

### 1. QBE Code Generator - Emit Functions ❌

**Task**: Implement `emitFunction()` method

**Needs**:
```cpp
void QBECodeGenerator::emitFunction(const std::string& functionName) {
    // 1. Emit function signature
    //    function <type> $functionName(param1_type %param1, param2_type %param2) {
    
    // 2. Emit function entry block
    
    // 3. Emit all basic blocks (same as main, use emitBlock())
    
    // 4. Emit function exit
    //    @exit
    //        ret %functionName   (return value stored in function name variable)
    //    }
}
```

**Details**:
- Map VariableType to QBE type: `INT→w, FLOAT→s, DOUBLE→d, STRING→l`
- Emit parameters: `function d $Square(d %X)`
- Handle function name variable (return value): `%Square =d copy %result`
- Emit return statement: `ret %Square`

### 2. Function Call Handling ❌

**Task**: Distinguish user functions from built-in functions

**Current Problem**: `Add(3, 4)` emits as `$basic_ADD` (built-in)

**Needs**:
```cpp
std::string QBECodeGenerator::emitFunctionCall(const std::string& funcName, 
                                                const std::vector<std::string>& args) {
    // Check if user-defined function
    if (m_programCFG->getFunctionCFG(funcName) != nullptr) {
        // User function: emit QBE call
        std::string result = allocTemp("w");  // TODO: get actual return type
        emit("    " + result + " =w call $" + funcName + "(");
        for (size_t i = 0; i < args.size(); i++) {
            if (i > 0) emit(", ");
            emit("w " + args[i]);  // TODO: get actual param type
        }
        emit(")\n");
        return result;
    } else {
        // Built-in function: use runtime library
        return emitBuiltinFunction(funcName, args);
    }
}
```

**Integration Point**: Modify `emitExpression()` for `FunctionCallExpression`

### 3. Function Name as Return Variable ❌

**BASIC Semantics**:
```basic
FUNCTION Square(X AS DOUBLE) AS DOUBLE
    Square = X * X    ' Assigns to function name
END FUNCTION
```

**QBE Implementation**:
```qbe
function d $Square(d %X) {
@entry
    # Declare function name variable
    %Square =d copy 0.0     # Initialize
    
    # Function body
    %t0 =d mul %X, %X
    %Square =d copy %t0     # Assign to function name
    
    # Return
    ret %Square
}
```

**Needs**:
- Track that function name is a variable inside function
- Initialize it at function entry
- Return it at function exit

### 4. EXIT FUNCTION / EXIT SUB ❌

**Semantics**: Early return from function

**Implementation**:
```cpp
void QBECodeGenerator::emitExit(const ExitStatement* stmt) {
    if (m_inFunction) {
        // Jump to function exit block
        emit("    jmp @exit\n");
    } else {
        // In main program - jump to main exit
        emit("    jmp @exit\n");
    }
}
```

**Needs**:
- Track `m_inFunction` flag
- EXIT FUNCTION jumps to function's exit block
- EXIT SUB jumps to SUB's exit block

### 5. Local vs Global Variables ❌

**Scoping**:
- Parameters are local to function
- Variables declared with DIM/LOCAL are local
- Variables in main are global
- Need to prefix local variables differently: `%local_X` vs `%var_X`

### 6. BYREF Parameter Passing ❌

**BASIC Semantics**:
```basic
SUB Swap(BYREF A AS INTEGER, BYREF B AS INTEGER)
    LET Temp = A
    LET A = B
    LET B = Temp
END SUB
```

**Implementation**: Need to pass pointers and load/store through them

### 7. Recursive Functions ❌

**Should work automatically** once functions are emitting correctly, since each call gets its own stack frame (QBE handles this).

### 8. Update Compiler Driver ❌

**Files to Update**:
- `fbc_qbe.cpp` - Main compiler driver

**Change**:
```cpp
// Old:
auto cfg = cfgBuilder.build(program, symbolTable);
std::string qbeIL = generateQBECode(*cfg, symbolTable, options);

// New:
auto programCFG = cfgBuilder.build(program, symbolTable);
std::string qbeIL = generateQBECode(*programCFG, symbolTable, options);
```

---

## Implementation Priority

### Phase 1: Basic Function Emission (2-3 hours)
1. ✅ CFG structure for multiple functions
2. ✅ CFGBuilder creates separate CFGs
3. ⚠️ QBE generator emits function signatures
4. ⚠️ Function call recognition and emission
5. ⚠️ Function name as return variable
6. ⚠️ Update compiler driver

### Phase 2: Advanced Features (1-2 hours)
7. EXIT FUNCTION/SUB
8. Local variable scoping
9. Proper type handling (parameters, return values)

### Phase 3: Complex Features (2-3 hours)
10. BYREF parameters
11. Recursive functions (should work automatically)
12. Function calls inside expressions

---

## Testing Plan

### Test 1: Simple Function ✅ (Parses, CFG builds)
```basic
FUNCTION Add(X, Y)
    Add = X + Y
END FUNCTION

LET Result = Add(3, 4)
PRINT Result
```

**Status**: Parses ✅, CFG builds ✅, Code generation ❌ (emits "Unhandled statement type 87")

### Test 2: Function with Types
```basic
FUNCTION Square(X AS DOUBLE) AS DOUBLE
    Square = X * X
END FUNCTION

LET Result = Square(5.0)
PRINT "Square of 5 is "; Result
```

### Test 3: Function with Control Flow
```basic
FUNCTION Max(A AS INTEGER, B AS INTEGER) AS INTEGER
    IF A > B THEN
        Max = A
    ELSE
        Max = B
    END IF
END FUNCTION

PRINT Max(10, 20)
```

### Test 4: SUB without Return Value
```basic
SUB PrintMessage(Msg AS STRING)
    PRINT "Message: "; Msg
END SUB

CALL PrintMessage("Hello!")
```

### Test 5: EXIT FUNCTION
```basic
FUNCTION SafeDivide(A AS DOUBLE, B AS DOUBLE) AS DOUBLE
    IF B = 0 THEN
        SafeDivide = 0
        EXIT FUNCTION
    END IF
    SafeDivide = A / B
END FUNCTION
```

### Test 6: Recursive Function
```basic
FUNCTION Factorial(N AS INTEGER) AS INTEGER
    IF N <= 1 THEN
        Factorial = 1
    ELSE
        Factorial = N * Factorial(N - 1)
    END IF
END FUNCTION

PRINT Factorial(5)   ' Should print 120
```

---

## Key Design Insights

### 1. Function as Return Variable
This is **classic BASIC semantics** (QuickBASIC, Visual Basic):
- The function name acts as a local variable
- Assigning to it sets the return value
- At END FUNCTION, the value is returned

### 2. Separate CFGs are Essential
- Can't just inline functions - they need their own scope
- Each function can have complex control flow (IF, WHILE, SELECT CASE)
- Functions can be recursive - need proper calling convention

### 3. QBE Makes This Natural
QBE has proper function support:
- Function definitions with parameters
- Call instruction with return values
- Return instruction
- Stack frames handled automatically

---

## Next Steps

**Immediate (to get basic functions working)**:
1. Implement `emitFunction()` method in QBE code generator
2. Add function call recognition (check ProgramCFG for user functions)
3. Emit function name variable and return statement
4. Update compiler driver to pass ProgramCFG
5. Test with simple function example

**Then**:
6. Add EXIT FUNCTION support
7. Handle parameter types properly
8. Implement local variable scoping
9. Test with all test cases

---

## Current Files Modified

1. ✅ `fasterbasic_cfg.h` - Added ProgramCFG, updated CFGBuilder interface
2. ✅ `fasterbasic_cfg.cpp` - Updated build() to create ProgramCFG, added processFunctionStatement/processSubStatement
3. ⚠️ `fasterbasic_qbe_codegen.h` - Updated generate() signature, added emitFunction() declaration
4. ⚠️ `qbe_codegen_main.cpp` - Updated generate() to iterate over functions (emitFunction not implemented)

**Status**: Infrastructure is in place, need to complete code generation implementation.

---

## Estimated Completion Time

- **Phase 1 (Basic Functions)**: 2-3 hours
- **Phase 2 (Advanced Features)**: 1-2 hours  
- **Phase 3 (Complex Features)**: 2-3 hours
- **Total**: 5-8 hours of focused development

**The hardest part (multi-function CFG architecture) is done!** ✅

Now it's "just" code generation - emitting the right QBE IL for functions.