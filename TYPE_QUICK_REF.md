# FasterBASIC QBE Type System - Quick Reference

## Default Type Rule

**Numbers default to DOUBLE unless explicitly marked with `%` for INTEGER**

```basic
x = 10          ' DOUBLE (10.0)
x% = 10         ' INTEGER (10)
```

---

## Type Suffixes

| Suffix | Type    | QBE | Size   | Example      |
|--------|---------|-----|--------|--------------|
| `%`    | INTEGER | `w` | 32-bit | `count% = 5` |
| `!`    | FLOAT   | `d` | 64-bit | `temp! = 98.6` (maps to DOUBLE) |
| `#`    | DOUBLE  | `d` | 64-bit | `pi# = 3.14159` |
| `$`    | STRING  | `l` | 64-bit | `name$ = "Bob"` |
| (none) | DOUBLE  | `d` | 64-bit | `value = 42` (defaults to DOUBLE) |

**Note**: QBE has no single-precision float, so `!` and `#` both map to 64-bit double (`d`)

---

## Type Conversions

### Widening (No Loss)
```basic
a% = 10         ' INTEGER
b = a%          ' → 10.0 (DOUBLE)
```
**QBE**: `extsw` (extend signed word to double)

### Narrowing (Truncates!)
```basic
x = 3.9         ' DOUBLE
y% = x          ' → 3 (INTEGER, truncates towards zero)
```
**QBE**: `dtosi` (double to signed integer)

---

## Coercion Points

### 1. Expression Evaluation
```basic
a% = 5          ' INTEGER
b = 2.5         ' DOUBLE
c = a% + b      ' → 5.0 + 2.5 = 7.5 (DOUBLE)
```
**Rule**: Promote to wider type (INT → DOUBLE)

### 2. Function Calls
```basic
FUNCTION Square(x AS DOUBLE) AS DOUBLE
    Square = x * x
END FUNCTION

n% = 4
result = Square(n%)     ' → n% promoted to DOUBLE
```
**Rule**: Coerce arguments to match parameter types

### 3. Variable Assignment
```basic
x = 10 / 3      ' → 3.333... (DOUBLE)
y% = 10 / 3     ' → 3 (DOUBLE → INT, truncates)
```
**Rule**: Coerce value to match variable type

---

## Arithmetic Operations

| Operands      | Result | Example         | QBE Instruction |
|---------------|--------|-----------------|-----------------|
| INT + INT     | INT    | `5% + 3%`       | `add` (w)       |
| INT + DOUBLE  | DOUBLE | `5% + 3.0`      | `add` (d)       |
| DOUBLE + DOUBLE | DOUBLE | `5.0 + 3.0`   | `add` (d)       |

**Division** always returns DOUBLE (unless both operands are explicit INT):
```basic
x = 10 / 3          ' → 3.333... (DOUBLE)
y% = 10% / 3%       ' → 3 (INTEGER division)
```

---

## Integer-Only Operations

These operations **require** and **return** INTEGER:

| Operation | Operator | Example       | Behavior            |
|-----------|----------|---------------|---------------------|
| Modulo    | `MOD`    | `10.7 MOD 3.2` | Converts: 10 MOD 3 = 1 |
| Bitwise AND | `AND`  | `12 AND 5`    | `12 AND 5 = 4`     |
| Bitwise OR  | `OR`   | `12 OR 5`     | `12 OR 5 = 13`     |
| Bitwise XOR | `XOR`  | `12 XOR 5`    | `12 XOR 5 = 9`     |
| Bitwise NOT | `NOT`  | `NOT 5`       | `NOT 5 = -6`       |

**Automatic conversion**: Non-integer operands converted to INT before operation

---

## Comparison Operations

Comparisons always return INTEGER (0 = FALSE, 1 = TRUE):

```basic
IF 3.14 > 3.0 THEN      ' Compares as DOUBLEs, returns INT (1)
IF 5% = 5 THEN          ' 5% (INT) promoted to DOUBLE, returns INT (1)
```

| Operator | QBE (INT)  | QBE (DOUBLE) |
|----------|------------|--------------|
| `=`      | `ceqw`     | `ceqd`       |
| `<>`     | `cnew`     | `cned`       |
| `<`      | `csltw`    | `csltd`      |
| `<=`     | `cslew`    | `csled`      |
| `>`      | `csgtw`    | `csgtd`      |
| `>=`     | `csgew`    | `csged`      |

