REM Test nested FOR loops
PRINT "Nested loop test:"
FOR i% = 1 TO 3
    PRINT "Outer: i% = "; i%
    FOR j% = 1 TO 2
        PRINT "  Inner: j% = "; j%
    NEXT j%
NEXT i%
PRINT "Done"
END
