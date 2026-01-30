REM Simple TRY/CATCH test with catch-all
PRINT "Before TRY"

TRY
    PRINT "Inside TRY block"
    THROW 99
    PRINT "After THROW (should not print)"
CATCH
    PRINT "Caught exception: "; ERR()
END TRY

PRINT "After TRY/CATCH"
END
