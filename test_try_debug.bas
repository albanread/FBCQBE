REM Debug TRY/CATCH to see what's happening
PRINT "=== TRY/CATCH Debug Test ==="
PRINT ""
PRINT "Step 1: Before TRY"

TRY
    PRINT "Step 2: Inside TRY block"
    PRINT "Step 3: About to THROW 42"
    THROW 42
    PRINT "Step 4: After THROW (ERROR if you see this!)"
CATCH 42
    PRINT "Step 5: Inside CATCH block"
    PRINT "Step 6: Error code = "; ERR()
END TRY

PRINT "Step 7: After END TRY"
PRINT ""
PRINT "=== Test Complete ==="
END
