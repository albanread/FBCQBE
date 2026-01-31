' Test program for corrected signed integer division
' Tests that division by powers of 2 works correctly for negative numbers

PRINT "Testing corrected signed division optimization..."
PRINT ""

' Test positive numbers
PRINT "=== Positive Numbers ==="
LET x& = 7
PRINT "7 \ 2 = "; x& \ 2; " (expected: 3)"
PRINT "7 \ 4 = "; x& \ 4; " (expected: 1)"
PRINT "7 \ 8 = "; x& \ 8; " (expected: 0)"
PRINT ""

LET x& = 100
PRINT "100 \ 2 = "; x& \ 2; " (expected: 50)"
PRINT "100 \ 4 = "; x& \ 4; " (expected: 25)"
PRINT "100 \ 8 = "; x& \ 8; " (expected: 12)"
PRINT ""

' Test negative numbers (the critical test!)
PRINT "=== Negative Numbers ==="
LET x& = -7
PRINT "-7 \ 2 = "; x& \ 2; " (expected: -3, NOT -4)"
PRINT "-7 \ 4 = "; x& \ 4; " (expected: -1, NOT -2)"
PRINT "-7 \ 8 = "; x& \ 8; " (expected: 0, NOT -1)"
PRINT ""

LET x& = -8
PRINT "-8 \ 2 = "; x& \ 2; " (expected: -4)"
PRINT "-8 \ 4 = "; x& \ 4; " (expected: -2)"
PRINT "-8 \ 8 = "; x& \ 8; " (expected: -1)"
PRINT ""

LET x& = -100
PRINT "-100 \ 2 = "; x& \ 2; " (expected: -50)"
PRINT "-100 \ 4 = "; x& \ 4; " (expected: -25)"
PRINT "-100 \ 8 = "; x& \ 8; " (expected: -12, NOT -13)"
PRINT ""

' Test edge cases
PRINT "=== Edge Cases ==="
LET x& = -1
PRINT "-1 \ 2 = "; x& \ 2; " (expected: 0, NOT -1)"
LET x& = -2
PRINT "-2 \ 2 = "; x& \ 2; " (expected: -1)"
LET x& = -3
PRINT "-3 \ 2 = "; x& \ 2; " (expected: -1, NOT -2)"
PRINT ""

' Test in a function
LET result& = TestDiv&(-15, 4)
PRINT "TestDiv(-15, 4) = "; result&; " (expected: -3)"
PRINT ""

PRINT "All tests completed!"
END

FUNCTION TestDiv&(dividend AS LONG, divisor AS LONG) AS LONG
    ' Test division in a function context
    RETURN dividend \ divisor
END FUNCTION
