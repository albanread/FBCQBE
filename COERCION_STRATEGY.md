# FasterBASIC Type Coercion Strategy

## Philosophy

Following classic BASIC convention: **"To make it go fast, make it an integer"**

By default, all numeric values are **DOUBLE** (64-bit floating point). Programmers explicitly use the `%` suffix when they need integer performance or integer semantics.

This matches:
- Classic BASIC behavior (numbers default to SINGLE/DOUBLE, not INTEGER)
- Lua semantics (all numbers are doubles unless context requires integers)
- Modern computing reality (floating point is fast, integer optimization is rarely necessary)

---

## The Three Coercion Points

Type coercion happens at exactly **three places** in the compiler:

### 1. Expression Evaluation (Mixed Types)

**Location**: `emitBinaryOp()` in `qbe_codegen_expressions.cpp`

**Rule**: When evaluating expressions with mixed types, coerce to the "wider" type.

```basic
a% = 10        ' INTEGER
b = 2.5        ' DOUBLE (default)
c = a% + b     ' → a% promoted to DOUBLE, result is DOUBLE
```

**Type Promotion Hierarchy**:
- `INT + INT` → `INT`
- `INT + DOUBLE` → `DOUBLE` (INT promoted)
- `DOUBLE + DOUBLE` → `DOUBLE`

**Special Cases**:
- **Integer-only operations** (MOD, AND, OR, XOR, NOT) force both operands to INT
- **Comparisons** operate on promoted types but always return INT (0 or 1)

**Implementation**:
```cpp
// Determine operation type
VariableType opType;
if (requiresInteger) {
    opType = VariableType::INT;  // MOD, AND, OR, XOR
} else if (leftType == DOUBLE || rightType == DOUBLE) {
    opType = VariableType::DOUBLE;  // Promote to DOUBLE
} else if (leftType == INT && rightType == INT) {
    opType = VariableType::INT;  // Both INT → INT result
} else {
    opType = VariableType::DOUBLE;  // Default
}

// Promote operands
leftTemp = promoteToType(leftTemp, leftType, opType);
rightTemp = promoteToType(rightTemp, rightType, opType);
```

---

### 2. Function Parameters (Call Site)

**Location**: `emitFunctionCall()` in `qbe_codegen_expressions.cpp`

**Rule**: When calling functions, coerce arguments to match the function's parameter types.

```basic
SUB PrintSquare(x AS DOUBLE)
    PRINT x * x
END SUB

a% = 5                 ' INTEGER
CALL PrintSquare(a%)   ' → a% promoted to DOUBLE before call
```

**Implementation Status**:
- ✅ User-defined functions: Need to look up function signature and coerce arguments
- ✅ Built-in functions: Runtime library handles conversions (basic_print_int vs basic_print_double)
- 🚧 DEF FN: Type inference needs completion

**TODO**: Complete function parameter type checking and coercion
```cpp
// For each argument
for (size_t i = 0; i < args.size(); i++) {
    std::string argTemp = emitExpression(args[i].get());
    VariableType argType = inferExpressionType(args[i].get());
    VariableType paramType = funcSignature.parameterTypes[i];
    
    if (argType != paramType) {
        argTemp = promoteToType(argTemp, argType, paramType);
    }
    
    // Emit argument with correct type
}
```

---

### 3. Variable Assignment (Storage)

**Location**: `emitLet()` in `qbe_codegen_statements.cpp`

**Rule**: When storing to a variable, coerce the value to match the variable's declared type.

```basic
x = 10 / 3      ' x is DOUBLE (default), result is DOUBLE
                ' → 3.333...

y% = 10 / 3     ' y% is INTEGER (explicit %), result is DOUBLE
                ' → DOUBLE converted to INT (truncates to 3)

z% = 5.7        ' z% is INTEGER
                ' → 5.7 (DOUBLE) converted to INT (truncates to 5)
```

**Implementation**:
```cpp
// Infer the type of the expression value
VariableType exprType = inferExpressionType(stmt->value.get());

// Convert value to match variable type if needed
if (varType != exprType) {
    // promoteToType emits the conversion and returns the result temp
    std::string convertedValue = promoteToType(valueTemp, exprType, varType);
    emit("    " + varRef + " =" + qbeType + " copy " + convertedValue + "\n");
} else {
    // Types match, just copy
    emit("    " + varRef + " =" + qbeType + " copy " + valueTemp + "\n");
}
```

---

## Type Conversion Operations

