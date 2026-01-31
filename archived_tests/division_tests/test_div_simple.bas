' Simple test for corrected signed division
' Returns results to verify correctness

FUNCTION TestPositive&() AS LONG
    LOCAL result&
    ' 7 \ 2 should be 3
    LET result& = 7 \ 2
    RETURN result&
END FUNCTION

FUNCTION TestNegative&() AS LONG
    LOCAL result&
    ' -7 \ 2 should be -3 (NOT -4)
    LET result& = -7 \ 2
    RETURN result&
END FUNCTION

FUNCTION TestNegative2&() AS LONG
    LOCAL result&
    ' -7 \ 4 should be -1 (NOT -2)
    LET result& = -7 \ 4
    RETURN result&
END FUNCTION

FUNCTION TestEdgeCase&() AS LONG
    LOCAL result&
    ' -1 \ 2 should be 0 (NOT -1)
    LET result& = -1 \ 2
    RETURN result&
END FUNCTION

' Main program
LET r1& = TestPositive&()
LET r2& = TestNegative&()
LET r3& = TestNegative2&()
LET r4& = TestEdgeCase&()

PRINT "7 \ 2 = "; r1&
PRINT "-7 \ 2 = "; r2&
PRINT "-7 \ 4 = "; r3&
PRINT "-1 \ 2 = "; r4&

' Verify results
IF r1& = 3 AND r2& = -3 AND r3& = -1 AND r4& = 0 THEN
    PRINT "SUCCESS: All division tests passed!"
ELSE
    PRINT "FAILURE: Some tests failed"
END IF

END
