REM Ultra-minimal test - array access after WHILE only (no IF)

DIM arr(100) AS INT
DIM i AS INT

REM Initialize array
i = 1
WHILE i <= 10
    arr(i) = i * 10
    i = i + 1
WEND

REM Direct array access after WHILE - no IF statement
PRINT "arr(1) = "; arr(1)
PRINT "arr(2) = "; arr(2)
PRINT "arr(3) = "; arr(3)
