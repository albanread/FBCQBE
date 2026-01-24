# FasterBASIC QBE Type System - Implementation Summary

## Overview

Successfully implemented a **DOUBLE-default type system** for the FasterBASIC QBE backend, matching classic BASIC philosophy: *"To make it go fast, make it an integer"* — otherwise, numbers are doubles.

## Key Principle

**All numeric values default to DOUBLE (64-bit float) unless explicitly marked with `%` suffix for INTEGER.**

This aligns with:
- Classic BASIC behavior (numbers default to SINGLE/DOUBLE, not INTEGER)
- Lua semantics (all numbers are doubles)
- Modern computing reality (floating point is fast)
- QBE's type system (only `w` and `d` for numbers, no single-precision float)

---

## Implementation Details

### 1. Type Mappings

| BASIC Type | Suffix | QBE Type | Size | Notes |
|------------|--------|----------|------|-------|
| INTEGER    | `%`    | `w`      | 32-bit | Explicit opt-in for performance |
| FLOAT      | `!`    | `d`      | 64-bit | Maps to DOUBLE (QBE has no `s`) |
| DOUBLE     | `#`    | `d`      | 64-bit | Default numeric type |
| STRING     | `$`    | `l`      | 64-bit | Pointer to string data |
| (none)     | -      | `d`      | 64-bit | **Defaults to DOUBLE** |

### 2. Type Coercion - The Three Points

Type coercion happens at exactly **three places**:

#### Point 1: Expression Evaluation (Mixed Types)
**Location**: `emitBinaryOp()` in `qbe_codegen_expressions.cpp`

```basic
a% = 10        ' INTEGER
b = 2.5        ' DOUBLE (default)
c = a% + b     ' → a% promoted to DOUBLE, result is DOUBLE
```

**Rules**:
- `INT + INT` → `INT`
- `INT + DOUBLE` → `DOUBLE` (INT promoted via `extsw`)
- `DOUBLE + DOUBLE` → `DOUBLE`
- MOD, AND, OR, XOR, NOT → Force operands to INT
- Comparisons → Operate on promoted types, return INT (0 or 1)

#### Point 2: Function Parameters (Call Site)
**Location**: `emitFunctionCall()` in `qbe_codegen_expressions.cpp`

```basic
FUNCTION Square(x AS DOUBLE) AS DOUBLE
    Square = x * x
END FUNCTION

a% = 5                ' INTEGER
result = Square(a%)   ' → a% promoted to DOUBLE before call
```

**Implementation**: Looks up function signature, coerces each argument to match parameter type.

#### Point 3: Variable Assignment (Storage)
**Location**: `emitLet()` in `qbe_codegen_statements.cpp`

```basic
x = 10 / 3      ' x is DOUBLE → 3.333...
y% = 10 / 3     ' y% is INTEGER → 3 (truncates)
z% = 5.7        ' z% is INTEGER → 5 (truncates)
```

**Implementation**: Infers expression type, converts to match variable type if needed.

### 3. Type Conversions

#### INT → DOUBLE (Widening)
**QBE**: `extsw` (extend signed word to double)
```qbe
%int_val =w copy 42
%double_val =d extsw %int_val    # 42 → 42.0
```

#### DOUBLE → INT (Narrowing)
**QBE**: `dtosi` (double to signed integer)
```qbe
%double_val =d copy d_3.14
%int_val =w dtosi %double_val    # 3.14 → 3 (truncates towards zero)
```

### 4. Special Cases

#### FOR Loop Counters
Always INTEGER, regardless of type:
```basic
FOR i = 1 TO 10        ' i is INTEGER
FOR x = 1.5 TO 10.5    ' x treated as INTEGER (1 TO 10)
```

**Rationale**: Classic BASIC semantics; loop counters are integral.

#### Array Indices
Must be INTEGER; non-integer expressions converted:
```basic
DIM arr(10)
x = 5.7
arr(x)              ' x converted to INT (5)
```

#### Integer-Only Operations
MOD, AND, OR, XOR, NOT require integer operands:
```basic
x = 12.9 MOD 5.7    ' 12.9→12, 5.7→5, result: 12 MOD 5 = 2
y = 10 AND 7        ' Bitwise AND on integers
```

#### Comparison Results
Always return INTEGER (0 or 1):
```basic
IF 3.14 > 3.0 THEN  ' Compares as DOUBLEs, returns INT
```

---

## Files Modified

### Core Type System
1. **`qbe_codegen_helpers.cpp`**
   - `getTypeSuffix()`: Default to `'#'` (DOUBLE) instead of `'%'` (INT)
   - `getQBETypeFromSuffix()`: Default returns `"d"`
   - `getVariableType()`: Default returns `VariableType::DOUBLE`
   - `inferExpressionType()`: Defaults to DOUBLE, checks operators (MOD/AND/OR/XOR → INT)

2. **`fasterbasic_semantic.cpp`**
   - `inferTypeFromName()`: Default `VariableType::DOUBLE`
   - Function parameter defaults: Changed from `FLOAT` to `DOUBLE`

3. **`qbe_codegen_expressions.cpp`**
   - `emitNumberLiteral()`: Always emits DOUBLE (removed integer detection)
   - `emitBinaryOp()`: Proper type handling with promotion
   - `emitUnaryOp()`: Typed operations (was hardcoded to `w`)
   - `emitFunctionCall()`: Parameter type coercion implemented
   - `emitArrayAccessExpr()`: Index conversion to INT

4. **`qbe_codegen_statements.cpp`**
   - `emitLet()`: Fixed to use `promoteToType()` instead of blind `swtof`

