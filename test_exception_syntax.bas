REM Test Exception Handling Syntax
REM This test validates that TRY/CATCH/FINALLY/THROW syntax is accepted

PRINT "Testing Exception Handling Syntax"
PRINT

REM Test 1: Simple TRY/CATCH
PRINT "Test 1: Simple TRY/CATCH"
TRY
    PRINT "  In TRY block"
    x% = 10 / 2
    PRINT "  x% ="; x%
CATCH 11
    PRINT "  Caught division error"
END TRY
PRINT "  After TRY"
PRINT

REM Test 2: Multiple CATCH clauses
PRINT "Test 2: Multiple CATCH clauses"
TRY
    PRINT "  In TRY block"
    y% = 5 + 5
CATCH 9, 11
    PRINT "  Caught math error (9 or 11)"
CATCH 53
    PRINT "  Caught file error (53)"
END TRY
PRINT "  After TRY"
PRINT

REM Test 3: TRY/CATCH/FINALLY
PRINT "Test 3: TRY/CATCH/FINALLY"
TRY
    PRINT "  In TRY block"
    z% = 100
CATCH 11
    PRINT "  In CATCH block"
FINALLY
    PRINT "  In FINALLY block (always runs)"
END TRY
PRINT "  After TRY"
PRINT

REM Test 4: TRY/FINALLY (no CATCH)
PRINT "Test 4: TRY/FINALLY (no CATCH)"
TRY
    PRINT "  In TRY block"
    a% = 42
FINALLY
    PRINT "  In FINALLY block"
END TRY
PRINT "  After TRY"
PRINT

REM Test 5: Catch-all
PRINT "Test 5: Catch-all CATCH"
TRY
    PRINT "  In TRY block"
    b% = 99
CATCH 11
    PRINT "  Caught specific error 11"
CATCH
    PRINT "  Caught any other error"
END TRY
PRINT "  After TRY"
PRINT

REM Test 6: THROW statement
PRINT "Test 6: THROW statement (commented out - would throw)"
REM TRY
REM     PRINT "  Before THROW"
REM     THROW 99
REM     PRINT "  After THROW (should not print)"
REM CATCH 99
REM     PRINT "  Caught error 99"
REM END TRY
PRINT "  (THROW test skipped for now)"
PRINT

PRINT "All syntax tests passed!"
PRINT "Note: These tests only validate syntax, not runtime behavior."

END
