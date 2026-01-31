' Test GOSUB/RETURN from within WHILE/WEND loop
PRINT "Testing GOSUB/RETURN within WHILE loop"
PRINT ""

LET counter& = 1

PRINT "Starting WHILE loop..."
WHILE counter& <= 5
    PRINT "Loop iteration "; counter&; " - before GOSUB"
    
    LET value& = counter& * 10
    GOSUB SubRoutine
    
    PRINT "Loop iteration "; counter&; " - after GOSUB, result="; result&
    PRINT ""
    
    LET counter& = counter& + 1
WEND

PRINT "Loop completed successfully!"
END

SubRoutine:
    PRINT "  In subroutine: value="; value&
    LET result& = value& * 2
    PRINT "  Returning from subroutine: result="; result&
    RETURN
