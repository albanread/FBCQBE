REM Test Array Access with Traditional FOR Loop
REM Verify we can read array elements correctly

PRINT "Array Access Test"
PRINT

REM Create double array
DIM nums#(2)
nums#(0) = 10.0
nums#(1) = 20.0
nums#(2) = 30.0

PRINT "Reading array with traditional FOR loop:"
FOR i# = 0 TO 2
    PRINT nums#(i#)
NEXT

PRINT
PRINT "Done!"
END
