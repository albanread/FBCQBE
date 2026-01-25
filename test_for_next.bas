FUNCTION Factorial%(N%)
    IF N% <= 1 THEN
        Factorial% = 1
    ELSE
        Factorial% = N% * Factorial%(N% - 1)
    END IF
END FUNCTION

PRINT "5! = "; Factorial%(5)

FOR i%=1 TO 100 STEP 1
NEXT
END
