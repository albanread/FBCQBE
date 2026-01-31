# ON GOTO and ON GOSUB Implementation in FasterBASIC QBE Backend

## Overview

The `ON` statements in FasterBASIC allow conditional branching based on the value of an integer expression. They are implemented in the QBE (QBE Intermediate Language) backend using chained conditional jumps.

## Syntax

### ON GOTO
```
ON expression GOTO target1, target2, ..., targetN
```

- `expression`: An integer expression (1-based index)
- `target1, target2, ...`: Line numbers or labels to jump to
- If expression == 1, goto target1; if == 2, goto target2; etc.
- If out of range, continues to the next statement

### ON GOSUB
```
ON expression GOSUB target1, target2, ..., targetN
```

- Similar to ON GOTO, but calls subroutines
- Pushes return address to stack before jumping
- Returns via `RETURN` statement, which pops from stack and resumes execution

## Code Generation Details

### Common Implementation
1. **Selector Evaluation**: The expression is evaluated and converted to integer if needed.
2. **Dispatch Logic**: Uses chained `jnz` (jump if not zero) instructions in QBE's 3-operand format:
   ```
   %t1 =w ceqw selector, 1
   jnz %t1, @target1, @next_check
   ```
3. **Fallthrough**: If selector is out of range, jumps to fallthrough block (next statement).

### ON GOTO Specifics
- Terminates the current block (no fallthrough unless out of range)
- No stack manipulation

### ON GOSUB Specifics
- **Return Stack**: Uses global `$return_stack` (16-entry array) and `$return_sp` (stack pointer)
- **Push Return Address**: Before dispatch, pushes the block ID of the statement after ON GOSUB
- **RETURN Handling**: Pops block ID and dispatches back using chained comparisons
- **Stack Overflow/Underflow**: Falls back to program exit if stack issues occur

### QBE IL Structure
- Generated as SSA (Static Single Assignment) form
- Uses temporary variables for comparisons
- Labels for targets and fallthrough points

## Examples

### ON GOTO Example
```basic
10 I = 2
20 ON I GOTO 100, 200, 300
30 PRINT "Out of range"
40 END
100 PRINT "Branch 1": END
200 PRINT "Branch 2": END
300 PRINT "Branch 3": END
```
Output: "Branch 2"

### ON GOSUB Example
```basic
10 FOR I = 1 TO 3
20 ON I GOSUB sub1, sub2, sub3
30 PRINT "Returned from branch"; I
40 NEXT I
50 END

sub1: PRINT "Sub 1": RETURN
sub2: PRINT "Sub 2": RETURN
sub3: PRINT "Sub 3": RETURN
```
Output:
```
Sub 1
Returned from branch 1
Sub 2
Returned from branch 2
Sub 3
Returned from branch 3
```

## Testing

- **ON GOTO**: Verified with `test_on_statements.bas`
- **ON GOSUB**: Verified with `test_on_gosub_sequence.bas`
- Run via: `./run_basic.sh ../test_file.bas`

## Implementation Files

- **Code Generation**: `fsh/FasterBASICT/src/codegen/qbe_codegen_statements.cpp`
  - `emitOnGoto()` and `emitOnGosub()` functions
- **CFG Building**: `fsh/FasterBASICT/src/fasterbasic_cfg.cpp`
  - Handles control flow edges for ON statements
- **Helpers**: `fsh/FasterBASICT/src/codegen/qbe_codegen_helpers.cpp`
  - `getFallthroughBlock()` for resolving return targets

## Notes

- Selector is 1-based (ON 1 GOSUB targets first item)
- Supports both line numbers and labels as targets
- Runtime stack prevents infinite recursion (16 levels)
- CFG-aware fallthrough resolution ensures correct return blocks</content>
<parameter name="filePath">/Users/oberon/FBFAM/FBCQBE/ON_STATEMENTS_DOCUMENTATION.md