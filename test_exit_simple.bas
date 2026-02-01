REM Simple EXIT FOR test
DIM i AS INTEGER
DIM count AS INTEGER

count = 0
FOR i = 1 TO 10
    count = count + 1
    PRINT "i="; i; " count="; count
    IF i = 5 THEN EXIT FOR
NEXT i

PRINT "After loop: i="; i; " count="; count
PRINT "Expected: i=5 count=5"

END
