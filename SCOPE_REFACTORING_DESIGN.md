# Scope Refactoring Design Document

**Status**: 🚧 IN PROGRESS  
**Goal**: Eliminate scope confusion by making all symbols explicitly scoped  
**Priority**: HIGH - Fixes FOR loop in SUB bug and prevents future scope issues

---

## Problem Statement

The current symbol table has inconsistent and confusing scope handling:

1. **Implicit Global Scope**: Empty string `""` means global, which is ambiguous
2. **Mixed Scoping Keys**: Sometimes `"varName"`, sometimes `"function::varName"`
3. **Inconsistent Lookups**: Different parts of compiler use different lookup strategies
4. **No Block Tracking**: Can't track variables within nested blocks (IF/FOR/WHILE)
5. **Scope Leakage**: FOR loop variables bleed across function boundaries

### Example of Current Confusion

```cpp
// Is this global or function-local?
m_symbolTable.variables["i"] = varSymbol;  // Ambiguous!

// Sometimes we use scoped keys
m_symbolTable.variables["myFunc::i"] = varSymbol;

// Lookups are inconsistent
auto it = m_symbolTable.variables.find("i");  // Will this find function-local i?
```

---

## Solution: Explicit Scope Hierarchy

Every symbol will have an explicit `Scope` structure that tracks:
1. **Scope Type**: `GLOBAL` or `FUNCTION`
2. **Scope Name**: Empty for global, function name for functions
3. **Block Number**: Which block within that scope (for future nested block support)

---

## New Data Structures

### Scope Structure

```cpp
struct Scope {
    enum class Type {
        GLOBAL,      // Top-level/main program scope
        FUNCTION     // Inside a SUB or FUNCTION
    };
    
    Type type;
    std::string name;        // Empty for global, function name for function scope
    int blockNumber;         // Block number within this scope (for nested blocks)
    
    // Helper constructors
    static Scope makeGlobal(int block = 0);
    static Scope makeFunction(const std::string& funcName, int block = 0);
    
    // Helper methods
    bool isGlobal() const;
    bool isFunction() const;
    std::string toString() const;  // "global" or "function:funcName"
};
```

### Updated VariableSymbol

```cpp
struct VariableSymbol {
    std::string name;
    TypeDescriptor typeDesc;
    Scope scope;                 // ✨ NEW: Explicit scope tracking
    bool isDeclared;
    bool isUsed;
    bool isGlobal;              // true if declared with GLOBAL statement
    // ... other fields
};
```

### Symbol Table with Scope Keys

```cpp
struct SymbolTable {
    // Keys are now: "global::varName" or "function:funcName::varName"
    std::unordered_map<std::string, VariableSymbol> variables;
    
    // Helper methods
    static std::string makeScopeKey(const std::string& varName, const Scope& scope);
    void insertVariable(const std::string& varName, const VariableSymbol& symbol);
    VariableSymbol* lookupVariable(const std::string& varName, const Scope& scope);
    VariableSymbol* lookupVariableWithFallback(const std::string& varName, const Scope& scope);
};
```

---

## Scope Key Format

All symbol table keys will follow a consistent format:

| Scope Type | Variable Name | Full Key |
|------------|--------------|----------|
| Global | `counter` | `global::counter` |
| Function | `i` in `TestLoop` | `function:TestLoop::i` |
| Function | `result` in `Calculate` | `function:Calculate::result` |

### Benefits
- ✅ No ambiguity - every key is self-documenting
- ✅ Easy to filter by scope (e.g., all variables in a function)
- ✅ Future-proof for nested blocks (can add block number to key)
- ✅ Debug-friendly - keys show exactly where variable lives

---

## Lookup Strategy

### Standard Lookup
```cpp
// Look in current scope only
Scope currentScope = getCurrentScope();
VariableSymbol* var = symbolTable.lookupVariable("i", currentScope);
```

### Lookup with Fallback
```cpp
// Try current scope, then fall back to global
Scope currentScope = getCurrentScope();
VariableSymbol* var = symbolTable.lookupVariableWithFallback("i", currentScope);
```

### Lookup Flow
```
1. Strip type suffix from variable name (i% → i)
2. Determine current scope (global or function)
3. Try lookup in current scope first
4. If not found and in function scope, try global scope
5. If still not found, variable is undefined
```

