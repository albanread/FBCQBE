# FasterBASIC Type Syntax Reference

Quick reference for the QBE-aligned type system syntax additions.

---

## Type Suffixes

### Available Suffixes

| Suffix | Type | Size | Range (Signed) | Example |
|--------|------|------|----------------|---------|
| `@` | BYTE | 8-bit | -128 to 127 | `counter@` |
| `^` | SHORT | 16-bit | -32,768 to 32,767 | `index^` |
| `%` | INTEGER | 32-bit | -2,147,483,648 to 2,147,483,647 | `value%` |
| `&` | LONG | 64-bit | -9,223,372,036,854,775,808 to 9,223,372,036,854,775,807 | `bigNum&` |
| `!` | SINGLE | 32-bit float | ±3.4E±38 (7 digits) | `price!` |
| `#` | DOUBLE | 64-bit float | ±1.7E±308 (15 digits) | `pi#` |
| `$` | STRING | Variable | UTF-8 text | `name$` |

### Suffix Usage

```basic
REM Variable declaration with suffix
DIM counter@
DIM temperature!
DIM productName$

REM Assignment
counter@ = 100
temperature! = 98.6
productName$ = "Widget"

REM In expressions
total% = counter@ * 10
```

---

## AS Type Declarations

### Signed Integer Types

```basic
DIM byteVar AS BYTE        REM 8-bit signed integer
DIM shortVar AS SHORT      REM 16-bit signed integer
DIM intVar AS INTEGER      REM 32-bit signed integer
DIM intVar AS INT          REM Alias for INTEGER
DIM longVar AS LONG        REM 64-bit signed integer
```

### Unsigned Integer Types

```basic
DIM ubyteVar AS UBYTE      REM 8-bit unsigned (0 to 255)
DIM ushortVar AS USHORT    REM 16-bit unsigned (0 to 65,535)
DIM uintVar AS UINTEGER    REM 32-bit unsigned (0 to 4,294,967,295)
DIM uintVar AS UINT        REM Alias for UINTEGER
DIM ulongVar AS ULONG      REM 64-bit unsigned (0 to 18,446,744,073,709,551,615)
```

### Floating Point Types

```basic
DIM floatVar AS SINGLE     REM 32-bit float
DIM floatVar AS FLOAT      REM Alias for SINGLE
DIM doubleVar AS DOUBLE    REM 64-bit float
```

### String Type

```basic
DIM textVar AS STRING      REM Variable-length string
```

---

## Combining Suffixes and AS Clauses

Both forms can be used together (suffix takes precedence):

```basic
DIM x@ AS BYTE             REM Explicit and redundant (but allowed)
DIM y AS BYTE              REM AS clause required (no suffix)
DIM z@                     REM Suffix alone is sufficient
```

---

## Arrays

### Array Declaration with Type

```basic
REM Suffix form
DIM byteArray@(100)
DIM shortArray^(50)
DIM intArray%(1000)

REM AS clause form
DIM explicitArray(100) AS BYTE
DIM matrix(10, 10) AS DOUBLE
DIM flags(256) AS UBYTE
```

---

## Functions and Subroutines

### Function Return Types

```basic
REM Using suffix on function name
FUNCTION GetByte@() AS BYTE
    GetByte@ = 42
END FUNCTION

REM Using AS clause only
FUNCTION GetValue() AS SHORT
    GetValue = 12345
END FUNCTION
```

### Typed Parameters

```basic
REM Using suffixes
FUNCTION AddBytes@(a@, b@) AS BYTE
    AddBytes@ = a@ + b@
END FUNCTION

REM Using AS clauses
FUNCTION AddShorts(x AS SHORT, y AS SHORT) AS SHORT
    AddShorts = x + y
END FUNCTION

REM Mixed style
FUNCTION Process(input@ AS BYTE, scale! AS SINGLE) AS INTEGER
    Process = input@ * scale!
END FUNCTION
```

### Subroutines with Typed Parameters

```basic
SUB PrintByte(value@ AS BYTE)
    PRINT "Byte value: "; value@
END SUB

SUB UpdateCounter(counter@ AS UBYTE, increment^ AS SHORT)
    counter@ = counter@ + increment^
END SUB
```

---

## Type Coercion

### Implicit Widening (Safe)

Smaller types automatically promote to larger types:

```basic
DIM b@ AS BYTE
DIM s^ AS SHORT
DIM i% AS INTEGER
DIM l& AS LONG

b@ = 10
s^ = b@        REM BYTE -> SHORT (safe, implicit)
i% = s^        REM SHORT -> INTEGER (safe, implicit)
l& = i%        REM INTEGER -> LONG (safe, implicit)
```

### Implicit Narrowing (Warning)

Larger types to smaller may cause data loss (compiler warns):

```basic
DIM i% AS INTEGER
DIM s^ AS SHORT

i% = 100000
s^ = i%        REM WARNING: Potential data loss (INTEGER -> SHORT)
```

### Explicit Conversion (Required for Float->Int)

Use conversion functions for float to integer:

```basic
DIM f! AS SINGLE
DIM i% AS INTEGER

f! = 3.14
i% = f!        REM ERROR: Explicit conversion required
i% = CINT(f!)  REM OK: Explicit conversion function
```

---

## Conversion Functions

### Integer Conversions

