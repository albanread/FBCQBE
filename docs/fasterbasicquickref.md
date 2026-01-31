# FasterBASIC Quick Reference Guide

## Overview

FasterBASIC is a modern BASIC dialect compiler that compiles to native code via QBE IL. It supports both classic line-numbered BASIC and modern structured programming with SUBs and FUNCTIONs.

## Basic Syntax

### Program Structure

```basic
' Modern style (no line numbers)
PRINT "Hello World"
END

' Classic style (with line numbers)
10 PRINT "Hello World"
20 END
```

### Comments

```basic
REM This is a comment
' This is also a comment
```

## Variables and Types

### Type Suffixes

- `%` - INTEGER (32-bit signed integer)
- `&` - LONG (64-bit signed integer)
- `#` - DOUBLE (64-bit floating point)
- `!` - SINGLE (32-bit floating point)
- `$` - STRING

### Variable Declaration

```basic
' Implicit declaration (with type suffix)
LET x% = 10
LET name$ = "Alice"
LET pi# = 3.14159

' Explicit declaration with DIM
DIM count AS INTEGER
DIM temperature AS DOUBLE
DIM message AS STRING

' Scope modifiers
LOCAL x%         ' Local to SUB/FUNCTION
GLOBAL total%    ' Global across program
SHARED count%    ' Share parent scope variable
```

### User-Defined Types

```basic
TYPE Point
    x AS INTEGER
    y AS INTEGER
END TYPE

DIM p AS Point
p.x = 10
p.y = 20
```

## Operators

### Arithmetic
- `+` Addition
- `-` Subtraction
- `*` Multiplication
- `/` Division
- `MOD` Modulus
- `^` Exponentiation

### Comparison
- `=` Equal
- `<>` Not equal
- `<` Less than
- `>` Greater than
- `<=` Less than or equal
- `>=` Greater than or equal

### Logical/Bitwise
- `AND` Logical/bitwise AND
- `OR` Logical/bitwise OR
- `XOR` Logical/bitwise XOR
- `NOT` Logical/bitwise NOT
- `EQV` Equivalence
- `IMP` Implication

*Note: Use `OPTION BITWISE` or `OPTION LOGICAL` to control operator behavior*

## Control Flow

### IF Statement

```basic
' Single-line IF
IF x > 10 THEN PRINT "Large" ELSE PRINT "Small"

' Multi-line IF
IF x > 10 THEN
    PRINT "Large"
    PRINT "Very large"
ELSEIF x > 5 THEN
    PRINT "Medium"
ELSE
    PRINT "Small"
END IF

' IF with GOTO (classic style)
IF x = 0 THEN GOTO 1000
```

### SELECT CASE

```basic
SELECT CASE score%
    CASE 90 TO 100
        PRINT "A"
    CASE 80 TO 89
        PRINT "B"
    CASE 70 TO 79
        PRINT "C"
    CASE 1, 2, 3
        PRINT "Multiple values"
    CASE IS > 100
        PRINT "Extra credit"
    CASE ELSE
        PRINT "F"
END SELECT
```

### GOTO and GOSUB

```basic
GOTO label_name
GOTO 1000

GOSUB subroutine
GOSUB 2000
RETURN
```

### ON GOTO/GOSUB

```basic
ON x GOTO label1, label2, label3
ON x GOSUB 1000, 2000, 3000
```

## Loops

### FOR Loop

```basic
' Basic FOR loop
FOR i = 1 TO 10
    PRINT i
NEXT i

' FOR with STEP
FOR i = 0 TO 100 STEP 10
    PRINT i
NEXT i

' Counting down
FOR i = 10 TO 1 STEP -1
    PRINT i
NEXT i

' FOR EACH (array iteration)
FOR EACH item IN array$
    PRINT item
NEXT
```

### WHILE Loop

```basic
WHILE x < 10
    PRINT x
    x = x + 1
WEND
' or END WHILE
```

### REPEAT/UNTIL Loop

```basic
REPEAT
    INPUT x
    PRINT x
UNTIL x = 0
```

### DO Loop

```basic
' DO...LOOP with condition at start
DO WHILE x < 10
    PRINT x
    x = x + 1
LOOP

DO UNTIL x >= 10
    PRINT x
    x = x + 1
LOOP

' DO...LOOP with condition at end
DO
    PRINT x
    x = x + 1
LOOP WHILE x < 10

DO
    PRINT x
    x = x + 1
LOOP UNTIL x >= 10
```

### EXIT Statements

```basic
EXIT FOR        ' Exit FOR loop
EXIT DO         ' Exit DO loop
EXIT WHILE      ' Exit WHILE loop
EXIT REPEAT     ' Exit REPEAT loop
EXIT FUNCTION   ' Exit FUNCTION
EXIT SUB        ' Exit SUB
```

## Arrays

### Array Declaration

```basic
' Single dimension
DIM numbers%(10)          ' Elements 0-10 (or 1-10 with OPTION BASE 1)
DIM names$(5)

' Multi-dimensional
DIM matrix%(10, 10)
DIM cube#(5, 5, 5)

' Dynamic arrays
REDIM values%(size%)
REDIM PRESERVE values%(new_size%)  ' Keep existing data
```

