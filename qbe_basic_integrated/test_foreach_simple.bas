REM Simple FOR EACH...IN Test
REM Test basic array iteration syntax

PRINT "Simple FOR EACH...IN Test"
PRINT

REM Test 1: Simple integer array iteration
PRINT "Test 1: Integer Array"
DIM nums%(3)
nums%(0) = 10
nums%(1) = 20
nums%(2) = 30

FOR EACH n IN nums%
    PRINT n
NEXT

PRINT
PRINT "Done!"
END
