# FasterBASIC Type Suffix Quick Reference

## Type Suffixes

| Suffix | Type      | Size    | QBE Type | Range                    | Example       |
|--------|-----------|---------|----------|--------------------------|---------------|
| `@`    | BYTE      | 8-bit   | sb       | -128 to 127              | `count@`      |
| `^`    | SHORT     | 16-bit  | sh       | -32,768 to 32,767        | `offset^`     |
| `%`    | INTEGER   | 32-bit  | w        | -2.1B to 2.1B            | `value%`      |
| `&`    | LONG      | 64-bit  | l        | -9.2E to 9.2E            | `bignum&`     |
| `!`    | SINGLE    | 32-bit  | s        | ±3.4E±38 (7 digits)      | `price!`      |
| `#`    | DOUBLE    | 64-bit  | d        | ±1.7E±308 (15 digits)    | `precise#`    |
| `$`    | STRING    | ptr     | l        | Text (ASCII or UTF-32)   | `name$`       |

## Explicit Type Keywords (AS Clause)

```basic
DIM x AS BYTE          ' 8-bit signed
DIM x AS UBYTE         ' 8-bit unsigned (0-255)
DIM x AS SHORT         ' 16-bit signed
DIM x AS USHORT        ' 16-bit unsigned (0-65535)
DIM x AS INTEGER       ' 32-bit signed (default)
DIM x AS UINTEGER      ' 32-bit unsigned (0-4.2B)
DIM x AS LONG          ' 64-bit signed
DIM x AS ULONG         ' 64-bit unsigned
DIM x AS SINGLE        ' 32-bit float
DIM x AS DOUBLE        ' 64-bit float
DIM x AS STRING        ' String (ASCII or UTF-32 based on OPTION UNICODE)
```

## Type Coercion Matrix

### Safe Implicit Conversions (No Warning)

```
BYTE → SHORT → INTEGER → LONG
              ↓
           SINGLE → DOUBLE
```

### Examples

```basic
DIM b@ AS BYTE
DIM i% AS INTEGER
DIM n& AS LONG
DIM x# AS DOUBLE

b@ = 10
i% = b@        ' ✓ OK: BYTE → INTEGER (widening)
n& = i%        ' ✓ OK: INTEGER → LONG (widening)
x# = n&        ' ✓ OK: LONG → DOUBLE (widening)
```

### Lossy Conversions (Warning)

```basic
DIM n& AS LONG
DIM i% AS INTEGER
DIM b@ AS BYTE

n& = 5000000000&
i% = n&        ' ⚠ WARNING: LONG → INTEGER (may overflow)
b@ = i%        ' ⚠ WARNING: INTEGER → BYTE (may overflow)
```

### Explicit Conversions Required

```basic
DIM x# AS DOUBLE
DIM i% AS INTEGER

x# = 3.14159
i% = x#        ' ✗ ERROR: Use CINT() or INT()
i% = CINT(x#)  ' ✓ OK: Explicit conversion
```

## Conversion Functions

| Function  | Purpose                     | Example              |
|-----------|-----------------------------|----------------------|
| `CBYTE()` | Convert to BYTE             | `b@ = CBYTE(100)`    |
| `CSHORT()`| Convert to SHORT            | `s^ = CSHORT(1000)`  |
| `CINT()`  | Convert to INTEGER          | `i% = CINT(x#)`      |
| `CLNG()`  | Convert to LONG             | `n& = CLNG(x#)`      |
| `CSNG()`  | Convert to SINGLE           | `x! = CSNG(n&)`      |
| `CDBL()`  | Convert to DOUBLE           | `y# = CDBL(i%)`      |
| `INT()`   | Truncate to integer         | `i% = INT(x#)`       |
| `FIX()`   | Remove fractional part      | `i% = FIX(x#)`       |
| `VAL()`   | String to number            | `x# = VAL(s$)`       |
| `STR$()`  | Number to string            | `s$ = STR$(x#)`      |

## Complete Coercion Rules

### Integer ↔ Integer

| From → To | BYTE | SHORT | INTEGER | LONG |
|-----------|------|-------|---------|------|
| BYTE      | =    | Auto  | Auto    | Auto |
| SHORT     | Warn | =     | Auto    | Auto |
| INTEGER   | Warn | Warn  | =       | Auto |
| LONG      | Warn | Warn  | Warn    | =    |

### Float ↔ Float

| From → To | SINGLE | DOUBLE |
|-----------|--------|--------|
| SINGLE    | =      | Auto   |
| DOUBLE    | Warn   | =      |

### Integer ↔ Float

| From → To | SINGLE | DOUBLE |
|-----------|--------|--------|
| BYTE      | Auto   | Auto   |
| SHORT     | Auto   | Auto   |
| INTEGER   | Auto   | Auto   |
| LONG      | Warn*  | Auto   |

*LONG → SINGLE may lose precision for values > 2^24

