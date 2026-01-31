' Test isPrime function
PRINT "Testing isPrime function"

LET q& = 23
GOSUB IsPrime
PRINT "Is 23 prime? "; is_prime%

LET q& = 45
GOSUB IsPrime  
PRINT "Is 45 prime? "; is_prime%

LET q& = 89
GOSUB IsPrime
PRINT "Is 89 prime? "; is_prime%

END

IsPrime:
    is_prime% = 0
    IF q& < 2 THEN
        RETURN
    END IF
    IF q& = 2 THEN
        is_prime% = 1
        RETURN
    END IF
    LET qmod2& = q& MOD 2
    IF qmod2& = 0 THEN
        RETURN
    END IF
    LET i& = 3
    LET sqrt_q# = SQR(q&)
    LET sqrt_q& = INT(sqrt_q#)
    LET sqrt_q& = sqrt_q& + 1
    WHILE i& <= sqrt_q&
        LET imod& = q& MOD i&
        IF imod& = 0 THEN
            RETURN
        END IF
        LET i& = i& + 2
    WEND
    is_prime% = 1
    RETURN
