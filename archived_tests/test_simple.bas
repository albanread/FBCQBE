PRINT "Test: Computing 2^11 mod 23"

LET answer& = 2048 MOD 23
PRINT "2^11 = 2048"
PRINT "2048 mod 23 = "; answer&

LET check& = 2047 MOD 23  
PRINT "2^11 - 1 = 2047"
PRINT "2047 mod 23 = "; check&

END