---

## Implementation Plan

### Phase 1: Core Infrastructure ✅ DONE
- [x] Add `Scope` struct to semantic.h
- [x] Update `VariableSymbol` to include `Scope scope` field
- [x] Add scope helper methods to `SymbolTable`
- [x] Add `getCurrentScope()` to `SemanticAnalyzer`

### Phase 2: Variable Declaration (NEXT)
- [ ] Update `declareVariable()` to use new scope system
- [ ] Update implicit variable creation to use new scope
- [ ] Update FOR loop variable registration to use new scope
- [ ] Migrate all direct symbol table insertions to use `insertVariable()`

### Phase 3: Variable Lookup
- [ ] Update `lookupVariableScoped()` to use new lookup methods
- [ ] Update all variable lookups to use scope-aware methods
- [ ] Remove old scoped key logic (e.g., `"function::var"` manual construction)

### Phase 4: Validation & Testing
- [ ] Test FOR loops in global scope
- [ ] Test FOR loops in function scope
- [ ] Test variable shadowing (local var vs global var with same name)
- [ ] Test SHARED variables in functions
- [ ] Run full test suite and verify 112+ tests still pass

### Phase 5: Array Symbols
- [ ] Apply same scope tracking to `ArraySymbol`
- [ ] Update array declaration and lookup

### Phase 6: Cleanup
- [ ] Remove legacy `functionScope` string field (use `scope.name` instead)
- [ ] Remove manual scoped key construction throughout codebase
- [ ] Update documentation and comments

---

## Migration Strategy

To avoid breaking everything at once, we'll use a gradual migration:

1. **Add new fields** alongside old fields
2. **Populate both** old and new fields during transition
3. **Migrate lookup logic** one component at a time
4. **Remove old fields** once everything is migrated

### Compatibility Layer

```cpp
// VariableSymbol legacy compatibility
struct VariableSymbol {
    Scope scope;              // NEW
    std::string functionScope;  // OLD (deprecated)
    
    // Helper to maintain compatibility
    std::string functionScope() const {
        return scope.isFunction() ? scope.name : "";
    }
};
```

---

## Expected Impact on FOR Loop Bug

The FOR loop in SUB bug happens because:
1. FOR variable is registered with inconsistent scope
2. Variable lookup doesn't find the registered symbol
3. Allocation never happens because lookup fails
4. QBE IL references undefined variable

**After scope refactoring**:
1. FOR variable explicitly registered in `Scope::makeFunction("TestLoop")`
2. Variable lookup uses same scope key: `"function:TestLoop::i"`
3. Lookup succeeds, allocation happens
4. QBE IL references valid allocated variable ✅

---

## Code Examples

### Before (Current - Confusing)

```cpp
// Variable declaration - unclear what scope this is
VariableSymbol varSym;
varSym.name = "i";
varSym.functionScope = m_currentFunctionScope.inFunction 
    ? m_currentFunctionScope.functionName 
    : "";
m_symbolTable.variables["i"] = varSym;  // Or is it "function::i"?

// Lookup - manual key construction
std::string key = functionScope.empty() 
    ? varName 
    : functionScope + "::" + varName;
auto it = m_symbolTable.variables.find(key);
```

### After (New - Crystal Clear)

```cpp
// Variable declaration - explicit scope
Scope currentScope = getCurrentScope();
VariableSymbol varSym("i", TypeDescriptor(BaseType::INTEGER), currentScope, true);
m_symbolTable.insertVariable("i", varSym);  // Key: "function:TestLoop::i"

// Lookup - clean and obvious
Scope currentScope = getCurrentScope();
VariableSymbol* var = m_symbolTable.lookupVariable("i", currentScope);
```

---

## Test Cases to Validate

### Test 1: FOR Loop in Global Scope
```basic
10 FOR i = 1 TO 5
20   PRINT i
30 NEXT i
```
**Expected**: Variable `i` registered in `Scope::makeGlobal()`, key: `"global::i"`

### Test 2: FOR Loop in SUB
```basic
10 SUB TestLoop()
20   FOR i = 1 TO 5
30     PRINT i
40   NEXT i
50 END SUB
```
**Expected**: Variable `i` registered in `Scope::makeFunction("TestLoop")`, key: `"function:TestLoop::i"`