---

## Special Cases

### FOR Loop Counters (Always INTEGER)
```basic
FOR i = 1 TO 10         ' i is INTEGER
FOR x = 1.5 TO 10.5     ' x treated as INTEGER (1 TO 10)
FOR y# = 1 TO 10        ' y# treated as INTEGER in loop
```

### Array Indices (Always INTEGER)
```basic
DIM arr(10)
x = 5.7
arr(x)                  ' x converted to INT (5)
```

### String Variables
```basic
name$ = "Alice"         ' STRING type (always uses $ suffix)
```

---

## Common Patterns

### Pattern 1: Count with INTEGER
```basic
FOR count% = 1 TO 100
    PRINT count%
NEXT count%
```
**Why**: Loop counter, use explicit INT for clarity

### Pattern 2: Calculate with DOUBLE (default)
```basic
total = 0
FOR i% = 1 TO 10
    total = total + i%      ' total is DOUBLE, i% promoted
NEXT i%
average = total / 10        ' DOUBLE result
PRINT "Average: "; average
```
**Why**: Accumulation in DOUBLE avoids precision issues

### Pattern 3: Mixed Math
```basic
price = 19.99               ' DOUBLE (default)
quantity% = 5               ' INTEGER (explicit)
total = price * quantity%   ' quantity% → DOUBLE, result: 99.95
```
**Why**: Natural mixing, automatic promotion

---

## Performance Tips

### When to Use INTEGER (`%`)
✅ **Use INTEGER when**:
- Loop counters (FOR/WHILE)
- Array indices
- Counting discrete items
- Bitwise operations
- Range fits in 32-bit signed (-2³¹ to 2³¹-1)

❌ **Don't use INTEGER for**:
- "Speed optimization" (DOUBLE is fast on modern CPUs!)
- Financial calculations (use DOUBLE for precision)
- Scientific calculations

### Modern Reality
- INT → DOUBLE conversion: ~1-3 cycles
- DOUBLE → INT conversion: ~1-3 cycles
- DOUBLE arithmetic: Same speed as INT on modern CPUs

**Guideline**: Use DOUBLE by default, explicit `%` only when semantics require integers!

---

## Quick Troubleshooting

### Issue: Unexpected truncation
```basic
x = 10 / 3
y% = x              ' → 3 (not 3.333...)
```
**Fix**: Don't assign DOUBLE to INTEGER variable unless truncation is intended

### Issue: Wrong type in function call
```basic
FUNCTION Process(n AS INTEGER)
    ' ...
END FUNCTION

value = 3.14
Process(value)      ' → Truncates to 3
```
**Fix**: Either change parameter type or explicitly convert:
```basic
FUNCTION Process(n AS DOUBLE)   ' Accept DOUBLE
' OR
Process(INT(value))             ' Explicit conversion
```

### Issue: Bitwise operation on floats
```basic
x = 12.9 AND 5.7    ' → 12 AND 5 = 4 (auto-converts)
```
**Fix**: This is correct! Bitwise ops auto-convert to INT. To avoid confusion:
```basic
x% = 12
y% = 5
result% = x% AND y%
```

---

## Philosophy

> **"To make it go fast, make it an integer"**
> 
> Otherwise, numbers are DOUBLE. This is the classic BASIC way.

**Default to DOUBLE** unless:
1. Semantics require integer (loops, indices, bitwise)
2. Explicit optimization needed (profile first!)
3. Interoperating with integer-only APIs

---

## Summary Table

| Context           | Default Type | Override With      |
|-------------------|--------------|-------------------|
| Literal           | DOUBLE       | `%` suffix → INT  |
| Variable          | DOUBLE       | `%` suffix → INT  |
| Expression result | DOUBLE       | Both ops INT → INT |
| FOR counter       | INT          | Always INT        |
| Array index       | INT          | Always INT        |
| MOD/AND/OR/XOR    | INT          | Always INT        |
| Comparison result | INT          | Always INT (0/1)  |

---

**Remember**: The compiler handles conversions automatically at the three coercion points: expressions, function calls, and assignments. Trust the defaults!