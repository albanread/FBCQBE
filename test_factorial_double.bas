FUNCTION Factorial#(N#)
    IF N# <= 1 THEN
        Factorial# = 1
    ELSE
        Factorial# = N# * Factorial#(N# - 1)
    END IF
END FUNCTION

FOR i%=1 TO 100000000 STEP 1
	K#=Factorial#(19)
NEXT

PRINT "19! = "; K#

END
