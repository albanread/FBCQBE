REM Simple FOR loop test without DIM
REM Tests FOR..NEXT with STEP and EXIT FOR

REM Test 1: Basic FOR loop counting 1 TO 5
PRINT "Test 1: FOR i = 1 TO 5"
FOR i% = 1 TO 5
    PRINT i%
NEXT i%
PRINT ""

REM Test 2: FOR loop with STEP 2
PRINT "Test 2: FOR i = 2 TO 10 STEP 2"
FOR i% = 2 TO 10 STEP 2
    PRINT i%
NEXT i%
PRINT ""

REM Test 3: FOR loop with expression in TO
PRINT "Test 3: FOR i = 1 TO 3+2"
FOR i% = 1 TO 3 + 2
    PRINT i%
NEXT i%
PRINT ""

REM Test 4: Count total in loop
PRINT "Test 4: Sum 1 TO 10"
total% = 0
FOR i% = 1 TO 10
    total% = total% + i%
NEXT i%
PRINT "Total = "; total%
PRINT ""

REM Test 5: EXIT FOR
PRINT "Test 5: EXIT FOR at i = 5"
FOR i% = 1 TO 10
    IF i% = 5 THEN EXIT FOR
    PRINT i%
NEXT i%
PRINT "Exited at i = 5"
PRINT ""

REM Test 6: Nested FOR loops
PRINT "Test 6: Nested loops"
FOR i% = 1 TO 3
    FOR j% = 1 TO 2
        PRINT "i="; i%; " j="; j%
    NEXT j%
NEXT i%
PRINT ""

PRINT "All tests completed!"
END
