PRINT "Test nested GOSUB"
GOSUB Sub1
PRINT "Done"
END

Sub1:
    PRINT "  In Sub1"
    GOSUB Sub2
    PRINT "  Back in Sub1"
    RETURN

Sub2:
    PRINT "    In Sub2"
    RETURN
