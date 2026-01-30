REM Simple TRY/CATCH test with numeric error codes
PRINT "Before TRY"

TRY
    PRINT "Inside TRY block"
    THROW 42
    PRINT "After THROW (should not print)"
CATCH 42
    PRINT "Caught exception 42: "; ERR()
END TRY

PRINT "After TRY/CATCH"
END