| From → To | BYTE | SHORT | INTEGER | LONG |
|-----------|------|-------|---------|------|
| SINGLE    | Func | Func  | Func    | Func |
| DOUBLE    | Func | Func  | Func    | Func |

### String ↔ Numeric

| From → To | Numeric | String |
|-----------|---------|--------|
| Numeric   | -       | STR$() |
| String    | VAL()   | -      |

## Usage Examples

### Traditional Style (Suffixes)

```basic
DIM counter% = 0
DIM total& = 0
DIM price# = 99.99
DIM name$ = "Product"

FOR counter% = 1 TO 100
    total& = total& + counter%
NEXT counter%

PRINT name$; " total: "; total&
```

### Modern Style (Explicit Types)

```basic
DIM counter AS INTEGER = 0
DIM total AS LONG = 0
DIM price AS DOUBLE = 99.99
DIM name AS STRING = "Product"

FOR counter = 1 TO 100
    total = total + counter
NEXT counter

PRINT name; " total: "; total
```

### Mixed Style (Both)

```basic
DIM counter% AS INTEGER = 0     ' Suffix + explicit (redundant but allowed)
DIM total AS LONG = 0           ' Explicit only
DIM price# = 99.99              ' Suffix only
```

## Array Examples

```basic
' Suffix style
DIM numbers%(100)               ' Array of INTEGERs
DIM values#(50, 50)             ' 2D array of DOUBLEs
DIM names$(10)                  ' Array of STRINGs

' Explicit style
DIM bytes(256) AS BYTE          ' Array of BYTEs
DIM indices(1000) AS LONG       ' Array of LONGs
DIM matrix(10, 10) AS DOUBLE    ' 2D array of DOUBLEs

' Access
bytes(0) = 255
indices(0) = 9999999999&
matrix(5, 5) = 3.14159
```

## User-Defined Type Fields

```basic
TYPE Pixel
    r@ AS BYTE        ' Red: 0-255 (using suffix)
    g AS BYTE         ' Green: 0-255 (no suffix needed)
    b AS BYTE         ' Blue: 0-255
    a AS BYTE         ' Alpha: 0-255
END TYPE

TYPE Rectangle
    x# AS DOUBLE      ' Position X
    y# AS DOUBLE      ' Position Y
    width% AS INTEGER ' Width in pixels
    height AS INTEGER ' Height in pixels
END TYPE
```

## Common Patterns

### Loop with Correct Type

```basic
' Small loop: INTEGER is fine
DIM i% AS INTEGER
FOR i% = 1 TO 1000
    PRINT i%
NEXT i%

' Large loop: Use LONG
DIM bigIndex& AS LONG
FOR bigIndex& = 1 TO 10000000000&
    ' Process large dataset
NEXT bigIndex&
```

### Byte Manipulation

```basic
DIM b@ AS BYTE
DIM flags@ AS BYTE = 0

' Set bit 0
flags@ = flags@ OR 1

' Check bit 1
IF (flags@ AND 2) <> 0 THEN
    PRINT "Bit 1 is set"
END IF

' Use unsigned for 0-255 range
DIM color AS UBYTE = 255
```

### Precision Control

```basic
DIM quickCalc! AS SINGLE = 3.14159!     ' Fast, less precise
DIM preciseCalc# AS DOUBLE = 3.14159#   ' Slower, more precise

' Money: always use DOUBLE or INTEGER cents
DIM price# AS DOUBLE = 19.99
DIM cents% AS INTEGER = 1999            ' Better for money
```

## Tips

1. **Default to INTEGER (%) for whole numbers** - it's the BASIC standard
2. **Use LONG (&) for array indices** if array is large (>2GB)
3. **Use BYTE (@) for pixel data** and other 0-255 values
4. **Use DOUBLE (#) for scientific calculations** requiring precision
5. **Use SINGLE (!) for graphics** and other speed-critical float ops
6. **Avoid mixing signed/unsigned** unless you need specific bit patterns
7. **Always use explicit conversion** when narrowing (large to small type)

## Comparison with Other BASICs

| Type       | FreeBasic  | QB64      | FasterBASIC |
|------------|------------|-----------|-------------|
| 8-bit      | `BYTE`     | `_BYTE`   | `BYTE` `@`  |
| 16-bit     | `SHORT`    | `INTEGER` | `SHORT` `^` |
| 32-bit     | `INTEGER`  | `LONG`    | `INTEGER` `%`|
| 64-bit     | `LONGINT`  | `_INTEGER64`| `LONG` `&`|
| 32-bit FP  | `SINGLE`   | `SINGLE`  | `SINGLE` `!`|
| 64-bit FP  | `DOUBLE`   | `DOUBLE`  | `DOUBLE` `#`|

**Key Difference**: FasterBASIC follows QBE's model where:
- INTEGER is 32-bit (not 16-bit like QB45)
- LONG is 64-bit (standard modern size)
- All types map cleanly to QBE types