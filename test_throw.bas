PRINT "Before TRY"
TRY
    PRINT "In TRY - about to throw"
    THROW 11
    PRINT "After THROW (should not print)"
CATCH 11
    PRINT "Caught error 11!"
END TRY
PRINT "After TRY"
END
