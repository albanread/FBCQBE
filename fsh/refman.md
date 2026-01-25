# FBCQBE Language Reference Manual

Version 1.0
Copyright © 2024-2025 FBCQBE Project
Terminal-based BASIC compiler using QBE backend
---

## Table of Contents

1. [Introduction](#introduction)
2. [Program Structure](#program-structure)
3. [Data Types](#data-types)
4. [Variables and Constants](#variables-and-constants)
5. [Operators](#operators)
6. [Control Flow](#control-flow)
7. [Procedures and Functions](#procedures-and-functions)
8. [Arrays](#arrays)
9. [User-Defined Types](#user-defined-types)
10. [Input/Output](#inputoutput)
11. [Graphics and Screen](#graphics-and-screen)
12. [File Operations](#file-operations)
13. [String Functions](#string-functions)
14. [Mathematical Functions](#mathematical-functions)
15. [System Functions](#system-functions)
16. [Error Handling](#error-handling)
17. [Compiler Options](#compiler-options)
18. [Built-in Functions Reference](#built-in-functions-reference)

---

## Introduction

FBCQBE (FasterBASIC QBE) is a native-code ahead-of-time (AOT) compiler for a modern BASIC dialect. It compiles BASIC programs to native machine code through the QBE (Quick Backend) intermediate representation, providing high performance while maintaining familiarity for classic BASIC programmers.

### Key Features

- **Native Performance**: Compiled to machine code, not interpreted
- **Classic BASIC Compatibility**: Line numbers, GOTO, GOSUB support
- **Modern Structured Programming**: Functions, procedures, local variables
- **Rich Type System**: Integers, floats, doubles, strings with slicing, user-defined types
- **String Arrays**: Full read/write access to string arrays
- **String Slicing**: Powerful substring extraction with `s$(start TO end)` syntax
- **Cross-Platform**: Works on x86-64, ARM64, and RISC-V via QBE
- **Terminal-Based**: Console I/O with comprehensive string and numeric operations

---

## Program Structure

### Line Numbers

Programs can use optional line numbers in the classic BASIC style:

```basic
10 PRINT "Hello, World!"
20 END
```

Line numbers must be integers and are typically used with GOTO/GOSUB. Modern code can omit line numbers entirely.

### Labels

Labels provide named jump targets without line numbers:

```basic
START:
PRINT "Enter a number"
INPUT N
IF N < 0 THEN GOTO START
```
Note you can use goto and gosub within reason.
There are some restrictions, you should not use goto to jump in and out of structured loops, or goto in and out of a subroutine.
It was simply not worth every degrading every programs performance to support arbitary leaps.


### Comments

```basic
REM This is a comment
' This is also a comment (single quote)
PRINT "Code" REM inline comment
```

### Program Termination

```basic
END                ' Terminate program
SYSTEM            ' Exit to operating system
STOP              ' Breakpoint for debugging
```

### Program Chaining

```basic
CHAIN "program.bas"    ' Load and run another program
COMMON A, B, C         ' Share variables between chained programs
```

### Program Flow Control

```basic
RUN                 ' Restart program from beginning
RUN 100            ' Restart from line 100
RUN "program.bas"  ' Load and run different program
```

---

## Data Types

### Basic Types

| Type | Suffix | Size | Range | Description |
|------|--------|------|-------|-------------|
| `INTEGER` | `%` | 32-bit | ±2,147,483,647 | Whole numbers |
| `LONG` | `&` | 64-bit | ±9,223,372,036,854,775,807 | Long integers |
| `SINGLE` | `!` | 32-bit | ±3.4E±38 | Single precision floating point |
| `DOUBLE` | `#` | 64-bit | ±1.7E±308 | Double precision floating point |
| `STRING` | `$` | Variable | N/A | Text data |
| `BOOLEAN` | | 8-bit | True/False | Logical values |

### Type Suffixes

Variables can use type suffixes for implicit typing:

```basic
A% = 100        ' Integer
B! = 3.14       ' Float
C# = 1.23456789 ' Double
D$ = "Hello"    ' String
```

Note that all numeric types such as float, double, integer are implemented in the code generation using the one Lua numeric type, but types are used for internal optimizations by the compiler.

### Type Declarations

Explicit type declarations using `AS`:

```basic
DIM Count AS INTEGER
DIM Price AS DOUBLE
DIM Name AS STRING
```

---

## Variables and Constants

### Variable Declaration

```basic
' Implicit declaration (type from suffix or first use)
X = 10
Name$ = "Alice"

' Explicit declaration
DIM Age AS INTEGER
DIM Score AS DOUBLE
```

### Constants

```basic
CONST PI = 3.14159265
CONST MAX_PLAYERS = 4
CONST APP_NAME = "My Game"
```

Constants are evaluated at compile time and cannot be changed.

### Variable Scope and Lifetime

- **Global**: Declared at program level, accessible everywhere
- **Local**: Declared in SUB/FUNCTION with `DIM` or `LOCAL`
- **Static**: Retain value between calls with `STATIC`
- **Shared**: Global variables accessed in SUB/FUNCTION with `SHARED`

```basic
DIM GlobalVar AS INTEGER

SUB MySub()
    LOCAL LocalVar AS INTEGER
    STATIC StaticVar AS INTEGER
    SHARED GlobalVar
    
    LocalVar = 10        ' Reset each call
    StaticVar = StaticVar + 1  ' Retains value
    GlobalVar = 20       ' Accesses global
END SUB
```

### DEF Type Statements

Traditional BASIC type definitions:

```basic
DEFINT A-Z     ' All variables A-Z are INTEGER
DEFLNG L       ' L variables are LONG
DEFSNG S       ' S variables are SINGLE
DEFDBL D       ' D variables are DOUBLE
DEFSTR S       ' S variables are STRING
```

---

## Operators

### Arithmetic Operators

| Operator | Operation | Example |
|----------|-----------|---------|
| `+` | Addition | `A + B` |
| `-` | Subtraction | `A - B` |
| `*` | Multiplication | `A * B` |
| `/` | Division | `A / B` |
| `\` | Integer Division | `A \ B` |
| `^` | Exponentiation | `A ^ B` |
| `MOD` | Modulo | `A MOD B` |

### Comparison Operators

| Operator | Meaning | Example |
|----------|---------|---------|
| `=` | Equal | `A = B` |
| `<>` | Not equal | `A <> B` |
| `<` | Less than | `A < B` |
| `<=` | Less or equal | `A <= B` |
| `>` | Greater than | `A > B` |
| `>=` | Greater or equal | `A >= B` |

### Logical Operators

```basic
OPTION LOGICAL      ' Use TRUE/FALSE for logical operations
OPTION BITWISE      ' Use bitwise operations (default)

' Logical mode
IF (A > 0) AND (B > 0) THEN PRINT "Both positive"
IF (X = 0) OR (Y = 0) THEN PRINT "At least one zero"

' Bitwise mode (operates on integer bits)
Flags = Flag1 OR Flag2
Mask = Value AND &HFF
```

| Operator | Logical Mode | Bitwise Mode |
|----------|--------------|--------------|
| `AND` | Logical AND | Bitwise AND |
| `OR` | Logical OR | Bitwise OR |
| `NOT` | Logical NOT | Bitwise NOT |
| `XOR` | Logical XOR | Bitwise XOR |
| `EQV` | Equivalence | Bitwise EQV |
| `IMP` | Implication | Bitwise IMP |

### Operator Precedence

1. `()` - Parentheses
2. `^` - Exponentiation
3. `-` - Unary minus
4. `*`, `/`, `\`, `MOD` - Multiplication, division
5. `+`, `-` - Addition, subtraction
6. `=`, `<>`, `<`, `<=`, `>`, `>=` - Comparison
7. `NOT` - Logical/bitwise NOT
8. `AND` - Logical/bitwise AND
9. `OR`, `XOR` - Logical/bitwise OR, XOR
10. `EQV`, `IMP` - Equivalence, implication

---

## Control Flow

### IF Statement

```basic
' Single-line IF
IF X > 0 THEN PRINT "Positive"

' Multi-line IF
IF Score > 90 THEN
    PRINT "Grade: A"
    Bonus = 100
END IF

' IF...ELSE
IF Age >= 18 THEN
    PRINT "Adult"
ELSE
    PRINT "Minor"
END IF

' IF...ELSEIF...ELSE
IF Score >= 90 THEN
    Grade$ = "A"
ELSEIF Score >= 80 THEN
    Grade$ = "B"
ELSEIF Score >= 70 THEN
    Grade$ = "C"
ELSE
    Grade$ = "F"
END IF
```

### SELECT CASE Statement

```basic
SELECT CASE DayNum
    CASE 1
        PRINT "Monday"
    CASE 2
        PRINT "Tuesday"
    CASE 6, 7
        PRINT "Weekend"
    CASE ELSE
        PRINT "Other day"
END CASE

' With expressions
SELECT CASE Score
    CASE IS >= 90
        Grade$ = "A"
    CASE IS >= 80
        Grade$ = "B"
    CASE ELSE
        Grade$ = "F"
END CASE
```
Note the BBC BASIC case statements are
also supported if you prefer those.

### FOR Loop

```basic
' Basic FOR loop
FOR I = 1 TO 10
    PRINT I
NEXT I

' With STEP
FOR X = 0 TO 100 STEP 5
    PRINT X
NEXT X

' Countdown
FOR Count = 10 TO 1 STEP -1
    PRINT Count
NEXT Count

' Nested loops
FOR Row = 1 TO 5
    FOR Col = 1 TO 5
        PRINT Row * Col;
    NEXT Col
    PRINT
NEXT Row
```

### FOR...IN Loop

```basic
' Iterate over array
DIM Names$(5)
Names$(1) = "Alice"
Names$(2) = "Bob"
Names$(3) = "Carol"

FOR Name$ IN Names$
    PRINT Name$
NEXT Name$
```

### WHILE Loop

```basic
WHILE Condition
    ' Loop body
WEND

' Example
Count = 0
WHILE Count < 10
    PRINT Count
    Count = Count + 1
WEND
```

### DO...LOOP

```basic
' DO WHILE (condition checked at start)
DO WHILE X < 100
    X = X * 2
LOOP

' DO UNTIL (condition checked at start)
DO UNTIL Done
    PRINT "Processing..."
LOOP

' REPEAT...UNTIL (condition checked at end)
REPEAT
    INPUT "Enter password: ", Pass$
UNTIL Pass$ = "secret"
```

### GOTO and GOSUB

```basic
' GOTO - Unconditional jump
GOTO 100
100 PRINT "Jumped here"

' GOSUB - Call subroutine
GOSUB 1000
PRINT "After subroutine"
END

1000 REM Subroutine
    PRINT "In subroutine"
    RETURN
```

### ON...GOTO and ON...GOSUB

```basic
' ON...GOTO
INPUT "Select 1-3: ", Choice
ON Choice GOTO 100, 200, 300

100 PRINT "Option 1": GOTO 400
200 PRINT "Option 2": GOTO 400
300 PRINT "Option 3"
400 END

' ON...GOSUB
ON MenuChoice GOSUB HandleFile, HandleEdit, HandleView
```

### EXIT Statement

```basic
' EXIT FOR - Exit a FOR loop early
FOR I = 1 TO 1000
    IF Found THEN EXIT FOR
    ' Search logic
NEXT I

' EXIT FUNCTION - Return from function
FUNCTION FindValue(Arr(), Target)
    FOR I = 1 TO UBOUND(Arr)
        IF Arr(I) = Target THEN EXIT FUNCTION I
    NEXT I
    EXIT FUNCTION -1
END FUNCTION

' EXIT SUB - Return from subroutine
SUB ProcessData()
    IF DataEmpty THEN EXIT SUB
    ' Process logic
END SUB
```

Note that the index variable can not be
changed in a FOR NEXT loop, you can not
exit a loop by changing the index.
Use EXIT FOR instead.

---

## Procedures and Functions

### Subroutines (SUB)

```basic
SUB Greet(Name AS STRING)
    PRINT "Hello, "; Name; "!"
END SUB

CALL Greet("Alice")
Greet("Bob")  ' CALL is optional
```

### Functions

```basic
FUNCTION Square(X AS DOUBLE) AS DOUBLE
    Square = X * X
END FUNCTION

FUNCTION Max(A AS INTEGER, B AS INTEGER) AS INTEGER
    IF A > B THEN
        Max = A
    ELSE
        Max = B
    END IF
END FUNCTION

' Using functions
Result = Square(5)
Largest = Max(10, 20)
```

### Parameter Passing

```basic
' By value (default) - copy of value passed
SUB Increment(Value AS INTEGER)
    Value = Value + 1  ' Only changes local copy
END SUB

' By reference - original variable modified
SUB IncrementRef(BYREF Value AS INTEGER)
    Value = Value + 1  ' Changes original
END SUB

DIM X AS INTEGER
X = 10
CALL Increment(X)      ' X still 10
CALL IncrementRef(X)   ' X now 11
```

### Local and Shared Variables

```basic
DIM GlobalCount AS INTEGER

SUB UpdateCount()
    ' Declare local variable
    LOCAL Temp AS INTEGER

    ' Access global variable
    SHARED GlobalCount

    Temp = 100
    GlobalCount = GlobalCount + 1
END SUB
```

### DEF FN (Single-line Functions)

```basic
' Define single-line function
DEF FN Double(X) = X * 2
DEF FN Hypotenuse(A, B) = SQR(A^2 + B^2)

' Use functions
Result = FN Double(5)
Distance = FN Hypotenuse(3, 4)
```

---

## Arrays

### Array Declaration

```basic
' Numeric arrays
DIM Numbers(10) AS INTEGER
DIM Values(10) AS DOUBLE

' String arrays
DIM Names$(10) AS STRING
DIM Names$(10)        ' Type suffix also works

' Multi-dimensional arrays
DIM Matrix(5, 5) AS INTEGER
DIM Grid(10, 10, 10) AS DOUBLE
```

### Array Initialization

```basic
' Individual elements
Numbers(1) = 100
Numbers(2) = 200

' String array elements
Names$(0) = "Alice"
Names$(1) = "Bob"
Names$(2) = "Charlie"

' Fill entire array with value
Numbers() = 0

' Using expressions for dimensions
Size = 100
DIM Buffer(Size) AS INTEGER
```

### Array Operations

```basic
' REDIM - Resize array (loses data)
REDIM Numbers(20)

' REDIM PRESERVE - Resize and keep data
REDIM PRESERVE Numbers(20)

' ERASE - Deallocate array
ERASE Numbers

' SWAP - Exchange two variables/array elements
SWAP A, B
SWAP Array(1), Array(10)
```

### Array Bounds

```basic
' Get array bounds
DIM MyArray(50) AS INTEGER
Lower = LBOUND(MyArray)  ' Returns 1 (or 0 if OPTION BASE 0)
Upper = UBOUND(MyArray)  ' Returns 50

' Use in loops
FOR I = LBOUND(MyArray) TO UBOUND(MyArray)
    MyArray(I) = I * 10
NEXT I
```

### Array Arithmetic (SIMD)

```basic
' Whole array operations (optimized with SIMD)
DIM A(100) AS DOUBLE
DIM B(100) AS DOUBLE
DIM C(100) AS DOUBLE

A() = 1.0           ' Fill with constant
B() = 2.0
C() = A() + B()     ' Element-wise addition
A() = A() * 2.0     ' Scale all elements
```

---

## User-Defined Types

### Defining Types

```basic
TYPE Point
    X AS DOUBLE
    Y AS DOUBLE
END TYPE

TYPE Sprite
    Name AS STRING
    Position AS Point    ' Nested type
    Active AS INTEGER
    Health AS DOUBLE
END TYPE
```

### Using Types

```basic
' Declare variable of custom type
DIM Player AS Sprite
DIM Enemy AS Sprite

' Access members
Player.Name = "Hero"
Player.Position.X = 100
Player.Position.Y = 200
Player.Health = 100.0

' Arrays of custom types
DIM Enemies(10) AS Sprite
Enemies(1).Name = "Goblin"
Enemies(1).Position.X = 50
Enemies(1).Health = 30

' 2D arrays of types
DIM Grid(10, 10) AS Point
Grid(5, 5).X = 100
Grid(5, 5).Y = 200
```

### Type Member Access in Expressions

```basic
Distance = Player.Position.X + Player.Position.Y
TotalHealth = Player.Health + Enemy.Health

IF Player.Position.X > Enemy.Position.X THEN
    PRINT "Player is to the right"
END IF
```

---

## Input/Output

### PRINT Statement

```basic
' Basic output
PRINT "Hello"
PRINT X
PRINT "Value:", X

' Multiple items (comma = tab spacing)
PRINT "Name", "Age", "Score"
PRINT Name$, Age, Score

' Semicolon = no spacing
PRINT "X="; X; " Y="; Y

' Suppress newline with trailing semicolon
PRINT "Loading";
PRINT "."

' Question mark shorthand
? "Hello World"

' Formatted output with USING
PRINT USING "###.##"; Value
PRINT USING "Name: @@@@@@@@@@"; Name$
```

### INPUT Statement

```basic
' Basic input
INPUT X
INPUT Name$

' With prompt
INPUT "Enter your name: ", Name$
INPUT "Enter age: ", Age

' Multiple values
INPUT "Enter X and Y: ", X, Y
```

### LINE INPUT Statement

```basic
' Read entire line including spaces
LINE INPUT "Enter your full name: ", FullName$
LINE INPUT TextLine$

' Read from file
LINE INPUT #1, TextLine$
```

### READ and DATA Statements

```basic
' DATA statements store literal values
DATA 10, 20, 30, "Alice", "Bob", "Charlie"
DATA 3.14, 2.71, 1.41

' READ values into variables
READ A, B, C, Name1$, Name2$, Name3$
READ PI, E, SQRT2

' RESTORE - Reset DATA pointer
RESTORE
RESTORE 100  ' Reset to specific line
```

### INKEY$ Function

```basic
' Read single character (non-blocking)
Key$ = INKEY$
IF Key$ <> "" THEN
    PRINT "You pressed: "; Key$
END IF
```

### Console Output

```basic
' Output to console (separate from screen output)
CONSOLE "Debug: X =", X
CONSOLE "Warning: Invalid value"
```

### Timing and Delays

```basic
' Wait for specified milliseconds
WAIT_MS 1000    ' Wait 1 second
WAIT_MS Delay   ' Wait for variable milliseconds
```

---

## Graphics and Screen

FBCQBE is a terminal-based compiler, so graphics commands operate on the text console.

### Screen Control

```basic
' Clear screen
CLS

' Locate cursor (row, column) - 1-based
LOCATE 10, 20
PRINT "Text at position 10,20"

' Get screen dimensions
Width = SCREENWIDTH()
Height = SCREENHEIGHT()

' Get cursor position
Row = CSRLIN     ' Current row
Col = POS(0)     ' Current column
```

### Text Colors

```basic
' Set text color (foreground)
COLOR 1          ' Blue text
COLOR 2          ' Green text
COLOR 4          ' Red text
COLOR 7          ' White text (default)
COLOR 8          ' Gray text
COLOR 15         ' Bright white text

' Set background color
COLOR , 1        ' Blue background
COLOR 7, 1       ' White text on blue background

' Reset to default colors
COLOR 7, 0
```

### Screen Functions

```basic
' Get screen width/height
W = WIDTH()
H = HEIGHT()

' Check if color is supported
HasColor = COLOR_SUPPORTED()

' Get current color settings
FG = FOREGROUND_COLOR()
BG = BACKGROUND_COLOR()
```

### Sound Commands

```basic
' Simple beep
BEEP

' Play tone (frequency in Hz, duration in milliseconds)
SOUND 440, 500    ' A note for 0.5 seconds
SOUND 880, 250    ' A octave higher for 0.25 seconds
```

---

## File Operations

### Opening Files

```basic
' Open file for reading
OPEN "data.txt" FOR INPUT AS #1

' Open file for writing
OPEN "output.txt" FOR OUTPUT AS #2

' Open file for appending
OPEN "log.txt" FOR APPEND AS #3

' Close files
CLOSE #1
CLOSE #2
CLOSE #3

' Close all files
CLOSE
```

### Reading from Files

```basic
' Read formatted data
OPEN "scores.txt" FOR INPUT AS #1
INPUT #1, Name$, Score
CLOSE #1

' Read line by line
OPEN "data.txt" FOR INPUT AS #1
WHILE NOT EOF(1)
    LINE INPUT #1, TextLine$
    PRINT TextLine$
WEND
CLOSE #1
```

### Writing to Files

```basic
' Write formatted data
OPEN "output.txt" FOR OUTPUT AS #1
PRINT #1, "Name", "Score"
PRINT #1, Name$, Score
CLOSE #1

' Write with WRITE# (adds quotes around strings)
OPEN "data.csv" FOR OUTPUT AS #1
WRITE #1, Name$, Age, Score
CLOSE #1
```

### File Functions

```basic
' Check if at end of file
IF EOF(1) THEN PRINT "End of file reached"

' Get file position
Position = LOC(1)

' Get file size
Size = LOF(1)
```

---

## String Functions

### String Manipulation

```basic
' Length of string
Len = LEN("Hello")  ' Returns 5

' Extract substring
S$ = "Hello World"
Left$ = LEFT$(S$, 5)      ' "Hello"
Right$ = RIGHT$(S$, 5)    ' "World"
Mid$ = MID$(S$, 7, 5)     ' "World"

' Find substring position
Pos = INSTR("Hello World", "World")  ' Returns 7

' String comparison
IF STRCMP(A$, B$) = 0 THEN PRINT "Equal"

' Convert case
Upper$ = UCASE$("hello")  ' "HELLO"
Lower$ = LCASE$("HELLO")  ' "hello"

' Trim whitespace
Trimmed$ = LTRIM$(S$)     ' Left trim
Trimmed$ = RTRIM$(S$)     ' Right trim
Trimmed$ = TRIM$(S$)      ' Both ends
```

### String Slicing

```basic
' Extract substrings using slicing syntax
S$ = "Hello World"

' Basic slice with start and end positions
Slice$ = S$(7 TO 11)    ' "World"

' Slice from start to position
Left$ = S$(TO 5)        ' "Hello"

' Slice from position to end
Right$ = S$(7 TO)       ' "World"

' Single character slice
Char$ = S$(7 TO 7)      ' "W"

' Using variables in slice expressions
StartPos = 7
EndPos = 11
Slice$ = S$(StartPos TO EndPos)
```

### String Conversion

```basic
' Number to string
S$ = STR$(123)      ' " 123" (with leading space)
S$ = STR$(45.67)

' String to number
X = VAL("123")      ' Returns 123
Y = VAL("45.67")    ' Returns 45.67

' Character/ASCII conversion
C$ = CHR$(65)       ' "A"
Code = ASC("A")     ' 65
```

### String Building

```basic
' Concatenation
FullName$ = FirstName$ + " " + LastName$

' Repeat character
Stars$ = STRING$(10, "*")    ' "**********"
Spaces$ = SPACE$(5)          ' "     "

' Format string
S$ = HEX$(255)               ' "FF"
S$ = OCT$(8)                 ' "10"
S$ = BIN$(5)                 ' "101"
```

### Unicode Strings

```basic
OPTION UNICODE

' Unicode string length (character count, not bytes)
Len = LEN("Hello 世界")  ' Returns 8

' Unicode substring operations
S$ = "Hello 世界"
Left$ = LEFT$(S$, 5)     ' "Hello"
Right$ = RIGHT$(S$, 2)   ' "世界"

' Unicode character at position
C$ = MID$(S$, 7, 1)      ' "世"
```

---

## Mathematical Functions

### Basic Math Functions

```basic
' Absolute value
X = ABS(-5)         ' Returns 5

' Sign
S = SGN(-10)        ' Returns -1 (or 0, 1)

' Square root
R = SQR(16)         ' Returns 4

' Power
P = POW(2, 8)       ' Returns 256

' Integer operations
I = INT(3.7)        ' Returns 3 (floor)
I = FIX(3.7)        ' Returns 3 (truncate)
I = CINT(3.7)       ' Returns 4 (round)
```

### Trigonometric Functions

```basic
' Basic trig (angles in radians)
S = SIN(X)
C = COS(X)
T = TAN(X)

' Inverse trig
A = ATN(X)          ' Arctangent
A = ASIN(X)         ' Arcsine
A = ACOS(X)         ' Arccosine

' Two-argument arctangent
Angle = ATN2(Y, X)

' Hyperbolic functions
SH = SINH(X)
CH = COSH(X)
TH = TANH(X)
```

### Logarithmic and Exponential

```basic
' Natural logarithm
L = LOG(X)          ' ln(x)

' Base-10 logarithm
L = LOG10(X)

' Exponential
E = EXP(X)          ' e^x
```

### Random Numbers

```basic
' Initialize random seed
RANDOMIZE

' Random number 0 to <1.0
R = RND()           ' Returns float between 0.0 and 1.0

' Random integer from 0 to n-1
Dice = RAND(6)      ' Returns 0, 1, 2, 3, 4, or 5
Card = RAND(52)     ' Returns 0-51

' Legacy random number usage (still supported)
R = RND(1)          ' Same as RND()
Dice = INT(RND(1) * 6) + 1    ' 1-6 (old style)
```

### Special Math

```basic
' Minimum/maximum
Min = MIN(A, B, C)
Max = MAX(A, B, C)

' Clamp value to range
Clamped = CLAMP(Value, MinVal, MaxVal)

' Linear interpolation
Interpolated = LERP(Start, End, T)

' Degrees/radians conversion
Rads = RAD(180)     ' π
Degs = DEG(PI)      ' 180
```

---

## System Functions

### Date and Time

```basic
' Get current date as string
Today$ = DATE$

' Get current time as string
Now$ = TIME$

' Get timer value (seconds since program start)
Seconds = TIMER      ' Returns float (e.g., 1.234)

' Get timer in milliseconds
Millis = TIMEMS
```

### Environment

```basic
' Get environment variable
Path$ = ENV$("PATH")
Home$ = ENV$("HOME")

' Set environment variable
ENV_SET "MY_VAR", "value"
```

### Command Line

```basic
' Get command line arguments
ArgCount = COMMAND$()
FirstArg$ = COMMAND$(1)
SecondArg$ = COMMAND$(2)
```

### System Information

```basic
' Get free memory
FreeMem = FRE()

' Get system information
OS$ = OS$()        ' Operating system name
Version$ = VERSION$()  ' FBCQBE version
```

### File System

```basic
' Check if file exists
Exists = FILEEXISTS("data.txt")

' Get file size
Size = FILESIZE("data.txt")

' Get current directory
CurrentDir$ = CURDIR$()

' Change directory
CHDIR "subdir"

' Make directory
MKDIR "newdir"

' Remove directory
RMDIR "olddir"
```

---

## Error Handling

### ON ERROR Statement

```basic
' Enable error trapping
ON ERROR GOTO ErrorHandler

' Disable error trapping
ON ERROR GOTO 0

' Error handler subroutine
ErrorHandler:
    PRINT "Error"; ERR; "at line"; ERL
    RESUME NEXT    ' Continue with next statement
    RESUME         ' Retry the failed statement
    RESUME 100     ' Resume at line 100
```

### Error Functions

```basic
' Get error number
ErrorNum = ERR

' Get error line number
LineNum = ERL

' Get error message
Msg$ = ERROR$(ErrorNum)
```

### Error Numbers

Common error codes:
- 1: NEXT without FOR
- 2: Syntax error
- 5: Illegal function call
- 6: Overflow
- 7: Out of memory
- 9: Subscript out of range
- 11: Division by zero
- 13: Type mismatch
- 14: Out of string space
- 19: No RESUME
- 20: RESUME without error
- 53: File not found
- 61: Disk full
- 70: Permission denied

---

## Compiler Options

### Setting Options

Options control compiler behavior and must appear before code that uses the feature.

```basic
' Array base index (0 or 1)
OPTION BASE 0      ' Arrays start at 0
OPTION BASE 1      ' Arrays start at 1 (default)

' Logical vs. Bitwise operators
OPTION LOGICAL     ' AND/OR/NOT are logical operators
OPTION BITWISE     ' AND/OR/NOT are bitwise operators (default)

' Explicit variable declaration
OPTION EXPLICIT    ' All variables must be declared with DIM

' Unicode string support
OPTION UNICODE     ' Enable UTF-8 string handling

' Error line tracking
OPTION ERROR       ' Track line numbers for error reporting

' File inclusion
OPTION ONCE        ' Include file only once (for headers)

' Quasi-preemptive handlers
OPTION FORCE_YIELD ' Force yield points in tight loops
```

### Include Files

```basic
' Include external file
INCLUDE "library.bas"

' Include with once guard
OPTION ONCE
INCLUDE "header.bas"
```

---

## Built-in Functions Reference

### Numeric Functions

```basic
ABS(x)              ' Absolute value
SGN(x)              ' Sign: -1, 0, or 1
INT(x)              ' Integer part (floor)
FIX(x)              ' Truncate towards zero
CINT(x)             ' Round to nearest integer
SQR(x)              ' Square root
POW(x, y)           ' x raised to power y
EXP(x)              ' e raise
