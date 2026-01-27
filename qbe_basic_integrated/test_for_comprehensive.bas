REM Comprehensive FOR loop test
REM Tests various FOR loop features

PRINT "=== FOR Loop Comprehensive Test ==="
PRINT

REM Test 1: Simple ascending loop
PRINT "Test 1: Simple ascending (1 TO 5)"
FOR i = 1 TO 5
    PRINT i;
NEXT i
PRINT
PRINT

REM Test 2: Loop with STEP
PRINT "Test 2: Loop with STEP 2"
FOR j = 0 TO 10 STEP 2
    PRINT j;
NEXT j
PRINT
PRINT

REM Test 3: Descending loop
PRINT "Test 3: Descending loop (10 TO 1 STEP -1)"
FOR k = 10 TO 1 STEP -1
    PRINT k;
NEXT k
PRINT
PRINT

REM Test 4: Loop with type suffixes
PRINT "Test 4: Loop with % suffix"
FOR n% = 1 TO 3
    PRINT "n% = "; n%
NEXT n%
PRINT

REM Test 5: Nested loops
PRINT "Test 5: Nested loops"
FOR outer% = 1 TO 2
    PRINT "Outer: "; outer%
    FOR inner% = 1 TO 3
        PRINT "  Inner: "; inner%
    NEXT inner%
NEXT outer%
PRINT

REM Test 6: Loop with calculations
PRINT "Test 6: Sum calculation"
DIM sum AS INTEGER
sum = 0
FOR x = 1 TO 10
    sum = sum + x
NEXT x
PRINT "Sum of 1 to 10 = "; sum
PRINT

REM Test 7: Triple nested loop
PRINT "Test 7: Triple nested"
FOR a% = 1 TO 2
    FOR b% = 1 TO 2
        FOR c% = 1 TO 2
            PRINT a%; ","; b%; ","; c%
        NEXT c%
    NEXT b%
NEXT a%
PRINT

PRINT "=== All FOR loop tests complete ==="
END
