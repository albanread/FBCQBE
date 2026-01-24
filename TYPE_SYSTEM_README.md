# FasterBASIC QBE Type System

## TL;DR

**Numbers default to DOUBLE, not INTEGER. Use `%` suffix explicitly when you need integers.**

```basic
x = 10          ' DOUBLE (10.0)
x% = 10         ' INTEGER (10)
```

This matches classic BASIC philosophy: *"To make it go fast, make it an integer"* — otherwise floats/doubles.

---

## What Changed

The FasterBASIC QBE backend now uses **DOUBLE as the default numeric type** instead of INTEGER.

### Before (Wrong)
- Untyped variables defaulted to INTEGER
- `x = 10` created an integer variable
- Mixed arithmetic had unexpected behaviors

### After (Correct)
- Untyped variables default to DOUBLE
- `x = 10` creates a double variable with value 10.0
- Explicit `%` suffix for integers: `x% = 10`
- Matches classic BASIC, Lua, and modern practice

---

## Quick Reference

### Type Suffixes

| Suffix | Type    | Example        | Notes                    |
|--------|---------|----------------|--------------------------|
| `%`    | INTEGER | `count% = 5`   | 32-bit signed integer    |
| `!`    | FLOAT   | `x! = 3.14`    | Maps to DOUBLE in QBE    |
| `#`    | DOUBLE  | `pi# = 3.14`   | 64-bit double (default)  |
| `$`    | STRING  | `name$ = "A"`  | String type              |
| (none) | DOUBLE  | `value = 42`   | **Defaults to DOUBLE**   |

### Type Conversions

```basic
' Widening (INT → DOUBLE, no loss)
a% = 10
b = a%          ' b = 10.0

' Narrowing (DOUBLE → INT, truncates!)
x = 3.9
y% = x          ' y% = 3 (truncates towards zero)
```

### Three Coercion Points

**1. Expression Evaluation** (mixed types → wider type)
```basic
a% = 5          ' INTEGER
b = 2.5         ' DOUBLE
c = a% + b      ' Result: 7.5 (DOUBLE)
```

**2. Function Calls** (arguments → parameter types)
```basic
FUNCTION Square(x AS DOUBLE) AS DOUBLE
    Square = x * x
END FUNCTION

n% = 4
result = Square(n%)     ' n% promoted to DOUBLE
```

**3. Variable Assignment** (value → variable type)
```basic
x = 10 / 3      ' x = 3.333... (DOUBLE)
y% = 10 / 3     ' y% = 3 (truncates)
```

---

## Special Cases

### Always INTEGER

These contexts **always use integers**:

1. **FOR loop counters**
   ```basic
   FOR i = 1 TO 10         ' i is INTEGER
   FOR x = 1.5 TO 10.5     ' x treated as INTEGER (1 TO 10)
   ```

2. **Array indices**
   ```basic
   DIM arr(10)
   x = 5.7
   arr(x)                  ' x converted to INT (5)
   ```

3. **Bitwise operations** (MOD, AND, OR, XOR, NOT)
   ```basic
   x = 12.9 MOD 5.7        ' 12 MOD 5 = 2 (converts to INT)
   y = 12 AND 5            ' Bitwise AND = 4
   ```

4. **Comparison results** (TRUE=1, FALSE=0)
   ```basic
   IF 3.14 > 3.0 THEN      ' Returns INT (1 or 0)
   ```

---

## Examples

### Example 1: Default DOUBLE
```basic
' All untyped numbers are DOUBLE
x = 10
y = 20
z = x + y
PRINT z             ' 30.0
```

### Example 2: Explicit INTEGER
```basic
' Use % suffix for integers
count% = 0
FOR i% = 1 TO 10
    count% = count% + 1
NEXT i%
PRINT count%        ' 10
```

### Example 3: Mixed Types
```basic
' Automatic type promotion
price = 19.99       ' DOUBLE
qty% = 5            ' INTEGER
total = price * qty%    ' qty% → DOUBLE, result: 99.95
```

### Example 4: Division
```basic
' Division with DOUBLE (default)
x = 10 / 3
PRINT x             ' 3.333...

' Division with explicit INT
a% = 10
b% = 3
c% = a% / b%
PRINT c%            ' 3 (integer division)
```

### Example 5: MOD with Doubles
```basic
' MOD forces integer conversion
m = 10.7
n = 3.2
r = m MOD n         ' 10 MOD 3 = 1
PRINT r             ' 1.0 (result converted back to DOUBLE)
```

---

## When to Use INTEGER

### ✅ Use INTEGER (`%`) for:
- Loop counters (FOR/WHILE)
- Array indices
- Counting discrete items
- Bitwise operations
- When value range fits in 32-bit signed (-2,147,483,648 to 2,147,483,647)

### ❌ Don't Use INTEGER for:
- "Speed optimization" (DOUBLE is fast on modern CPUs!)
- Financial calculations (use DOUBLE for precision, or better yet, scaled integers)
- Scientific calculations
- Default numeric storage

