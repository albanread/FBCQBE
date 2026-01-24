REM Test modifying FOR loop index to exit early
REM Classic BASIC trick: set i = limit to exit on next iteration

PRINT "Test 1: Normal loop 1 TO 5"
FOR i% = 1 TO 5
    PRINT "i% = "; i%
NEXT i%
PRINT ""

PRINT "Test 2: Modify index to exit early"
FOR i% = 1 TO 10
    PRINT "i% = "; i%
    IF i% = 3 THEN
        PRINT "Setting i% = 10 to exit loop"
        i% = 10
    END IF
NEXT i%
PRINT "After loop, i% = "; i%
PRINT ""

PRINT "Test 3: Modify index to skip iterations"
FOR i% = 1 TO 10
    PRINT "i% = "; i%
    IF i% = 3 THEN
        PRINT "Jumping to i% = 7"
        i% = 7
    END IF
NEXT i%
PRINT ""

PRINT "Test 4: Modify index beyond limit"
FOR i% = 1 TO 5
    PRINT "i% = "; i%
    IF i% = 3 THEN
        PRINT "Setting i% = 100 (way past limit)"
        i% = 100
    END IF
NEXT i%
PRINT "After loop, i% = "; i%
PRINT ""

PRINT "All tests completed!"
END
