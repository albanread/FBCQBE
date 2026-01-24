# FasterBASIC QBE Type System

## Overview

The FasterBASIC QBE backend uses a simplified type system that maps BASIC types to QBE intermediate language types. The key principle is that **DOUBLE is the default numeric type**, matching Lua's behavior where all numbers are represented as doubles unless explicitly specified as integers.

## Type Mapping

### BASIC to QBE Type Mapping

| BASIC Type | Suffix | QBE Type | Description |
|------------|--------|----------|-------------|
| INTEGER    | `%`    | `w`      | 32-bit signed integer |
| FLOAT      | `!`    | `d`      | 64-bit double (QBE has no single-precision float) |
| DOUBLE     | `#`    | `d`      | 64-bit double precision |
| STRING     | `$`    | `l`      | 64-bit pointer to string data |
| (default)  | none   | `d`      | **Default to DOUBLE** |

### Key Design Decisions

1. **QBE has no single-precision float (`s`) support** in practice, so both FLOAT and DOUBLE map to `d`
2. **Default numeric type is DOUBLE**, not INTEGER
3. Only use INTEGER when:
   - Explicitly marked with `%` suffix
   - Required by operation (MOD, AND, OR, XOR, NOT)
   - Array indices
   - FOR loop counters
   - Bitwise operations

## Type Inference Rules

### Literals

```basic
10        ' → DOUBLE (d_10.0)
10%       ' → INTEGER (w)
10.5      ' → DOUBLE (d_10.5)
10.5!     ' → DOUBLE (d_10.5) 
10.5#     ' → DOUBLE (d_10.5)
"hello"   ' → STRING (l)
```

**Note:** Numeric literals without suffix default to DOUBLE, even if they have no decimal point.

### Variables

```basic
x         ' → DOUBLE (default)
x%        ' → INTEGER (explicit)
x!        ' → DOUBLE (maps to d)
x#        ' → DOUBLE (explicit)
x$        ' → STRING
```

### Expressions

Type inference follows these rules:

1. **Binary operations** (arithmetic: +, -, *, /):
   - If either operand is DOUBLE → result is DOUBLE
   - If both operands are INT → result is INT
   - Default: DOUBLE

2. **Comparison operations** (<, <=, >, >=, =, <>):
   - Operands promoted to common type (INT or DOUBLE)
   - Result is always INTEGER (0 or 1)

3. **Bitwise operations** (AND, OR, XOR, NOT, MOD):
   - **Always require INTEGER operands**
   - Operands converted to INT if needed
   - Result is always INTEGER

4. **Type promotion**:
   - INT → DOUBLE (automatic, via `extsw`)
   - DOUBLE → INT (explicit, via `dtosi`, truncates)

## Special Cases

### FOR Loop Variables

FOR loop counters are **always integers**, regardless of suffix or type annotation:

```basic
FOR i = 1 TO 10        ' i is INTEGER (w)
FOR x# = 1 TO 10       ' x# is treated as INTEGER in loop context
```

This matches classic BASIC semantics where loop counters are integral.

### Array Indices

Array indices **must be integers**. Non-integer expressions are automatically converted:

```basic
DIM arr(10)
x = 5.7
arr(x)              ' x converted to INT (5) before array access
arr(i% + 1)         ' Already INT, no conversion needed
```

### Operations Requiring Integers

The following operations require integer operands and will convert as needed:

- `MOD` - modulo (remainder)
- `AND` - bitwise AND
- `OR` - bitwise OR
- `XOR` - bitwise XOR
- `NOT` - bitwise NOT

Example:
```basic
x = 10.5 MOD 3      ' 10.5 → 10 (INT), then 10 MOD 3 = 1
y = 5.9 AND 3.1     ' 5.9 → 5, 3.1 → 3, then 5 AND 3 = 1
```

## QBE Instruction Suffixes

QBE instructions are typed and require correct suffixes:

### Arithmetic Operations

```qbe
%result =w add %a, %b        # Integer add
%result =d add %a, %b        # Double add
%result =w sub %a, %b        # Integer subtract
%result =d sub %a, %b        # Double subtract
%result =w mul %a, %b        # Integer multiply
%result =d mul %a, %b        # Double multiply
%result =w div %a, %b        # Integer divide
%result =d div %a, %b        # Double divide
%result =w rem %a, %b        # Integer remainder (MOD)
```

### Comparison Operations

Comparisons always return integer (w) results:

```qbe
%result =w ceqw %a, %b       # Integer equality
%result =w ceqd %a, %b       # Double equality
%result =w cnew %a, %b       # Integer not-equal
%result =w cned %a, %b       # Double not-equal
%result =w csltw %a, %b      # Integer signed less-than
%result =w csltd %a, %b      # Double less-than
%result =w cslew %a, %b      # Integer signed less-equal
%result =w csled %a, %b      # Double less-equal
%result =w csgtw %a, %b      # Integer signed greater-than
%result =w csgtd %a, %b      # Double greater-than
%result =w csgew %a, %b      # Integer signed greater-equal
%result =w csged %a, %b      # Double greater-equal
```