```basic
CBYTE(expr)    REM Convert to BYTE
CSHORT(expr)   REM Convert to SHORT
CINT(expr)     REM Convert to INTEGER
CLNG(expr)     REM Convert to LONG
```

### Float Conversions

```basic
CSNG(expr)     REM Convert to SINGLE
CDBL(expr)     REM Convert to DOUBLE
```

### String Conversions

```basic
STR$(number)   REM Number to string
VAL(string$)   REM String to number
```

---

## Unsigned Type Considerations

### Range Checking

Unsigned types have different ranges:

```basic
DIM u AS UBYTE
u = 255        REM OK: Maximum value for UBYTE
u = 256        REM ERROR or wraps to 0 (depending on runtime)
u = -1         REM ERROR or wraps to 255
```

### Mixing Signed and Unsigned

```basic
DIM signed@ AS BYTE
DIM unsigned AS UBYTE

signed@ = -50
unsigned = signed@     REM Warning: signed to unsigned conversion
```

---

## Special Type Considerations

### Loop Index Variables

Loop indices are internally promoted to LONG (64-bit) to prevent overflow:

```basic
DIM i@ AS BYTE
FOR i@ = 1 TO 1000     REM Internally uses LONG, then converts back
    REM Loop body
NEXT i@
```

### String Variables

Strings are always reference types (pointers):

```basic
DIM name$ AS STRING
name$ = "Hello"        REM Pointer to string data
```

### User-Defined Types

Type suffixes cannot be used with UDT fields directly:

```basic
TYPE Player
    score AS INTEGER
    health@ AS BYTE    REM ERROR: Suffix not allowed in TYPE
    health AS BYTE     REM OK: Use AS clause
END TYPE
```

---

## Best Practices

### 1. Choose Appropriate Size

Use the smallest type that fits your data:

```basic
REM Good: Small counter
DIM count@ AS BYTE     REM 0-127 range

REM Wasteful: Small counter
DIM count& AS LONG     REM Uses 8 bytes for small values
```

### 2. Use Unsigned for Bit Flags

```basic
DIM flags AS UBYTE     REM 8 flag bits (0-255)
DIM permissions AS USHORT   REM 16 permission bits
```

### 3. Be Explicit with AS Clause

For clarity in function signatures:

```basic
REM Clear and explicit
FUNCTION Calculate(width AS SHORT, height AS SHORT) AS INTEGER
    Calculate = width * height
END FUNCTION
```

### 4. Consistent Naming

Use suffixes for local variables, AS clauses for function parameters:

```basic
FUNCTION Process(input AS BYTE, output AS SHORT) AS INTEGER
    DIM temp@, result^
    temp@ = input * 2
    result^ = temp@ + output
    Process = result^
END FUNCTION
```

---

## Quick Examples

### Example 1: Byte Array Processing

```basic
DIM buffer@(256) AS BYTE
DIM index^, sum%

sum% = 0
FOR index^ = 0 TO 255
    buffer@(index^) = index^ MOD 128
    sum% = sum% + buffer@(index^)
NEXT index^

PRINT "Sum: "; sum%
```

### Example 2: Type Coercion Chain

```basic
DIM b@ AS BYTE
DIM s^ AS SHORT
DIM i% AS INTEGER
DIM l& AS LONG

b@ = 100
s^ = b@ * 2        REM Promotes to SHORT
i% = s^ * 100      REM Promotes to INTEGER
l& = i% * 1000     REM Promotes to LONG

PRINT b@; s^; i%; l&
```

### Example 3: Unsigned Flags

```basic
DIM flags AS UBYTE

REM Set flags using bit operations
flags = 0
flags = flags OR 1    REM Bit 0
flags = flags OR 4    REM Bit 2
flags = flags OR 128  REM Bit 7

IF (flags AND 1) THEN PRINT "Flag 0 is set"
IF (flags AND 4) THEN PRINT "Flag 2 is set"
```

---

## Summary Table

| Feature | Syntax | Example |
|---------|--------|---------|
| BYTE suffix | `@` | `DIM x@` |
| SHORT suffix | `^` | `DIM y^` |
| INTEGER suffix | `%` | `DIM z%` |
| LONG suffix | `&` | `DIM w&` |
| SINGLE suffix | `!` | `DIM a!` |
| DOUBLE suffix | `#` | `DIM b#` |
| STRING suffix | `$` | `DIM s$` |
| AS BYTE | `AS BYTE` | `DIM x AS BYTE` |
| AS UBYTE | `AS UBYTE` | `DIM x AS UBYTE` |
| AS SHORT | `AS SHORT` | `DIM y AS SHORT` |
| AS USHORT | `AS USHORT` | `DIM y AS USHORT` |
| AS INTEGER | `AS INTEGER` | `DIM z AS INTEGER` |
| AS UINTEGER | `AS UINTEGER` | `DIM z AS UINTEGER` |
| AS LONG | `AS LONG` | `DIM w AS LONG` |
| AS ULONG | `AS ULONG` | `DIM w AS ULONG` |

---

## Notes

- All suffixes can be used in variable names, array names, and function names
- AS clauses provide explicit type declarations
- Unsigned types share suffixes with signed types (tracked internally)
- The `^` character is both power operator and SHORT suffix (context determines meaning)
- Name mangling prevents conflicts: `value@` becomes `value_BYTE` internally