### Array Operations

```basic
' Access elements
LET numbers%(5) = 42
PRINT numbers%(5)

' Erase array
ERASE numbers%

' Swap variables
SWAP a%, b%
```

## Strings

### String Functions

```basic
LEN(s$)              ' Length of string
LEFT$(s$, n)         ' Left n characters
RIGHT$(s$, n)        ' Right n characters
MID$(s$, pos, len)   ' Substring starting at pos
CHR$(n)              ' Character from ASCII code
ASC(s$)              ' ASCII code of first character
STR$(n)              ' Number to string
VAL(s$)              ' String to number
UCASE$(s$)           ' Uppercase
LCASE$(s$)           ' Lowercase
LTRIM$(s$)           ' Remove leading spaces
RTRIM$(s$)           ' Remove trailing spaces
INSTR(s1$, s2$)      ' Find substring position
SPACE$(n)            ' String of n spaces
STRING$(n, c$)       ' Repeat character n times
```

### String Slicing

```basic
' Access string slice
PRINT s$[1:5]        ' Characters 1-5
PRINT s$[:5]         ' First 5 characters
PRINT s$[5:]         ' From character 5 to end

' Assign to string slice
MID$(s$, 3, 2) = "XX"
s$[3:4] = "XX"
```

## Input/Output

### PRINT Statement

```basic
PRINT "Hello, World!"
PRINT x%, y%, z%
PRINT "Value: "; x%    ' Semicolon: no space
PRINT "Value: ", x%    ' Comma: tab spacing
PRINT x%;              ' No newline at end
```

### INPUT Statement

```basic
INPUT x%
INPUT "Enter name: "; name$
INPUT "X, Y: "; x%, y%

' Line input (entire line)
LINE INPUT prompt$, response$
```

### File I/O

```basic
' Open file
OPEN "filename.txt" FOR INPUT AS #1
OPEN "filename.txt" FOR OUTPUT AS #2
OPEN "filename.txt" FOR APPEND AS #3

' Read from file
INPUT #1, x%, y$

' Write to file
PRINT #2, "Data: "; x%
WRITE #2, x%, y$

' Close file
CLOSE #1
CLOSE              ' Close all files
```

## Procedures and Functions

### SUB Definition

```basic
SUB PrintSum(a%, b%)
    LOCAL result%
    result% = a% + b%
    PRINT result%
END SUB

' Call SUB
CALL PrintSum(10, 20)
PrintSum 10, 20        ' CALL is optional
```

### FUNCTION Definition

```basic
FUNCTION Add%(a%, b%)
    RETURN a% + b%
END FUNCTION

FUNCTION Square#(x#) AS DOUBLE
    RETURN x# * x#
END FUNCTION

' Call FUNCTION
result% = Add%(5, 3)
area# = Square#(4.5)
```

### Parameter Passing

```basic
SUB ByValue(x%)
    x% = x% + 1       ' Does not affect caller
END SUB

SUB ByRef(BYREF x%)
    x% = x% + 1       ' Modifies caller's variable
END SUB
```

### DEF FN (Single-line Functions)

```basic
DEF FNDouble%(x%) = x% * 2
DEF FNArea#(r#) = 3.14159 * r# * r#

PRINT FNDouble%(5)
PRINT FNArea#(2.5)
```

### Inline IF (IIF)

```basic
result$ = IIF(score >= 60, "Pass", "Fail")
max% = IIF(a% > b%, a%, b%)
```

## Math Functions

```basic
ABS(x)         ' Absolute value
SGN(x)         ' Sign (-1, 0, 1)
INT(x)         ' Integer part
FIX(x)         ' Truncate toward zero
SQR(x)         ' Square root
EXP(x)         ' e^x
LOG(x)         ' Natural logarithm
SIN(x)         ' Sine
COS(x)         ' Cosine
TAN(x)         ' Tangent
ASIN(x)        ' Arcsine
ACOS(x)        ' Arccosine
ATAN(x)        ' Arctangent
ATAN2(y, x)    ' Arctangent of y/x
RND            ' Random number 0-1
```

## Data Statements

```basic
' Define data
DATA 1, 2, 3, 4, 5
DATA "apple", "banana", "cherry"

' Read data
READ x%, y%, z%
READ fruit$

' Reset data pointer
RESTORE
RESTORE label_name
```

## Exception Handling

```basic
TRY
    ' Code that might fail
    x% = 100 / y%
CATCH 11
    ' Handle division by zero
    PRINT "Error: Division by zero"
CATCH
    ' Handle any other error
    PRINT "Error code: "; ERR
    PRINT "Error line: "; ERL
FINALLY
    ' Always executed
    PRINT "Cleanup"
END TRY

' Throw error
THROW 100
```

## Constants

```basic
CONSTANT PI# = 3.14159265
CONSTANT MAX_SIZE% = 1000
CONSTANT APP_NAME$ = "MyApp"
```

## Compiler Directives

