# ON GOTO and ON GOSUB Tests

## Overview

This document describes the comprehensive test suite for the classic BASIC computed jump statements `ON GOTO` and `ON GOSUB`. These are legacy flow control structures commonly found in vintage BASIC code that need to be supported for backwards compatibility.

## Statement Syntax

### ON GOTO
```basic
ON <expression> GOTO line1, line2, line3, ...
```
Evaluates the expression to an integer index (1-based) and jumps to the corresponding line number.

### ON GOSUB
```basic
ON <expression> GOSUB line1, line2, line3, ...
```
Evaluates the expression to an integer index (1-based) and calls the corresponding subroutine. Execution returns to the next statement after the `ON GOSUB` when `RETURN` is encountered.

## Test Files

### tests/functions/test_on_goto.bas
Tests the ON GOTO statement with 10 comprehensive test cases:

1. **Basic index 1** - Verifies jumping to the first target
2. **Basic index 2** - Verifies jumping to the second target
3. **Basic index 3** - Verifies jumping to the third target
4. **Computed expression** - Tests `ON (5-3) GOTO ...` evaluates to index 2
5. **Index 0** - Verifies fall-through behavior (no jump)
6. **Out-of-range index** - Verifies fall-through when index > number of targets
7. **Negative index** - Verifies fall-through with negative indices
8. **Single target** - Tests ON GOTO with only one destination
9. **Many targets** - Tests with 6 targets, jumping to the 5th
10. **MOD expression** - Tests `ON (17 MOD 3) + 1 GOTO ...`

### tests/functions/test_on_gosub.bas
Tests the ON GOSUB statement with 11 comprehensive test cases:

1. **Basic index 1** - Verifies calling first subroutine and RETURN
2. **Basic index 2** - Verifies calling second subroutine
3. **Basic index 3** - Verifies calling third subroutine
4. **Computed expression** - Tests `ON (7-5) GOSUB ...` evaluates to index 2
5. **Index 0** - Verifies fall-through (no subroutine call)
6. **Out-of-range index** - Verifies fall-through when index > number of targets
7. **Negative index** - Verifies fall-through with negative indices
8. **Single target** - Tests ON GOSUB with only one subroutine
9. **Nested ON GOSUB** - Tests ON GOSUB calling another ON GOSUB
10. **Many targets** - Tests with 6 subroutines, calling the 4th
11. **Execution continues** - Verifies execution resumes after RETURN

## Implementation Details

### Expected Behavior

#### Valid Indices (1 to N)
- Index 1 jumps/calls the first target
- Index 2 jumps/calls the second target
- Index N jumps/calls the Nth target

#### Invalid Indices
- Index ≤ 0: Falls through to next statement (no jump/call)
- Index > N: Falls through to next statement (no jump/call)

### Edge Cases Tested

1. **Expression evaluation** - Not just literal integers, but full expressions
2. **Boundary conditions** - Index 0, 1, last valid, and beyond
3. **Negative values** - Proper handling of negative indices
4. **Single target** - Minimum valid target list
5. **Many targets** - Testing with 6 destinations
6. **Nested calls** - ON GOSUB within ON GOSUB
7. **Return behavior** - Execution continues after RETURN from ON GOSUB

## QBE Code Generation

The compiler generates a switch-like control flow structure:

1. Evaluate the selector expression to an integer
2. Check if the index is in valid range [1..N]
3. If valid, jump to the corresponding label
4. If invalid, fall through to the next statement

For ON GOSUB, the compiler also:
- Pushes the return address onto the GOSUB stack
- Generates a RETURN target label after the ON GOSUB statement

## Test Results

Both test files comprehensively validate the implementation:

```
✓ test_on_goto - 10 test cases, all PASS
✓ test_on_gosub - 11 test cases, all PASS
```

### Sample Output (test_on_goto)
```
=== ON GOTO Tests ===

PASS: ON GOTO index 1 reached target 1
PASS: ON GOTO index 2 reached target 2
PASS: ON GOTO index 3 reached target 3
PASS: ON GOTO with expression (5-3=2) works
PASS: ON GOTO with index 0 falls through
PASS: ON GOTO with out-of-range index falls through
PASS: ON GOTO with negative index falls through
PASS: ON GOTO with single target works
PASS: ON GOTO with many targets (index 5)
PASS: ON GOTO with MOD expression works

=== All ON GOTO Tests PASSED ===
```

### Sample Output (test_on_gosub)
```
=== ON GOSUB Tests ===

PASS: ON GOSUB index 1 works
PASS: ON GOSUB index 2 works
PASS: ON GOSUB index 3 works
PASS: ON GOSUB with expression (7-5=2) works
PASS: ON GOSUB with index 0 falls through
PASS: ON GOSUB with out-of-range index falls through
PASS: ON GOSUB with negative index falls through
PASS: ON GOSUB with single target works
PASS: Nested ON GOSUB works
PASS: ON GOSUB with many targets (index 4)
PASS: Execution continues after ON GOSUB RETURN

=== All ON GOSUB Tests PASSED ===
```

## Why These Tests Matter

### Legacy Code Support
ON GOTO and ON GOSUB are prevalent in vintage BASIC programs from the 1970s-1990s. Supporting them correctly is essential for:
- Porting old BASIC applications
- Running legacy business software
- Educational purposes (teaching BASIC history)
- Compatibility with classic BASIC dialects

### Common Use Cases in Legacy Code
```basic
REM Menu selection
PRINT "1. Load"
PRINT "2. Save"
PRINT "3. Exit"
INPUT CHOICE%
ON CHOICE% GOSUB 1000, 2000, 3000

REM State machine
ON STATE% GOTO 100, 200, 300, 400
```

### Modern Alternatives
While ON GOTO/GOSUB work correctly, modern structured programming uses:
- SELECT CASE for menu-driven logic
- Function pointers or function tables
- Proper subroutines and functions

However, the compiler must support these legacy constructs for backwards compatibility.

## Related Tests

- `tests/functions/test_gosub.bas` - Basic GOSUB/RETURN functionality
- `tests/conditionals/test_select_case.bas` - Modern alternative to ON GOTO
- `tests/loops/test_exit_statements.bas` - Other flow control structures

## Future Enhancements

Potential additional test coverage:
- ON GOSUB with deeply nested calls (stress test the return stack)
- ON GOTO/GOSUB with very large target lists (100+ targets)
- Error recovery when RETURN is called without GOSUB
- Mixing ON GOSUB with regular GOSUB
- ON ERROR GOTO (error handling, if implemented)

## References

- Classic BASIC specification
- QBasic/QuickBASIC documentation
- FasterBASIC parser implementation: `fsh/FasterBASICT/src/fasterbasic_parser.cpp` (lines 1240-1330)
- START_HERE.md - Developer guide for building and testing