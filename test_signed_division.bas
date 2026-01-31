' Test signed division with bias correction
' This tests that integer division by powers of 2 works correctly
' for both positive and negative numbers

DIM results%(10)
DIM expected%(10)
DIM test_count%
DIM pass_count%

test_count% = 0
pass_count% = 0

' Test 1: Positive dividend
test_count% = test_count% + 1
results%(1) = TestDiv1%()
expected%(1) = 3
IF results%(1) = expected%(1) THEN pass_count% = pass_count% + 1

' Test 2: Negative dividend (critical!)
test_count% = test_count% + 1
results%(2) = TestDiv2%()
expected%(2) = -3
IF results%(2) = expected%(2) THEN pass_count% = pass_count% + 1

' Test 3: Negative dividend, divide by 4
test_count% = test_count% + 1
results%(3) = TestDiv3%()
expected%(3) = -1
IF results%(3) = expected%(3) THEN pass_count% = pass_count% + 1

' Test 4: Edge case -1 \ 2
test_count% = test_count% + 1
results%(4) = TestDiv4%()
expected%(4) = 0
IF results%(4) = expected%(4) THEN pass_count% = pass_count% + 1

' Test 5: Exact division negative
test_count% = test_count% + 1
results%(5) = TestDiv5%()
expected%(5) = -4
IF results%(5) = expected%(5) THEN pass_count% = pass_count% + 1

' Test 6: Large positive
test_count% = test_count% + 1
results%(6) = TestDiv6%()
expected%(6) = 12
IF results%(6) = expected%(6) THEN pass_count% = pass_count% + 1

' Test 7: Large negative (non-exact)
test_count% = test_count% + 1
results%(7) = TestDiv7%()
expected%(7) = -12
IF results%(7) = expected%(7) THEN pass_count% = pass_count% + 1

PRINT "========================================"
PRINT "Signed Division Test Results"
PRINT "========================================"
PRINT "Tests passed: "; pass_count%; " of "; test_count%
PRINT ""
PRINT "Test 1:  7 \\ 2 = "; results%(1); " (expected "; expected%(1); ")"
PRINT "Test 2: -7 \\ 2 = "; results%(2); " (expected "; expected%(2); ")"
PRINT "Test 3: -7 \\ 4 = "; results%(3); " (expected "; expected%(3); ")"
PRINT "Test 4: -1 \\ 2 = "; results%(4); " (expected "; expected%(4); ")"
PRINT "Test 5: -8 \\ 2 = "; results%(5); " (expected "; expected%(5); ")"
PRINT "Test 6: 100 \\ 8 = "; results%(6); " (expected "; expected%(6); ")"
PRINT "Test 7: -100 \\ 8 = "; results%(7); " (expected "; expected%(7); ")"
PRINT ""

IF pass_count% = test_count% THEN
    PRINT "SUCCESS: All tests passed!"
ELSE
    PRINT "FAILURE: "; test_count% - pass_count%; " tests failed"
END IF

END

FUNCTION TestDiv1%() AS INTEGER
    RETURN 7 \ 2
END FUNCTION

FUNCTION TestDiv2%() AS INTEGER
    LOCAL x%
    x% = -7
    RETURN x% \ 2
END FUNCTION

FUNCTION TestDiv3%() AS INTEGER
    LOCAL x%
    x% = -7
    RETURN x% \ 4
END FUNCTION

FUNCTION TestDiv4%() AS INTEGER
    LOCAL x%
    x% = -1
    RETURN x% \ 2
END FUNCTION

FUNCTION TestDiv5%() AS INTEGER
    LOCAL x%
    x% = -8
    RETURN x% \ 2
END FUNCTION

FUNCTION TestDiv6%() AS INTEGER
    RETURN 100 \ 8
END FUNCTION

FUNCTION TestDiv7%() AS INTEGER
    LOCAL x%
    x% = -100
    RETURN x% \ 8
END FUNCTION
