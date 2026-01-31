' Simple test to find factor of M11
PRINT "Finding factor of 2^11 - 1"
PRINT ""

LET P& = 11
LET k& = 1
LET q& = 2 * k& * P& + 1

PRINT "Testing k="; k&; ", q="; q&

' Check if 23 is prime
LET is_prime% = 1
IF q& < 2 THEN
    is_prime% = 0
END IF
IF q& = 2 THEN
    is_prime% = 1
END IF
LET qmod& = q& MOD 2
IF qmod& = 0 THEN
    is_prime% = 0
END IF

IF is_prime% = 1 THEN
    LET i& = 3
    LET sqrtq& = 5
    WHILE i& <= sqrtq&
        LET imod& = q& MOD i&
        IF imod& = 0 THEN
            is_prime% = 0
            i& = sqrtq& + 1
        ELSE
            i& = i& + 2
        END IF
    WEND
END IF

PRINT "Is "; q&; " prime? "; is_prime%

' Now test modpow
LET result& = 1
LET base& = 2
LET exp& = P&
LET b& = base& MOD q&
LET e& = exp&

WHILE e& > 0
    LET bit& = e& MOD 2
    IF bit& = 1 THEN
        result& = (result& * b&) MOD q&
    END IF
    b& = (b& * b&) MOD q&
    e& = e& / 2
WEND

PRINT "2^"; P&; " mod "; q&; " = "; result&

IF result& = 1 THEN
    PRINT q&; " is a factor of 2^"; P&; " - 1"
END IF

END
