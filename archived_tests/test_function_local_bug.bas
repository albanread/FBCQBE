' Minimal test case for LOCAL variable type bug in functions
' This isolates the "temporary is assigned with multiple types" error

PRINT "Testing function with LOCAL LONG variables..."

' Test 1: Simple function with LOCAL LONG
LET result1& = TestFunction1&(5&)
PRINT "TestFunction1(5) = "; result1&

' Test 2: Function with multiple LOCAL LONG variables
LET result2& = TestFunction2&(10&, 3&)
PRINT "TestFunction2(10, 3) = "; result2&

PRINT "All tests completed!"
END

' Simple function with one LOCAL LONG variable
FUNCTION TestFunction1&(n AS LONG) AS LONG
    LOCAL temp&
    LET temp& = n * 2
    RETURN temp&
END FUNCTION

' Function with multiple LOCAL LONG variables
FUNCTION TestFunction2&(a AS LONG, b AS LONG) AS LONG
    LOCAL result&
    LOCAL x&
    LOCAL y&

    LET x& = a MOD b
    LET y& = a * b
    LET result& = x& + y&

    RETURN result&
END FUNCTION
