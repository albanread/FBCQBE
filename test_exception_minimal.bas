REM Minimal Exception Handling Test
REM Tests basic TRY/CATCH/THROW functionality

PRINT "Exception Handling Test"
PRINT

REM Test 1: Simple TRY/CATCH (no exception)
PRINT "Test 1: TRY/CATCH without exception"
TRY
    PRINT "  Inside TRY block"
    x% = 42
    PRINT "  x% = "; x%
CATCH 11
    PRINT "  ERROR: Should not catch"
END TRY
PRINT "  After TRY - PASS"
PRINT

REM Test 2: TRY/FINALLY (no exception)
PRINT "Test 2: TRY/FINALLY without exception"
TRY
    PRINT "  Inside TRY block"
    y% = 100
FINALLY
    PRINT "  FINALLY executed"
END TRY
PRINT "  After TRY - PASS"
PRINT

PRINT "All tests completed successfully!"
END