### Type Conversions

```qbe
%double =d extsw %int        # Convert int (w) to double (d)
%int =w dtosi %double        # Convert double (d) to int (w) - truncates
```

### Copy Operations

```qbe
%result =w copy 42           # Copy integer literal
%result =d copy d_42.5       # Copy double literal
%result =l copy %ptr         # Copy pointer
```

## Implementation Notes

### Type Inference Function

`inferExpressionType()` determines the type of an expression:

- Number literals → DOUBLE (changed from detecting integer-valued doubles)
- String literals → STRING
- Variables → Based on suffix (default DOUBLE)
- Binary expressions → Promote to common type (prefer DOUBLE)

### Type Promotion Function

`promoteToType(value, fromType, toType)` emits conversion code:

```cpp
// INT to DOUBLE
if (fromType == VariableType::INT && toType == VariableType::DOUBLE) {
    return emitIntToDouble(value);  // %result =d extsw %value
}

// DOUBLE to INT
if (fromType == VariableType::DOUBLE && toType == VariableType::INT) {
    return emitDoubleToInt(value);  // %result =w dtosi %value
}
```

### Binary Operation Type Resolution

```cpp
// Determine operation type
VariableType opType;
if (requiresInteger) {
    opType = VariableType::INT;
} else if (leftType == DOUBLE || rightType == DOUBLE) {
    opType = VariableType::DOUBLE;  // Promote to DOUBLE
} else if (leftType == INT && rightType == INT) {
    opType = VariableType::INT;
} else {
    opType = VariableType::DOUBLE;  // Default
}

// Promote operands to operation type
leftTemp = promoteToType(leftTemp, leftType, opType);
rightTemp = promoteToType(rightTemp, rightType, opType);
```

## Examples

### Mixed-Type Arithmetic

```basic
x% = 10                 ' INTEGER
y = 20                  ' DOUBLE (default)
z = x% + y              ' x% → DOUBLE, then add → DOUBLE result

PRINT z                 ' 30.0
```

Generated QBE:
```qbe
%x =w copy 10
%y =d copy d_20.0
%temp1 =d extsw %x              # Convert x% to double
%z =d add %temp1, %y
```

### Integer-Only Operations

```basic
a = 10.7
b = 3.2
c = a MOD b             ' Both converted to INT: 10 MOD 3 = 1

PRINT c                 ' 1
```

Generated QBE:
```qbe
%a =d copy d_10.7
%b =d copy d_3.2
%temp_a =w dtosi %a             # 10.7 → 10
%temp_b =w dtosi %b             # 3.2 → 3
%c =w rem %temp_a, %temp_b      # 10 MOD 3 = 1
```

### Comparisons

```basic
x = 10.5
y = 10.5
IF x = y THEN PRINT "Equal"
```

Generated QBE:
```qbe
%x =d copy d_10.5
%y =d copy d_10.5
%cmp =w ceqd %x, %y             # Compare doubles, return int
jnz %cmp, @then, @else
```

## Migration Notes

### Changes from Previous Implementation

1. **Number literals**: Previously detected integer-valued doubles (e.g., `10.0` → INT); now always DOUBLE
2. **Default variable type**: Changed from INT to DOUBLE
3. **Type suffix defaults**: Changed from `%` (INT) to `#` (DOUBLE)
4. **Binary operations**: Now properly typed (was hardcoded to `w`)
5. **Unary operations**: Now properly typed (was hardcoded to `w`)

### Compatibility

This change aligns FasterBASIC with:
- **Lua semantics**: Numbers are doubles by default
- **Modern BASIC dialects**: FreeBASIC, QB64 default to double-precision
- **QBE limitations**: No single-precision float support

Classic BASIC programs should work correctly because:
- Explicit type suffixes (`%`, `!`, `#`, `$`) are honored
- Integer operations (MOD, bitwise) force integer conversion
- FOR loops always use integer counters
- Array indices are always integers

## Testing

Key test cases to verify type system:

1. Mixed int/double arithmetic
2. Integer-only operations (MOD, AND, OR, XOR)
3. Type promotion and conversion
4. Comparison operations with mixed types
5. FOR loops with different variable types
6. Array indexing with float expressions
7. Function calls with mixed parameter types

## Future Enhancements

Potential improvements:

1. **Optimize away unnecessary conversions** when analysis proves types match
2. **Warning system** for implicit conversions that may lose precision
3. **Type annotations** for function parameters and return values
4. **Compile-time constant folding** for type conversions
5. **Support for LONG (64-bit int)** using QBE `l` type (currently mapped to pointer)