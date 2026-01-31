PRINT "Test: Is 23 a factor of 2^11 - 1?"
PRINT ""

LET P& = 11
LET q& = 23

' Test modpow
GOSUB ModPow
PRINT "2^"; P&; " mod "; q&; " = "; result&

IF result& = 1 THEN
    PRINT "Yes! 23 is a factor of 2^11 - 1"
ELSE
    PRINT "No, 23 is not a factor"
END IF

END

ModPow:
    LET base& = 2
    LET exp& = P&
    LET modulus& = q&
    
    LET result& = 1
    LET b& = base& MOD modulus&
    LET e& = exp&
    
    WHILE e& > 0
        LET bit& = e& MOD 2
        IF bit& = 1 THEN
            LET result& = (result& * b&) MOD modulus&
        END IF
        LET b& = (b& * b&) MOD modulus&
        LET e& = e& / 2
    WEND
    RETURN
