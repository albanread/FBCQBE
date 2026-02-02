# FOR Loop Variable Handling in SUB/FUNCTION Issue

## Status: INCOMPLETE / BREAKS TESTS

The FOR loop variable handling inside SUBs and FUNCTIONs has a critical bug that prevents compilation of tests like `test_abs_sgn.bas`.

## The Problem

When a FOR loop is used inside a SUB or FUNCTION, the loop variable is not being properly handled, resulting in QBE compilation errors:

```
qbe:tests/arithmetic/test_abs_sgn.bas:324: invalid type for second operand %var_i_INT in storew
```

### Example Code That Fails

```basic
SUB test_loop()
  LOCAL i AS INTEGER
  LOCAL sum AS DOUBLE
  sum = 0.0
  FOR i = -5 TO 5      ' <-- This FOR loop causes the error
    sum = sum + ABS(i)
  NEXT i
  PRINT "Sum = "; sum
END SUB
```

### Root Cause

The issue stems from inconsistent handling of FOR loop variables across multiple compiler layers:

1. **Symbol Table Registration**: FOR loop variables inside SUBs need to be registered with:
   - Scoped keys (e.g., `"functionName::varName"`)
   - Proper `functionScope` field set
   - Marked as function-local, not global

2. **Variable Name Mangling**: The variable name needs consistent mangling:
   - Parser strips type suffixes (`i%` → `i`)
   - Semantic analyzer normalizes FOR variables with uppercase text suffixes (`i` → `i_INT`)
   - Codegen must use the same normalized form when emitting allocation and references

3. **Variable Allocation**: Function-local FOR variables need explicit allocation in QBE IL:
   ```qbe
   %var_i_INT =l alloc4 4
   ```
   Without this allocation, stores/loads to the variable fail.

4. **Variable Tracking**: FOR loop variables are tracked in `m_forLoopVariables` set, but this was global and caused cross-function interference. Each function's FOR variables should be tracked separately using scoped keys.

## Previous Attempted Fix

An attempt was made to fix this issue with changes to:
- `fasterbasic_semantic.cpp`: Function-scoped FOR variable registration
- `fasterbasic_semantic.h`: Scoped FOR variable tracking
- `ast_emitter.cpp`: FOR variable allocation in function scope

### Changes Made (Caused Regressions)

```cpp
// In SemanticAnalyzer::validateForStatement():
if (m_currentFunctionScope.inFunction) {
    // Use scoped key for local variables
    std::string scopedKey = m_currentFunctionScope.functionName + "::" + plainVarName;
    m_forLoopVariables.insert(scopedKey);
    
    VariableSymbol varSym;
    varSym.name = plainVarName;
    varSym.isDeclared = true;
    varSym.firstUse = stmt.location;
    varSym.functionScope = m_currentFunctionScope.functionName;
    varSym.isGlobal = false;
    varSym.typeDesc = TypeDescriptor(forVarType);
    
    m_symbolTable.variables[scopedKey] = varSym;
}
```

```cpp
// In ASTEmitter::emitForInit():
if (symbolMapper_.inFunctionScope()) {
    std::string currentFunc = symbolMapper_.getCurrentFunction();
    std::string lookupName = normalizeForLoopVarName(stmt->variable);
    const auto* varSymbol = semantic_.lookupVariableScoped(lookupName, currentFunc);
    
    if (varSymbol && !varSymbol->functionScope.empty()) {
        std::string mangledName = symbolMapper_.mangleVariableName(lookupName, false);
        BaseType varType = varSymbol->typeDesc.baseType;
        int64_t size = typeManager_.getTypeSize(varType);
        
        builder_.emitComment("Allocate FOR loop variable: " + stmt->variable);
        if (size == 4) {
            builder_.emitRaw("    " + mangledName + " =l alloc4 4");
        }
    }
}
```

### Why It Failed

These changes **broke 57 tests**, reducing the pass rate from **112 passes to 55 passes**.

The issue is likely that:
1. The scoped key approach interfered with existing variable lookup logic
2. Some tests rely on FOR variables being accessible across different scopes
3. The normalization logic (`normalizeForLoopVarName`) may have side effects on other variable handling
4. The explicit allocation may conflict with existing allocation logic elsewhere in the compiler

## Related Working Code

The old codegen (`codegen_old/`) and the main codegen have different approaches to FOR loop handling. The v2 codegen may be missing some logic that exists in the older implementations.

## Current Workaround

Currently, FOR loops work correctly in:
- Global/main scope
- Programs without SUBs/FUNCTIONs

FOR loops DO NOT work in:
- SUBs with LOCAL variables
- FUNCTIONs with parameters or local variables

## Tests Affected

Directly fails to compile:
- `tests/arithmetic/test_abs_sgn.bas` - has FOR loop in SUB test_loop()
- Any other test with FOR loops inside SUBs/FUNCTIONs

## Recommended Next Steps

1. **Isolate the Problem**: Create a minimal test case with just a FOR loop in a SUB
   ```basic
   SUB test_for()
     FOR i = 1 TO 3
       PRINT i
     NEXT i
   END SUB
   
   CALL test_for()
   ```

2. **Debug the QBE IL**: Generate QBE IL with `-i` flag and examine:
   - Where is the variable allocated (or not allocated)?
   - What mangled name is used for allocation vs. store/load?
   - Is there a mismatch between variable names?

3. **Check Variable Lifecycle**: Trace through the entire compiler pipeline:
   - Parser: What name is stored in the ForStatement?
   - Semantic: How is the variable registered in the symbol table?
   - Codegen: What name is used for allocation? For store/load?

4. **Review Working Examples**: Look at how WHILE loops handle their condition variables in SUBs - do they work? What's different?

5. **Incremental Fix**: Instead of changing all three layers at once:
   - First fix just the symbol table registration
   - Test if that alone helps or breaks anything
   - Then add allocation logic
   - Then add scoped tracking

6. **Consider Alternative Approach**: Maybe FOR loop variables should be treated more like parameters - explicitly passed through the semantic/codegen interface rather than being "discovered" from the symbol table.

## Related Files

- `fsh/FasterBASICT/src/fasterbasic_semantic.cpp` - Symbol table and FOR validation
- `fsh/FasterBASICT/src/fasterbasic_semantic.h` - FOR variable tracking
- `fsh/FasterBASICT/src/codegen_v2/ast_emitter.cpp` - FOR loop code emission
- `fsh/FasterBASICT/src/codegen_v2/symbol_mapper.cpp` - Variable name mangling
- `fsh/FasterBASICT/src/cfg/cfg_builder_statements.cpp` - CFG FOR loop handling

## Test Status

With attempted fix:
- **55 tests passing** (down from 112)
- **67 compile failures** (up from ~10)
- **57 test regressions**

Without fix:
- **112 tests passing**
- `test_abs_sgn.bas` cannot compile (but was mentioned as working in thread context?)

## Thread Context Note

The previous thread discussion indicated that `test_abs_sgn.bas` was compiling and running successfully after applying the FOR loop fixes. However, when the fixes are reverted to test the SGN fix in isolation, the test fails to compile. This suggests:

1. Either the thread context was from a point where both fixes were applied together, OR
2. There's some dependency or state issue that's not being captured

The SGN fix itself (sign-extending integers for printing) is verified to work correctly and causes no regressions (112 tests still pass). The FOR loop issue is a separate, pre-existing (or newly introduced) bug that needs dedicated attention.