' Test GOSUB/RETURN from within multiline IF block
PRINT "Testing GOSUB/RETURN in multiline IF block"
PRINT ""

LET x& = 20

PRINT "Testing with x="; x&
PRINT "Checking if x is divisible by 10..."

LET modval& = x& MOD 10
IF modval& = 0 THEN
    PRINT "  Before GOSUB"
    GOSUB DoSomething  
    PRINT "  After GOSUB - result="; answer&
    PRINT "  This should print!"
END IF

PRINT "Done!"
END

DoSomething:
    PRINT "    In subroutine"
    LET answer& = 42
    PRINT "    Returning with answer="; answer&
    RETURN
