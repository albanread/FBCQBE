REM Minimal FOR EACH...IN Test with doubles

PRINT "FOR EACH Test with Double Array"
PRINT

REM Create double array
DIM nums#(2)
nums#(0) = 10.0
nums#(1) = 20.0
nums#(2) = 30.0

PRINT "Array contents via FOR EACH:"
FOR EACH n IN nums#
    PRINT n
NEXT

PRINT
PRINT "Done!"
END
