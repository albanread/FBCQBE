' Minimal test case for GOSUB bug in multiline IF
PRINT "Bug Test: GOSUB in multiline IF block"
PRINT ""

LET test% = 1

IF test% = 1 THEN
    PRINT "1. Before GOSUB"
    GOSUB MySub
    PRINT "2. After GOSUB (THIS SHOULD PRINT BUT MIGHT NOT!)"
    PRINT "3. Still in IF block"
END IF

PRINT "4. After END IF"
END

MySub:
    PRINT "  Inside subroutine"
    RETURN
