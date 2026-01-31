' Test runtime signed division (not constant-foldable)

FUNCTION DivideBy2&(n AS LONG) AS LONG
    RETURN n \ 2
END FUNCTION

FUNCTION DivideBy4&(n AS LONG) AS LONG
    RETURN n \ 4
END FUNCTION

FUNCTION DivideBy8&(n AS LONG) AS LONG
    RETURN n \ 8
END FUNCTION

' Test with actual runtime values
LET num& = -7
LET r1& = DivideBy2&(num&)
LET r2& = DivideBy4&(num&)
LET r3& = DivideBy8&(num&)

PRINT "Testing negative number: -7"
PRINT "  -7 div 2 = "; r1&; " (expected: -3)"
PRINT "  -7 div 4 = "; r2&; " (expected: -1)"
PRINT "  -7 div 8 = "; r3&; " (expected: 0)"
PRINT ""

LET num& = 7
LET r1& = DivideBy2&(num&)
LET r2& = DivideBy4&(num&)
LET r3& = DivideBy8&(num&)

PRINT "Testing positive number: 7"
PRINT "  7 div 2 = "; r1&; " (expected: 3)"
PRINT "  7 div 4 = "; r2&; " (expected: 1)"
PRINT "  7 div 8 = "; r3&; " (expected: 0)"
PRINT ""

LET num& = -100
LET r1& = DivideBy8&(num&)
PRINT "Testing -100 div 8 = "; r1&; " (expected: -12)"

END
