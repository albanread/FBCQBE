' Test to compare FUNCTION vs SUB behavior
' This should help identify why FUNCTION works but SUB does not

PRINT "Testing FUNCTION vs SUB"
PRINT

' Test 1: Simple FUNCTION with return value
PRINT "Test 1: Calling FUNCTION AddTwo"
result% = AddTwo(5, 3)
PRINT "AddTwo(5, 3) = "; result%
PRINT

' Test 2: Simple SUB (void) that prints
PRINT "Test 2: Calling SUB PrintSum"
CALL PrintSum(5, 3)
PRINT

' Test 3: SUB that modifies a variable via PRINT
PRINT "Test 3: Calling SUB ShowMessage"
CALL ShowMessage("Hello from SUB")
PRINT

PRINT "All tests complete"
END

' FUNCTION that returns a value
FUNCTION AddTwo(a%, b%)
    AddTwo = a% + b%
END FUNCTION

' SUB that doesn't return a value (void)
SUB PrintSum(x%, y%)
    PRINT "Sum is: "; x% + y%
END SUB

' SUB with string parameter
SUB ShowMessage(msg$)
    PRINT "Message: "; msg$
END SUB
