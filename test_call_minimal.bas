REM Minimal CALL test
PRINT "Testing CALL"

CALL Hello()
CALL PrintNum(42)
CALL PrintNumAS(99)

PRINT "Done"
END

SUB Hello()
    PRINT "Hello from SUB"
END SUB

SUB PrintNum(n%)
    PRINT "Number: "; n%
END SUB

SUB PrintNumAS(x AS INTEGER)
    PRINT "Number AS: "; x
END SUB
