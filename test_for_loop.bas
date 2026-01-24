REM Test FOR loops with STEP and EXIT FOR

DIM total AS INTEGER
DIM i AS INTEGER
DIM j AS INTEGER

REM Test 1: Basic FOR loop counting up
PRINT "Test 1: Basic FOR loop 1 TO 5"
total = 0
FOR i = 1 TO 5
    PRINT "i = "; i
    total = total + i
NEXT i
PRINT "Total: "; total
PRINT ""

REM Test 2: FOR loop with STEP 2
PRINT "Test 2: FOR loop 1 TO 10 STEP 2"
total = 0
FOR i = 1 TO 10 STEP 2
    PRINT "i = "; i
    total = total + i
NEXT i
PRINT "Total: "; total
PRINT ""

REM Test 3: FOR loop with negative STEP
PRINT "Test 3: FOR loop 10 TO 1 STEP -1"
total = 0
FOR i = 10 TO 1 STEP -1
    PRINT "i = "; i
    total = total + i
NEXT i
PRINT "Total: "; total
PRINT ""

REM Test 4: EXIT FOR
PRINT "Test 4: EXIT FOR when i = 5"
total = 0
FOR i = 1 TO 10
    IF i = 5 THEN
        PRINT "Exiting at i = 5"
        EXIT FOR
    END IF
    PRINT "i = "; i
    total = total + i
NEXT i
PRINT "Total after EXIT: "; total
PRINT ""

REM Test 5: Nested FOR loops
PRINT "Test 5: Nested FOR loops"
total = 0
FOR i = 1 TO 3
    FOR j = 1 TO 3
        PRINT "i = "; i; ", j = "; j
        total = total + (i * 10 + j)
    NEXT j
NEXT i
PRINT "Total: "; total
PRINT ""

REM Test 6: FOR loop with expression in TO
PRINT "Test 6: FOR loop 1 TO 2+3"
total = 0
FOR i = 1 TO 2 + 3
    PRINT "i = "; i
    total = total + i
NEXT i
PRINT "Total: "; total
PRINT ""

PRINT "All FOR loop tests completed!"
END
