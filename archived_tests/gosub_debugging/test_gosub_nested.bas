' Test GOSUB/RETURN from within nested IF inside WHILE loop
PRINT "Testing GOSUB/RETURN with nested IF in WHILE loop"
PRINT ""

LET counter& = 1

WHILE counter& <= 10
    LET value& = counter& * 10
    
    PRINT "Iteration "; counter&; ": value="; value&
    
    ' Check if value is divisible by 20
    LET mod20& = value& MOD 20
    IF mod20& = 0 THEN
        PRINT "  value is divisible by 20, calling subroutine..."
        
        LET testval& = value&
        GOSUB ProcessValue
        
        PRINT "  Back from subroutine: processed="; processed&
        
        IF processed& > 100 THEN
            PRINT "  Processed value is large!"
        END IF
    END IF
    
    LET counter& = counter& + 1
WEND

PRINT ""
PRINT "All iterations completed!"
END

ProcessValue:
    PRINT "    In ProcessValue: testval="; testval&
    LET processed& = testval& * 5
    PRINT "    Calculated processed="; processed&
    RETURN
