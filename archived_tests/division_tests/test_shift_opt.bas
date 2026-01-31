' Test program for power-of-2 shift optimization
' Tests that division and multiplication by powers of 2
' are optimized to shift instructions

PRINT "Testing shift optimizations..."
PRINT ""

' Test multiplication by powers of 2
LET x& = 100
PRINT "x = "; x&
PRINT "x * 2 = "; x& * 2
PRINT "x * 4 = "; x& * 4
PRINT "x * 8 = "; x& * 8
PRINT "x * 16 = "; x& * 16
PRINT ""

' Test integer division by powers of 2
LET y& = 1000
PRINT "y = "; y&
PRINT "y \ 2 = "; y& \ 2
PRINT "y \ 4 = "; y& \ 4
PRINT "y \ 8 = "; y& \ 8
PRINT "y \ 16 = "; y& \ 16
PRINT ""

' Test in a function with LOCAL variables
LET result& = TestShifts&(64)
PRINT "TestShifts(64) = "; result&
PRINT ""

' Test modular power shift optimization
LET n& = 929
LET e& = n&
PRINT "Simulating e = e / 2 loop:"
LET count% = 0
WHILE e& > 0 AND count% < 10
    PRINT "  e = "; e&
    LET e& = e& \ 2
    LET count% = count% + 1
WEND

PRINT ""
PRINT "All tests completed!"
END

FUNCTION TestShifts&(value AS LONG) AS LONG
    LOCAL temp&

    ' Multiple by 8, then divide by 4
    ' Should generate: shl by 3, then sar by 2
    LET temp& = value * 8
    LET temp& = temp& \ 4

    RETURN temp&
END FUNCTION
