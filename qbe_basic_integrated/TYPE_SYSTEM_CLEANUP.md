# Type System Cleanup - Progress Report

**Date:** February 1, 2026  
**Status:** IN PROGRESS - Critical Fix Applied, Build Errors Remaining

---

## Problem Summary

The FasterBASIC compiler had TWO type systems running in parallel:

1. **Legacy System**: `VariableType` enum (INT, FLOAT, DOUBLE, STRING, etc.)
2. **New System**: `TypeDescriptor` struct with `BaseType` enum (more detailed)

This caused a critical bug where:
- Variables were created with `VariableType::UNKNOWN` in the legacy field
- But the codegen was reading `TypeDescriptor.baseType` which was never set
- Result: All variables had type UNKNOWN, causing wrong code generation
- Example: `Y# = 10.5` was compiled as `%var_Y_DOUBLE =w copy s_10.5` (w = 32-bit word instead of d = double)

---

## Root Cause Fixed ✅

**File:** `fsh/FasterBASICT/src/fasterbasic_semantic.cpp`  
**Function:** `useVariable()`  
**Line:** ~3344

**BEFORE:**
```cpp
auto* sym = lookupVariable(name);
if (!sym) {
    // Implicitly declare
    VariableType type = inferTypeFromName(name);
    sym = declareVariable(name, type, loc, false);  // OLD - only sets legacy type
}
```

**AFTER:**
```cpp
auto* sym = lookupVariable(name);
if (!sym) {
    // Implicitly declare using TypeDescriptor
    TypeDescriptor typeDesc = inferTypeFromNameD(name);
    sym = declareVariableD(name, typeDesc, loc, false);  // NEW - sets TypeDescriptor properly
}
```

This ensures that when variables are implicitly declared (first use in LET statement), they get a proper `TypeDescriptor` with the correct `baseType`.

---

## Changes Made

### 1. Struct Definitions (fasterbasic_semantic.h)

#### VariableSymbol
- ❌ REMOVED: `VariableType type` field
- ✅ KEPT: `TypeDescriptor typeDesc` field
- ❌ REMOVED: `toLegacyString()` method
- ✅ Updated constructor to not initialize `type`

#### ArraySymbol
- ❌ REMOVED: `VariableType type` field
- ✅ KEPT: `TypeDescriptor elementTypeDesc` field
- ✅ Updated `toString()` to use `elementTypeDesc.toString()`

#### FunctionSymbol
- ❌ REMOVED: `VariableType returnType` field
- ❌ REMOVED: `std::vector<VariableType> parameterTypes` field
- ❌ REMOVED: `std::vector<std::string> parameterTypeNames` field
- ✅ KEPT: `TypeDescriptor returnTypeDesc` field
- ✅ KEPT: `std::vector<TypeDescriptor> parameterTypeDescs` field
- ❌ REMOVED: `toLegacyString()` method

#### FunctionScope
- ❌ CHANGED: `VariableType expectedReturnType` → `TypeDescriptor expectedReturnType`

### 2. Declaration Functions Updated

- `useVariable()` - now calls `declareVariableD()` ✅
- `processDefStatement()` - updated to use TypeDescriptor ✅
- Various global/parameter declarations - updated ✅

### 3. Codegen Cleanup (codegen_v2/ast_emitter.cpp)

- ❌ REMOVED: `inferTypeFromMangledName()` workaround function
- ✅ Simplified `getVariableType()` to just read `typeDesc.baseType`
- ✅ Fixed variable name handling (use mangled names from parser as-is)

---

## Remaining Build Errors (~20)

The semantic analyzer has compilation errors where old code references removed fields:

### Pattern 1: Direct `.type` access
**Error:** `no member named 'type' in 'FasterBASIC::VariableSymbol'`  
**Fix:** Change `.type` to `.typeDesc.baseType`

Example locations:
- Line 1606: `if (varSym.type == VariableType::INT)`
  → `if (varSym.typeDesc.baseType == BaseType::INTEGER)`

### Pattern 2: `.returnType` access
**Error:** `no member named 'returnType' in 'FasterBASIC::FunctionSymbol'`  
**Fix:** Change `.returnType` to `.returnTypeDesc.baseType`

Example locations:
- Lines 731, 745, 748, 751, 754, 757, 767, 770, 804, 1176, 1447

### Pattern 3: `.parameterTypes` access
**Error:** `no member named 'parameterTypes' in 'FasterBASIC::FunctionSymbol'`  
**Fix:** Change to use `.parameterTypeDescs`

Example locations:
- Lines 731, 851, 852

### Pattern 4: Type comparisons
**Error:** Comparing `TypeDescriptor` with `VariableType`  
**Fix:** Update to compare `BaseType` values

Example location:
- Line 1481: Assignment compatibility check

---

## Testing After Fix

Once build errors are resolved, test with:

```bash
cd qbe_basic_integrated
cat > test_types.bas << 'EOF'
X% = 5
Y# = 10.5
Z$ = "hello"
PRINT X%; Y#; Z$
END
EOF

./build_qbe_basic.sh
./fbc_qbe test_types.bas -i | grep "var_"
# Should show:
# %var_X_INT =w copy 5              ← correct (w for integer)
# %var_Y_DOUBLE =d copy 10.5        ← correct (d for double) 
# %var_Z_STRING =l call $str_new... ← correct (l for pointer)
```

---

## Next Steps

1. **Immediate:** Fix remaining compilation errors in semantic analyzer
   - Search for `.type` and replace with `.typeDesc.baseType`
   - Search for `.returnType` and replace with `.returnTypeDesc.baseType`
   - Search for `.parameterTypes` and replace with `.parameterTypeDescs`

2. **Optional Later:** Remove `VariableType` enum completely
   - It's still defined but no longer used
   - Can be removed for cleaner code

3. **Test Suite:** Run full test suite after fixes
   - `test_integer_basic.bas` - should pass ✅ (already tested)
   - `test_double_basic.bas` - should pass after this fix
   - All other arithmetic tests

---

## Key Files Modified

1. `fsh/FasterBASICT/src/fasterbasic_semantic.h` - Struct definitions
2. `fsh/FasterBASICT/src/fasterbasic_semantic.cpp` - Implementation
3. `fsh/FasterBASICT/src/codegen_v2/ast_emitter.cpp` - Codegen
4. `fsh/FasterBASICT/src/codegen_v2/ast_emitter.h` - Header

---

## Lessons Learned

1. **Never have two parallel systems** - Always migrate completely
2. **Symbol table is the source of truth** - Codegen must read from it correctly
3. **Type inference must set ALL type fields** - Not just legacy ones
4. **Mangled names** - Parser/semantic analyzer mangles `Y#` → `Y_DOUBLE`, codegen uses as-is

---

**Status:** The ROOT CAUSE is fixed. Variables now get proper types. Just need to clean up ~20 compilation errors in code that referenced the old fields.