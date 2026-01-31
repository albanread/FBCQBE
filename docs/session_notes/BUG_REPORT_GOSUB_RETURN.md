# BUG REPORT: GOSUB/RETURN in Multiline IF Blocks

## Summary
When `GOSUB` is called from within a multiline `IF...END IF` block, the `RETURN` statement incorrectly jumps to after the `END IF` instead of returning to the statement immediately following the `GOSUB`.

## Expected Behavior
```
IF condition THEN
    PRINT "Before GOSUB"
    GOSUB MySub
    PRINT "After GOSUB"  ← Should execute this
END IF
PRINT "After END IF"

MySub:
    PRINT "In subroutine"
    RETURN  ← Should return to "After GOSUB" line
```

Expected output:
```
Before GOSUB
In subroutine
After GOSUB
After END IF
```

## Actual Behavior
Actual output:
```
Before GOSUB
In subroutine
After END IF
```

The "After GOSUB" line never executes!

## Test Case
See: `test_gosub_return_bug.bas`

Run with:
```bash
./qbe_basic -o test_gosub_return_bug test_gosub_return_bug.bas
./test_gosub_return_bug
```

## Impact
This bug prevents proper use of subroutines within multiline IF blocks, which is a common programming pattern. It particularly affects the Mersenne factors program where we need to:
1. Check if a number passes mod 8 test (IF)
2. Call GOSUB to check if it's prime
3. Check the result (still in the IF block)
4. Call GOSUB to test if it's a factor
5. Check that result (still in the IF block)

## Workaround
Avoid calling GOSUB from within multiline IF blocks. Instead:
- Use single-line IF statements
- Restructure code to call GOSUB outside of IF blocks
- Use flag variables and check them after the IF block

## Files Affected
- `tests/rosetta/mersenne_factors.bas` - Cannot complete Rosetta Code challenge due to this bug