### Modern Reality
- Type conversions are cheap (~1-3 CPU cycles)
- DOUBLE arithmetic is as fast as INT on modern CPUs
- Use DOUBLE by default, explicit `%` only when semantics require integers

---

## Implementation Details

### QBE Type Mapping

| BASIC      | QBE Type | Size   | Notes                          |
|------------|----------|--------|--------------------------------|
| INTEGER    | `w`      | 32-bit | Signed word                    |
| FLOAT      | `d`      | 64-bit | QBE has no single-precision    |
| DOUBLE     | `d`      | 64-bit | 64-bit double precision        |
| STRING     | `l`      | 64-bit | Pointer (64-bit long)          |

### QBE Conversion Instructions

```qbe
# INT → DOUBLE (widening)
%double_val =d extsw %int_val

# DOUBLE → INT (narrowing, truncates)
%int_val =w dtosi %double_val
```

### Generated Code Example

```basic
a% = 10         ' INTEGER
b = 2.5         ' DOUBLE
c = a% + b      ' Mixed arithmetic
```

```qbe
# Variable declarations
%var_a_INT =w copy 0
%var_b =d copy d_0.0
%var_c =d copy d_0.0

# a% = 10
%t0 =d copy d_10.000000
%t1 =w dtosi %t0
%var_a_INT =w copy %t1

# b = 2.5
%t2 =d copy d_2.500000
%var_b =d copy %t2

# c = a% + b
%t3 =d extsw %var_a_INT         # Convert INT to DOUBLE
%t4 =d add %t3, %var_b          # DOUBLE addition
%var_c =d copy %t4
```

---

## Migration Guide

### Classic BASIC Programs

Most classic BASIC programs will work without changes because:
- Explicit type suffixes (`%`, `!`, `#`, `$`) are honored
- FOR loops still use integer counters
- Integer-only operations (MOD, AND, OR, XOR) work correctly

### Programs That May Need Updates

1. **Integer-only arithmetic**: Add `%` suffixes
   ```basic
   ' Before (relied on default INT)
   x = 10
   y = 3
   z = x / y       ' Expected integer division
   
   ' After (explicit INT)
   x% = 10
   y% = 3
   z% = x% / y%    ' Integer division
   ```

2. **Bitwise operations**: Consider explicit `%`
   ```basic
   ' Works (auto-converts)
   flags = 0
   flags = flags OR 1
   
   ' Better (explicit)
   flags% = 0
   flags% = flags% OR 1
   ```

---

## Testing

Test programs included:
- `test_types.bas` - Comprehensive type system tests
- `test_types_simple.bas` - Basic type coercion tests
- `test_function_coercion.bas` - Function parameter tests

Run tests:
```bash
./fsh/fbc_qbe test_types_simple.bas
cat a.out | head -50      # View generated QBE IL
```

---

## Documentation

- **`TYPE_SYSTEM.md`** - Full type system specification
- **`COERCION_STRATEGY.md`** - Three-point coercion strategy
- **`TYPE_QUICK_REF.md`** - Quick reference card
- **`TYPE_SYSTEM_SUMMARY.md`** - Implementation summary

---

## Philosophy

Classic BASIC convention: **"To make it go fast, make it an integer"**

- By default, numbers are DOUBLE (float)
- Programmers explicitly use `%` for integer performance/semantics
- Matches classic BASIC, Lua, and modern best practices
- Trust the defaults, optimize only where needed

---

## FAQ

**Q: Why did we change from INT to DOUBLE default?**  
A: Classic BASIC traditionally defaulted to SINGLE/DOUBLE, not INTEGER. This change aligns with historical behavior and modern numeric practices.

**Q: Won't this be slower?**  
A: No. Modern CPUs handle DOUBLE arithmetic as fast as INT. Type conversions are cheap (1-3 cycles).

**Q: When should I use `%` for integers?**  
A: When semantics require it: loop counters, array indices, bitwise operations, or when your value range fits in 32-bit signed integers.

**Q: What about FLOAT vs DOUBLE?**  
A: QBE only has 64-bit doubles (no 32-bit float). Both `!` and `#` suffixes map to the same QBE `d` type.

**Q: How do I force integer division?**  
A: Use explicit `%` suffixes on both operands and result:
```basic
a% = 10
b% = 3
c% = a% / b%    ' Integer division: c% = 3
```

**Q: My old program broke! What do I do?**  
A: Add `%` suffixes to variables/literals where you need integer semantics. Most programs will work unchanged.

---

## Summary

✅ **Numbers default to DOUBLE**  
✅ **Explicit `%` suffix for INTEGER**  
✅ **Automatic type coercion at three points**  
✅ **Special cases handled correctly**  
✅ **Compatible with classic BASIC**  
✅ **Modern and performant**  

**Trust the defaults, optimize only where needed!**