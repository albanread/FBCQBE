# Type Coercion Strategy

## Philosophy

**Default: DOUBLE** — Numbers are 64-bit floating point by default. Use `%` suffix for integers when semantics require it (array indices, loop counters, bitwise ops).

Classic BASIC: *"To make it go fast, make it an integer"* — but modern CPUs make this rarely necessary.

---

## Implementation Status

✅ **Fully implemented** in QBE code generator:
- Expression evaluation with mixed types ([qbe_codegen_expressions.cpp](fsh/FasterBASICT/src/codegen/qbe_codegen_expressions.cpp))
- Function parameter coercion ([qbe_codegen_expressions.cpp](fsh/FasterBASICT/src/codegen/qbe_codegen_expressions.cpp))
- Variable assignment conversion ([qbe_codegen_statements.cpp](fsh/FasterBASICT/src/codegen/qbe_codegen_statements.cpp))
- Type promotion helper: `promoteToType()` ([qbe_codegen_helpers.cpp](fsh/FasterBASICT/src/codegen/qbe_codegen_helpers.cpp))

---

## Three Coercion Points

### 1. Expression Evaluation
**Rule**: Promote to wider type (INT + DOUBLE → DOUBLE)

```basic
a% = 10         ' INTEGER
b = 2.5         ' DOUBLE
c = a% + b      ' → DOUBLE result (a% promoted)
```

**Special cases**:
- MOD, AND, OR, XOR: Force both operands to INT
- Comparisons: Return INT (0 or 1)
- INT + INT: Result is INT

### 2. Function Calls
**Rule**: Match parameter types

```basic
SUB PrintSquare(x AS DOUBLE)
a% = 5
CALL PrintSquare(a%)   ' → a% promoted to DOUBLE
```

### 3. Assignment
**Rule**: Convert to variable type

```basic
y% = 10 / 3     ' DOUBLE result → INT (truncates to 3)
z = 5%          ' INT → DOUBLE (widens to 5.0)
```

---

## QBE Instructions

**INT → DOUBLE**: `extsw` (extend signed word)
```qbe
%t =d extsw %int_val    # Widening conversion
```

**DOUBLE → INT**: `dtosi` (double to signed integer)  
```qbe
%t =w dtosi %dbl_val    # Truncates toward zero
```

**Truncation behavior**: `3.9→3`, `-3.9→-3` (not rounded)

---

## Context-Specific Rules

| Context | Type | Notes |
|---------|------|-------|
| FOR loop counter | INT | Always, regardless of suffix |
| Array index | INT | Auto-converted if needed |
| MOD/AND/OR/XOR | INT | Forces operands to INT |
| Comparison | INT | Result is 0 or 1 |
| Literal | DOUBLE | Unless `%` suffix |
| Variable | DOUBLE | Unless `%` suffix |

---

## Example

```basic
a% = 10         ' INT
b = 2.5         ' DOUBLE
c% = a% * b     ' Result?
```

Generated QBE:
```qbe
%a =w copy 10
%b =d copy d_2.5
%t1 =d extsw %a         # INT → DOUBLE
%t2 =d mul %t1, %b      # DOUBLE * DOUBLE
%t3 =w dtosi %t2        # DOUBLE → INT (truncate)
%c =w copy %t3
```

Result: `c% = 25`

---

## When to Use INT

Use `%` suffix when:
1. **Semantics matter**: Array indices, loop counters, bitwise ops
2. **Range is small**: Fits in 32-bit signed (-2³¹ to 2³¹-1)
3. **External APIs**: C functions expecting integers

Don't micro-optimize for "speed" — INT↔DOUBLE conversion is ~1-3 CPU cycles.