' Minimal ON GOSUB test
PRINT "Testing ON GOSUB"
LET X% = 1
ON X% GOSUB 100, 200
PRINT "After ON GOSUB"
END

100 PRINT "Subroutine 1"
    RETURN

200 PRINT "Subroutine 2"
    RETURN