5. **`qbe_codegen_runtime.cpp`**
   - `emitIntToDouble()`: Fixed to use `extsw` instead of `swtof`
   - `emitDoubleToInt()`: Uses `dtosi` (truncates)

6. **`qbe_codegen_main.cpp`**
   - Variable initialization: Added FLOAT case (maps to `d`)

---

## Examples

### Example 1: Default DOUBLE Behavior
```basic
x = 10          ' x is DOUBLE (10.0)
y = 20          ' y is DOUBLE (20.0)
z = x + y       ' z is DOUBLE (30.0)
```

**Generated QBE**:
```qbe
%var_x =d copy d_0.0
%var_y =d copy d_0.0
%var_z =d copy d_0.0

%t0 =d copy d_10.000000
%var_x =d copy %t0
%t1 =d copy d_20.000000
%var_y =d copy %t1
%t2 =d add %var_x, %var_y
%var_z =d copy %t2
```

### Example 2: Explicit INTEGER
```basic
a% = 5          ' a% is INTEGER (explicit %)
b% = 3          ' b% is INTEGER
c% = a% + b%    ' c% is INTEGER
```

**Generated QBE**:
```qbe
%var_a_INT =w copy 0
%var_b_INT =w copy 0
%var_c_INT =w copy 0

%t0 =d copy d_5.000000
%t1 =w dtosi %t0               # Convert literal to INT
%var_a_INT =w copy %t1
%t2 =d copy d_3.000000
%t3 =w dtosi %t2
%var_b_INT =w copy %t3
%t4 =w add %var_a_INT, %var_b_INT
%var_c_INT =w copy %t4
```

### Example 3: Mixed Types with Coercion
```basic
i% = 4          ' INTEGER
d = 2.5         ' DOUBLE (default)
result = i% + d ' result is DOUBLE
```

**Generated QBE**:
```qbe
%var_i_INT =w copy 0
%var_d =d copy d_0.0
%var_result =d copy d_0.0

%t0 =d copy d_4.000000
%t1 =w dtosi %t0
%var_i_INT =w copy %t1
%t2 =d copy d_2.500000
%var_d =d copy %t2
%t3 =d extsw %var_i_INT        # Promote INT to DOUBLE
%t4 =d add %t3, %var_d         # DOUBLE + DOUBLE
%var_result =d copy %t4
```

### Example 4: MOD with Doubles
```basic
m = 10.7        ' DOUBLE
n = 3.2         ' DOUBLE
r = m MOD n     ' r is DOUBLE (MOD result converted)
```

**Generated QBE**:
```qbe
%var_m =d copy d_0.0
%var_n =d copy d_0.0
%var_r =d copy d_0.0

%t0 =d copy d_10.700000
%var_m =d copy %t0
%t1 =d copy d_3.200000
%var_n =d copy %t1
%t2 =w dtosi %var_m            # Convert to INT (10)
%t3 =w dtosi %var_n            # Convert to INT (3)
%t4 =w rem %t2, %t3            # 10 MOD 3 = 1
%t5 =d extsw %t4               # Convert result back to DOUBLE
%var_r =d copy %t5             # Store as DOUBLE
```

---

## Testing

### Test Programs Created
1. **`test_types.bas`** - Comprehensive type system tests
2. **`test_types_simple.bas`** - Basic type coercion tests
3. **`test_function_coercion.bas`** - Function parameter coercion

### Verification
All generated QBE IL shows:
- ✅ Variables default to DOUBLE (`d`)
- ✅ Explicit `%` suffix creates INTEGER (`w`)
- ✅ Proper type conversions (`extsw`, `dtosi`)
- ✅ Correct operation type suffixes (`add`, `mul`, `div` with `w` or `d`)
- ✅ MOD/bitwise ops force INT conversion
- ✅ Comparisons use correct typed instructions (`ceqd`, `csltw`)

---

## Documentation

### Created Files
1. **`TYPE_SYSTEM.md`** - Detailed type system specification
2. **`COERCION_STRATEGY.md`** - Three-point coercion strategy
3. **`TYPE_SYSTEM_SUMMARY.md`** - This file (implementation summary)

### Key Sections
- Type mapping tables
- Coercion rules and examples
- QBE instruction reference
- Special case handling
- Testing checklist

---

## Benefits

### 1. Simplicity
- One default numeric type (DOUBLE) matches programmer intuition
- Explicit `%` only when needed for integer semantics

### 2. Correctness
- All three coercion points implemented consistently
- No silent precision loss (DOUBLE → INT requires explicit `%` variable)
- Type-safe function calls

### 3. Compatibility
- Matches classic BASIC convention
- Aligns with Lua's numeric model
- Works within QBE's type system limitations

### 4. Performance
- Modern CPUs handle DOUBLE efficiently
- Conversions are cheap (1-3 cycles)
- Programmers optimize only where needed (explicit `%`)

---

## Future Enhancements

1. **Constant folding**: Evaluate type conversions at compile time
2. **Optimization**: Eliminate redundant conversions
3. **Warnings**: Flag lossy conversions (DOUBLE → INT)
4. **Flow-sensitive inference**: Track value ranges to infer types
5. **LONG type**: Use QBE `l` (64-bit int) for large integers

---

## Conclusion

The type system is now **complete and consistent**:
- ✅ DOUBLE is the default numeric type
- ✅ Three-point coercion strategy implemented
- ✅ Special cases handled (FOR, arrays, bitwise, comparisons)
- ✅ Generated QBE IL is type-correct
- ✅ Comprehensive documentation

**Philosophy validated**: *"To make it go fast, make it an integer"* — otherwise, DOUBLE is fast enough!