### Test 3: Variable Shadowing
```basic
10 DIM x AS INTEGER
20 x = 10
30 SUB Test()
40   LOCAL x AS INTEGER
50   x = 20
60   PRINT x        ' Should print 20 (local)
70 END SUB
80 CALL Test()
90 PRINT x          ' Should print 10 (global)
```
**Expected**: Two separate symbols:
- `"global::x"` with value 10
- `"function:Test::x"` with value 20

### Test 4: SHARED Variable
```basic
10 DIM counter AS INTEGER
20 counter = 0
30 SUB Increment()
40   SHARED counter
50   counter = counter + 1
60 END SUB
```
**Expected**: Single symbol `"global::counter"`, accessible from both scopes

---

## Future Enhancements

### Block-Level Scoping
With block numbers, we can support block-level scoping:

```basic
SUB Test()
  LOCAL x AS INTEGER
  x = 1
  
  IF x > 0 THEN
    LOCAL y AS INTEGER  ' Block scope
    y = 2
  END IF
  
  ' y is not accessible here
END SUB
```

Each block would have:
- `Scope::makeFunction("Test", 0)` - outer function block
- `Scope::makeFunction("Test", 1)` - IF block

### Scope Stack
For future nested scopes:

```cpp
class SemanticAnalyzer {
    std::vector<Scope> m_scopeStack;  // Stack of nested scopes
    
    void enterScope(Scope::Type type, const std::string& name = "") {
        m_scopeStack.push_back(Scope(type, name, m_scopeStack.size()));
    }
    
    void exitScope() {
        m_scopeStack.pop_back();
    }
    
    Scope getCurrentScope() const {
        return m_scopeStack.empty() ? Scope::makeGlobal() : m_scopeStack.back();
    }
};
```

---

## Risk Mitigation

### Risks
1. **Massive code change** - touches many files
2. **Breaking existing tests** - scope keys change
3. **Subtle bugs** - wrong scope in some code path

### Mitigation Strategies
1. **Gradual migration** - keep old and new code working simultaneously
2. **Comprehensive testing** - test after each phase
3. **Git branches** - work in feature branch, can revert if needed
4. **Logging** - add debug output to see which scope is used where
5. **Compatibility layer** - keep legacy helpers during transition

---

## Success Criteria

✅ All existing tests pass (112+)  
✅ `test_for_in_sub_minimal.bas` compiles and runs  
✅ `test_abs_sgn.bas` compiles and runs  
✅ No scope-related confusion in symbol table  
✅ All variable declarations use explicit scope  
✅ All variable lookups use scope-aware methods  
✅ Documentation updated to reflect new system  

---

## Files to Modify

### Core Infrastructure (Phase 1) ✅
- `fsh/FasterBASICT/src/fasterbasic_semantic.h` - Add Scope struct, update VariableSymbol

### Variable Declaration (Phase 2)
- `fsh/FasterBASICT/src/fasterbasic_semantic.cpp` - Update all variable declarations

### Variable Lookup (Phase 3)
- `fsh/FasterBASICT/src/fasterbasic_semantic.cpp` - Update lookup logic
- `fsh/FasterBASICT/src/codegen_v2/ast_emitter.cpp` - Update variable references
- `fsh/FasterBASICT/src/codegen_v2/symbol_mapper.cpp` - Update name mangling

### Validation (Phase 4)
- Create new test files for scope testing
- Update `test_for_in_sub_minimal.bas` to use new scope system

---

## Related Issues

- **FOR Loop in SUB Bug**: `FOR_LOOP_SUB_ISSUE.md`
- **SGN Print Fix**: `SGN_PRINT_FIX.md` (already completed)
- **Previous SELECT CASE fix**: Used similar scope tracking concepts

---

## Timeline Estimate

- Phase 1 (Infrastructure): ✅ 30 minutes (DONE)
- Phase 2 (Declaration): 1-2 hours
- Phase 3 (Lookup): 2-3 hours  
- Phase 4 (Testing): 1 hour
- Phase 5 (Arrays): 1 hour
- Phase 6 (Cleanup): 30 minutes

**Total**: ~6-8 hours of focused work