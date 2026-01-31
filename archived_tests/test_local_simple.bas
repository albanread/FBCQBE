' Simplest possible test for LOCAL variable bug
PRINT "Test: LOCAL variable with integer literal assignment"

LET result& = TestSimple&(10&)
PRINT "Result: "; result&

END

FUNCTION TestSimple&(n AS LONG) AS LONG
    LOCAL x&
    LET x& = 5
    RETURN x&
END FUNCTION