```basic
OPTION BASE 0          ' Array indexing starts at 0
OPTION BASE 1          ' Array indexing starts at 1
OPTION EXPLICIT        ' Require variable declarations
OPTION BITWISE         ' AND/OR/XOR are bitwise
OPTION LOGICAL         ' AND/OR/XOR are logical
OPTION UNICODE         ' Enable Unicode strings
OPTION ASCII           ' Use ASCII strings
OPTION DETECTSTRING    ' Auto-detect string type
OPTION ERROR OFF       ' Disable error checking
OPTION CANCELLABLE     ' Allow program cancellation
```

## Utility Commands

```basic
END                    ' End program
SWAP x%, y%            ' Swap two variables
INC x%                 ' Increment x by 1
INC x%, 5              ' Increment x by 5
DEC x%                 ' Decrement x by 1
DEC x%, 3              ' Decrement x by 3
```

## Include Files

```basic
INCLUDE "library.bas"
INCLUDE ONCE "header.bas"  ' Include only once
```

## Type Conversion

### Explicit Conversion

```basic
CINT(x)         ' Convert to INTEGER
CLNG(x)         ' Convert to LONG
CSNG(x)         ' Convert to SINGLE
CDBL(x)         ' Convert to DOUBLE
STR$(x)         ' Convert number to STRING
VAL(s$)         ' Convert STRING to number
```

### Implicit Conversion

FasterBASIC performs automatic type conversion when needed, following these rules:
- INTEGER → LONG → SINGLE → DOUBLE (widening conversions are automatic)
- Narrowing conversions may lose precision
- String to number conversion uses VAL() semantics

## Common Idioms

### Loop Through Array

```basic
DIM items$(10)
FOR i = 1 TO 10
    PRINT items$(i)
NEXT i

' Or with FOR EACH
FOR EACH item$ IN items$
    PRINT item$
NEXT
```

### Menu System

```basic
REPEAT
    PRINT ""
    PRINT "1. New Game"
    PRINT "2. Load Game"
    PRINT "3. Quit"
    INPUT choice%
    SELECT CASE choice%
        CASE 1
            GOSUB NewGame
        CASE 2
            GOSUB LoadGame
        CASE 3
            quit% = 1
    END SELECT
UNTIL quit% = 1
```

### Error Handling Pattern

```basic
TRY
    OPEN filename$ FOR INPUT AS #1
    ' Process file
    CLOSE #1
CATCH
    PRINT "Error opening file: "; filename$
END TRY
```

### Reading and Processing Data

```basic
DATA 10, 20, 30, 40, 50
LET total% = 0
LET count% = 0
WHILE count% < 5
    READ value%
    total% = total% + value%
    count% = count% + 1
WEND
PRINT "Total: "; total%
PRINT "Average: "; total% / count%
```

## Best Practices

1. **Use meaningful variable names** with type suffixes
2. **Declare variables** with DIM or LOCAL/GLOBAL
3. **Use OPTION EXPLICIT** to catch typos
4. **Use structured control flow** (SUB/FUNCTION) over GOTO
5. **Handle errors** with TRY/CATCH
6. **Comment your code** with REM or '
7. **Use CONST** for magic numbers
8. **Close files** after use
9. **Initialize arrays** with DIM before use
10. **Use proper indentation** for readability

## Example Programs

### Hello World

```basic
PRINT "Hello, World!"
END
```

### Factorial Function

```basic
FUNCTION Factorial&(n%)
    IF n% <= 1 THEN
        RETURN 1
    ELSE
        RETURN n% * Factorial&(n% - 1)
    END IF
END FUNCTION

INPUT "Enter number: "; num%
PRINT "Factorial of "; num%; " is "; Factorial&(num%)
END
```

### Bubble Sort

```basic
SUB BubbleSort(arr%(), n%)
    LOCAL i%, j%, temp%
    FOR i% = 1 TO n% - 1
        FOR j% = 1 TO n% - i%
            IF arr%(j%) > arr%(j% + 1) THEN
                SWAP arr%(j%), arr%(j% + 1)
            END IF
        NEXT j%
    NEXT i%
END SUB
```

### File Processing

```basic
DIM lines$(1000)
DIM count%

OPEN "input.txt" FOR INPUT AS #1
count% = 0
WHILE NOT EOF(1)
    count% = count% + 1
    LINE INPUT #1, lines$(count%)
WEND
CLOSE #1

PRINT "Read "; count%; " lines"
```

## Differences from Other BASICs

### From QuickBASIC/QBasic
- Uses modern compiler technology (compiles to native code via QBE)
- Improved string handling with Unicode support
- Better error handling with TRY/CATCH
- Extended type system (BYTE, UBYTE, etc.)
- Event-driven programming support

### From FreeBASIC
- Simpler syntax, closer to classic BASIC
- More lightweight runtime
- Focus on terminal/console applications

### From BBC BASIC
- Different procedure syntax (SUB/FUNCTION vs PROC/FN)
- C-style type suffixes instead of declarative typing

## Resources

- **Compiler**: `qbe_basic` (integrated compiler)
- **Test Suite**: Located in `tests/` directory
- **Documentation**: See `docs/` directory
- **Examples**: See `test_programs/examples/` directory

---

*FasterBASIC Quick Reference v1.0*