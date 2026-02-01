' Test program for SUB and FUNCTION CFG support
' This demonstrates that the CFG v2 builder can now handle
' SUB and FUNCTION definitions with their own separate CFGs

PRINT "Testing SUB and FUNCTION CFG support"

' Call a SUB
CALL MySub(5, 10)

' Call a FUNCTION
result% = AddNumbers%(3, 7)
PRINT "AddNumbers(3, 7) = "; result%

' Another function call
area# = CalculateArea#(4.5, 6.2)
PRINT "CalculateArea(4.5, 6.2) = "; area#

END

' SUB definition with parameters
SUB MySub(x%, y%)
    LOCAL total%
    PRINT "MySub called with x="; x%; " y="; y%
    total% = x% + y%
    PRINT "Sum = "; total%
END SUB

' FUNCTION definition with return value
FUNCTION AddNumbers%(a%, b%)
    AddNumbers% = a% + b%
END FUNCTION

' Another FUNCTION with different type and control flow
FUNCTION CalculateArea#(width#, height#)
    IF width# <= 0 THEN
        CalculateArea# = 0
    ELSE
        IF height# <= 0 THEN
            CalculateArea# = 0
        ELSE
            CalculateArea# = width# * height#
        END IF
    END IF
END FUNCTION

' Simple factorial without loops
FUNCTION SimpleCalc%(n%)
    IF n% <= 1 THEN
        SimpleCalc% = 1
    ELSE
        SimpleCalc% = n% * 2
    END IF
END FUNCTION
