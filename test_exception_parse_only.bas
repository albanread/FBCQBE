REM Test Exception Handling - Parse and Semantic Analysis Only
REM This validates that the parser and semantic analyzer accept exception syntax

PRINT "Exception Handling Syntax Test"

REM Test 1: Minimal TRY/CATCH
TRY
    PRINT "Hello"
CATCH 11
    PRINT "Error"
END TRY

REM Test 2: Multiple error codes in one CATCH
TRY
    x% = 5
CATCH 9, 11, 13
    PRINT "Math error"
END TRY

REM Test 3: Multiple CATCH clauses
TRY
    y% = 10
CATCH 9
    PRINT "Subscript error"
CATCH 11
    PRINT "Division by zero"
CATCH 53
    PRINT "File not found"
END TRY

REM Test 4: TRY/FINALLY without CATCH
TRY
    z% = 20
FINALLY
    PRINT "Cleanup"
END TRY

REM Test 5: TRY/CATCH/FINALLY
TRY
    a% = 30
CATCH 11
    PRINT "Error caught"
FINALLY
    PRINT "Always runs"
END TRY

REM Test 6: Catch-all must be last
TRY
    b% = 40
CATCH 11
    PRINT "Specific"
CATCH
    PRINT "Catch all others"
END TRY

REM Test 7: Nested TRY blocks
TRY
    c% = 50
    TRY
        d% = 60
    CATCH 11
        PRINT "Inner error"
    END TRY
CATCH 9
    PRINT "Outer error"
END TRY

PRINT "Syntax validation complete!"
END