### INT → DOUBLE (Widening, No Loss)

**QBE Instruction**: `extsw` (extend signed word to double)

```qbe
%int_val =w copy 42
%double_val =d extsw %int_val    # 42 → 42.0
```

**Use Cases**:
- Mixed arithmetic: `5% + 2.5`
- Assigning INT to DOUBLE variable
- Passing INT to function expecting DOUBLE

---

### DOUBLE → INT (Narrowing, Truncates)

**QBE Instruction**: `dtosi` (double to signed integer)

```qbe
%double_val =d copy d_3.14
%int_val =w dtosi %double_val    # 3.14 → 3 (truncates)
```

**Use Cases**:
- Assigning DOUBLE to INT variable
- Array indices (must be integers)
- Integer-only operations (MOD, AND, OR, XOR)
- FOR loop counters (always integers)
- Passing DOUBLE to function expecting INT

**Behavior**: **Truncates towards zero** (not rounds!)
- `3.9 → 3`
- `3.1 → 3`
- `-3.9 → -3`
- `-3.1 → -3`

---

## Special Cases

### FOR Loop Counters

FOR loop variables are **always integers**, regardless of type suffix or default:

```basic
FOR i = 1 TO 10        ' i is INTEGER
FOR x = 1.5 TO 10.5    ' x treated as INTEGER in loop (1 TO 10)
FOR y# = 1 TO 10       ' y# treated as INTEGER in loop
```

**Rationale**: Classic BASIC semantics; loop counters are integral by definition.

**Implementation**: Tracked in `m_forLoopVariables` set; `getVariableType()` returns INT for these.

---

### Array Indices

Array indices **must be integers**. Non-integer expressions are automatically converted:

```basic
DIM arr(10)
x = 5.7
arr(x)              ' x (5.7) converted to INT (5) before array access
arr(i% + 1)         ' Already INT, no conversion needed
```

**Implementation**: `emitArrayAccessExpr()` converts each index expression to INT if needed.

---

### Bitwise Operations

Bitwise operations (AND, OR, XOR, NOT) and MOD **require integer operands**:

```basic
x = 12.9 AND 5.7    ' 12.9→12, 5.7→5, then 12 AND 5 = 4
y = 10.7 MOD 3.2    ' 10.7→10, 3.2→3, then 10 MOD 3 = 1
```

**Implementation**: `emitBinaryOp()` detects these operators and forces INT type.

---

### Comparison Operations

Comparisons operate on promoted types but **always return integer (0 or 1)**:

```basic
IF 3.14 > 3.0 THEN   ' Compare as DOUBLEs, result is INT (1 = TRUE)
IF 5% = 5# THEN      ' 5% promoted to DOUBLE, compare, result is INT
```

**QBE Instructions**: Comparisons have type-specific suffixes but always return `w`:
```qbe
%result =w ceqd %a, %b     # Compare DOUBLEs, return INT
%result =w csltw %a, %b    # Compare INTs, return INT
```

---

## Examples

### Example 1: Mixed Arithmetic

```basic
a% = 10         ' INTEGER
b = 2.5         ' DOUBLE (default)
c% = a% * b     ' What happens?
```

**Steps**:
1. **Expression Evaluation**: `a% * b`
   - Left: INT, Right: DOUBLE
   - Promote left to DOUBLE: `a%` → `extsw` → temp DOUBLE
   - Multiply: `mul` (double)
   - Result: DOUBLE
2. **Variable Assignment**: Store to `c%`
   - Target: INT, Source: DOUBLE
   - Convert: `dtosi` (truncates)
   - Result: `c% = 25` (not 25.0!)

**Generated QBE**:
```qbe
%a_INT =w copy 10
%b =d copy d_2.5
%t1 =d extsw %a_INT           # INT → DOUBLE
%t2 =d mul %t1, %b            # DOUBLE * DOUBLE
%t3 =w dtosi %t2              # DOUBLE → INT
%c_INT =w copy %t3
```

---

### Example 2: Function Call with Coercion

```basic
SUB PrintValue(x AS DOUBLE)
    PRINT x
END SUB

n% = 42
CALL PrintValue(n%)
```

**Steps**:
1. Evaluate argument: `n%` → INT temporary
2. Look up function signature: expects DOUBLE
3. Coerce: `extsw` INT → DOUBLE
4. Call with DOUBLE argument

**Generated QBE**:
```qbe
%n_INT =w copy 42
%t1 =d extsw %n_INT           # Convert for parameter
call $PrintValue(d %t1)
```

