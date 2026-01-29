# NOT Operator - Quick Reference

## Overview

The NOT operator performs bitwise NOT (complement) on integers, flipping all bits in a 32-bit value.

**Syntax:** `NOT <expression>`

**Type:** Unary operator, always returns INTEGER

## Basic Usage

```basic
10 LET A% = 5
20 LET B% = NOT A%
30 PRINT B%           ' Prints: -6
```

## Bitwise Operation

NOT flips all 32 bits in the integer:

```basic
NOT 0     = -1        ' 00000000 → 11111111 (all bits set)
NOT -1    = 0         ' 11111111 → 00000000 (all bits clear)
NOT 1     = -2        ' 00000001 → 11111110
NOT 5     = -6        ' 00000101 → 11111010
NOT 255   = -256      ' 0...0011111111 → 1...1100000000
```

## Mathematical Relationship

For any integer `n`:
```
NOT n = -(n + 1)
```

Examples:
- `NOT 0 = -(0 + 1) = -1`
- `NOT 5 = -(5 + 1) = -6`
- `NOT -10 = -(-10 + 1) = 9`

## Type Coercion

NOT automatically coerces arguments to INTEGER:

```basic
10 LET X# = 10.7         ' DOUBLE
20 LET Y% = NOT X#       ' Converts to 10, then performs NOT
30 PRINT Y%              ' Prints: -11
```

## Use Cases

### 1. Bit Manipulation

```basic
10 REM Set all bits
20 LET MASK% = NOT 0     ' All bits = 1
30 PRINT MASK%           ' Prints: -1
```

### 2. Logical Negation (Truth Values)

In BASIC, 0 = FALSE, non-zero = TRUE:

```basic
10 LET FALSE% = 0
20 LET TRUE% = NOT FALSE%    ' -1 (all bits set)
30 IF NOT 0 THEN PRINT "This always executes"
```

### 3. Complement Calculations

```basic
10 REM Find two's complement preparation
20 LET VALUE% = 42
30 LET COMPLEMENT% = NOT VALUE%
40 PRINT COMPLEMENT%         ' Prints: -43
```

### 4. Double NOT (Returns Original)

```basic
10 LET X% = 42
20 LET Y% = NOT (NOT X%)
30 PRINT Y%                  ' Prints: 42
```

## Usage in Expressions

NOT can be used anywhere an expression is valid:

```basic
10 LET RESULT% = (NOT 10) + 5    ' -11 + 5 = -6
20 IF NOT FLAG% THEN PRINT "Flag is false"
30 PRINT NOT (X% AND Y%)         ' De Morgan's law
```

## Operator Precedence

NOT has high precedence (binds tightly):

```basic
NOT 5 + 3    → (NOT 5) + 3 = -6 + 3 = -3
NOT (5 + 3)  → NOT 8 = -9
```

Order (highest to lowest):
1. Parentheses `()`
2. Unary operators: `NOT`, `-`, `+`
3. Exponentiation: `^`
4. Multiplication/Division: `*`, `/`, `\`, `MOD`
5. Addition/Subtraction: `+`, `-`
6. Comparisons: `=`, `<>`, `<`, `>`, `<=`, `>=`
7. Logical operators: `AND`, `OR`, `XOR`

## Common Patterns

### Toggle All Bits
```basic
10 LET BITS% = 170           ' 10101010
20 LET FLIPPED% = NOT BITS%  ' 01010101 (as 32-bit: -171)
```

### Check if Value is -1
```basic
10 IF VALUE% = NOT 0 THEN PRINT "Value is -1 (all bits set)"
```

### Bitwise De Morgan's Laws
```basic
10 REM NOT (A AND B) = (NOT A) OR (NOT B)
20 REM NOT (A OR B) = (NOT A) AND (NOT B)
```

## Common Mistakes

### ❌ Wrong: Treating NOT as Boolean Negation
```basic
10 LET FLAG% = 1
20 IF NOT FLAG% = 0 THEN ...   ' This checks if -2 = 0 (FALSE!)
```

### ✓ Correct: Use Comparison for Boolean Logic
```basic
10 LET FLAG% = 1
20 IF FLAG% = 0 THEN ...       ' Check if FLAG is false
```

### ❌ Wrong: Expecting Single-Bit Flip
```basic
10 LET X% = 8           ' 00001000
20 LET Y% = NOT X%      ' NOT all bits, not just bit 3!
30 PRINT Y%             ' -9, not 0!
```

## Implementation Details

### QBE IL Generation

For `LET B% = NOT A%`:

```qbe
%t1 =w copy %var_A_INT      # Truncate to 32-bit
%t2 =w xor %t1, -1          # XOR with all-bits-set
%t3 =l extsw %t2            # Sign-extend to 64-bit
%var_B_INT =l copy %t3      # Store result
```

### Type System

- **Argument type:** Any (coerced to INTEGER)
- **Operation type:** 32-bit integer (`w` in QBE)
- **Return type:** INTEGER (semantic), 32-bit `w` (QBE concrete)
- **Storage type:** 64-bit `l` when assigned to INT variable

## Differences from Other BASICs

### QB64/QuickBASIC
- Similar behavior
- NOT returns INTEGER type

### Visual Basic
- NOT is bitwise on integers
- Has separate logical NOT for booleans

### FreeBASIC
- NOT is bitwise
- Can operate on different integer sizes (8, 16, 32, 64-bit)
- FasterBASIC uses 32-bit for consistency

## Related Operators

- `AND` - Bitwise AND
- `OR` - Bitwise OR
- `XOR` - Bitwise XOR (exclusive OR)
- `EQV` - Bitwise equivalence
- `IMP` - Bitwise implication

## Examples from Test Suite

See `tests/arithmetic/test_not_basic.bas` for comprehensive examples:

```basic
10 REM Test: NOT Operator (Bitwise NOT)
20 PRINT "NOT 0 = "; NOT 0       ' -1
30 PRINT "NOT -1 = "; NOT -1     ' 0
40 PRINT "NOT 5 = "; NOT 5       ' -6
50 PRINT "NOT -10 = "; NOT -10   ' 9
60 PRINT "NOT 255 = "; NOT 255   ' -256
```

## Technical Notes

1. **32-bit Operation:** NOT operates on 32-bit integers for efficiency and consistency
2. **Sign Extension:** Results are sign-extended when stored in 64-bit INT variables
3. **Two's Complement:** Uses standard two's complement arithmetic
4. **QBE Backend:** Implemented as `xor` with -1 (0xFFFFFFFF)

## See Also

- [Math Operators Status](../MATH_OPERATORS_STATUS.md)
- [NOT Implementation Details](../NOT_OPERATOR_IMPLEMENTATION.md)
- [Type System Documentation](../fsh/FasterBASICT/docs/TYPE_SYSTEM.md)