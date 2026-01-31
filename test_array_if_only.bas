REM Test with IF but no WHILE loop

DIM arr(100) AS INT

REM Direct initialization (no loop)
arr(1) = 10
arr(2) = 20
arr(3) = 30

REM IF statement that accesses array
IF arr(1) = 10 THEN
    PRINT "arr(1) is correct: "; arr(1)
END IF

REM Array access after IF
PRINT "After IF, arr(1) = "; arr(1)
PRINT "arr(2) = "; arr(2)
PRINT "arr(3) = "; arr(3)