---

### Example 3: MOD with Doubles

```basic
x = 10.7        ' DOUBLE
y = 3.2         ' DOUBLE
z = x MOD y     ' z is DOUBLE
```

**Steps**:
1. **Expression Evaluation**: `x MOD y`
   - MOD requires INT operands
   - Convert x: `dtosi` → 10
   - Convert y: `dtosi` → 3
   - MOD: `rem` (integer)
   - Result: INT (value 1)
2. **Variable Assignment**: Store to `z`
   - Target: DOUBLE, Source: INT
   - Convert: `extsw`
   - Result: `z = 1.0`

**Generated QBE**:
```qbe
%x =d copy d_10.7
%y =d copy d_3.2
%t1 =w dtosi %x               # DOUBLE → INT (10)
%t2 =w dtosi %y               # DOUBLE → INT (3)
%t3 =w rem %t1, %t2           # 10 MOD 3 = 1
%t4 =d extsw %t3              # INT → DOUBLE
%z =d copy %t4                # Store as DOUBLE
```

---

## Type Inference Rules Summary

| Context | Default Type | Override Conditions |
|---------|--------------|---------------------|
| Literal without suffix | DOUBLE | Explicit `%` suffix → INT |
| Variable without suffix | DOUBLE | Explicit `%` suffix → INT |
| Binary expression | DOUBLE | Both operands INT → INT |
| MOD, AND, OR, XOR, NOT | INT | Always (forces operands to INT) |
| Comparison | INT | Always (result is 0 or 1) |
| FOR loop counter | INT | Always (classic BASIC rule) |
| Array index | INT | Always (converted if needed) |
| Function parameter | (varies) | Matches function signature |

---

## Testing Checklist

Verify coercion works correctly at all three points:

### Expression Evaluation
- [ ] INT + DOUBLE → DOUBLE
- [ ] DOUBLE + DOUBLE → DOUBLE
- [ ] INT + INT → INT
- [ ] MOD with DOUBLEs → converts to INT
- [ ] Comparison with mixed types → INT result

### Function Parameters
- [ ] Pass INT to DOUBLE parameter
- [ ] Pass DOUBLE to INT parameter
- [ ] Multiple parameters with different types
- [ ] DEF FN with typed parameters

### Variable Assignment
- [ ] Assign DOUBLE to INT variable (truncates)
- [ ] Assign INT to DOUBLE variable (widens)
- [ ] Assign expression result to typed variable
- [ ] Array element assignment with type conversion

---

## Performance Notes

### When Conversions Are Free

Modern CPUs handle INT↔DOUBLE conversion efficiently:
- `extsw` (INT→DOUBLE): ~1-3 cycles
- `dtosi` (DOUBLE→INT): ~1-3 cycles

**Guideline**: Don't micro-optimize! Use the natural type and let the compiler handle conversions.

### When to Use INT

Use explicit `%` suffix when:
1. **Semantics matter**: Array indices, loop counters, bitwise operations
2. **Range is small**: Values guaranteed to fit in 32-bit signed integer (-2³¹ to 2³¹-1)
3. **Interoperability**: Calling external functions expecting integers

**Do NOT use INT for "speed"** unless profiling shows it matters!

---

## Future Enhancements

1. **Warning system**: Flag conversions that may lose precision
   ```basic
   x% = 3.9    ' Warning: truncates to 3
   ```

2. **Constant folding**: Evaluate conversions at compile time
   ```basic
   x% = 10.5 + 2.5    ' Could evaluate to 13 at compile time
   ```

3. **Optimization**: Eliminate redundant conversions
   ```basic
   x = 5%        ' Don't convert 5 to DOUBLE, keep as INT until needed
   y = x + 1     ' Convert here when mixed with DOUBLE default
   ```

4. **Type inference**: Flow-sensitive type tracking
   ```basic
   x = 5         ' Infer INT from constant
   y = 3.14      ' Infer DOUBLE from constant
   z = x + y     ' Know x is INT, y is DOUBLE → promote
   ```

---

## Summary

**Three coercion points, three rules:**

1. **Expressions**: Coerce to wider type (INT → DOUBLE)
2. **Function calls**: Coerce to match function signature
3. **Assignment**: Coerce to match variable type

**Default is DOUBLE**, explicit `%` for INTEGER.

**"To make it go fast, make it an integer"** — but in 2024, DOUBLE is usually fast enough!