' Minimal test case
LET x% = 1
IF x% = 1 THEN
    PRINT "A"
    GOSUB Sub1
    PRINT "B"
END IF
PRINT "C"
END

Sub1:
    PRINT "S"
    RETURN
