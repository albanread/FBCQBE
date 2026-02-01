' Minimal test for SUB and FUNCTION CFG support
' No loops, just basic control flow

PRINT "Testing SUB and FUNCTION"

' Call a SUB
CALL TestSub(5)

' Call a FUNCTION
result% = Add%(3, 7)
PRINT "Result = "; result%

END

' Simple SUB
SUB TestSub(x%)
    PRINT "In SUB, x = "; x%
END SUB

' Simple FUNCTION
FUNCTION Add%(a%, b%)
    Add% = a% + b%
END FUNCTION

' FUNCTION with IF
FUNCTION Max%(x%, y%)
    IF x% > y% THEN
        Max% = x%
    ELSE
        Max% = y%
    END IF
END FUNCTION
