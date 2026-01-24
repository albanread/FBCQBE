REM Test CALL statement with subroutines
REM Simple test of calling SUBs with and without parameters

PRINT "Testing CALL statement"
PRINT ""

REM Test 1: Call SUB with no parameters
PRINT "Test 1: No parameters"
CALL Greet

REM Test 2: Call SUB with one parameter
PRINT ""
PRINT "Test 2: One parameter"
CALL PrintNumber(42)

REM Test 3: Call SUB with multiple parameters
PRINT ""
PRINT "Test 3: Multiple parameters"
CALL AddAndPrint(10, 20)

REM Test 4: Call SUB with different types
PRINT ""
PRINT "Test 4: Mixed types"
CALL MixedTypes(5, 3.14, "Hello")

REM Test 5: Nested SUB calls
PRINT ""
PRINT "Test 5: Nested calls"
CALL Outer(7)

PRINT ""
PRINT "All tests completed!"
END

SUB Greet()
    PRINT "  Hello from Greet!"
END SUB

SUB PrintNumber(n AS INTEGER)
    PRINT "  Number is: "; n
END SUB

SUB AddAndPrint(a AS INTEGER, b AS INTEGER)
    DIM result AS INTEGER
    result = a + b
    PRINT "  "; a; " + "; b; " = "; result
END SUB

SUB MixedTypes(x AS INTEGER, y AS DOUBLE, s AS STRING)
    PRINT "  Integer: "; x
    PRINT "  Double: "; y
    PRINT "  String: "; s
END SUB

SUB Outer(num AS INTEGER)
    PRINT "  Outer called with "; num
    CALL Inner(num * 2)
END SUB

SUB Inner(num AS INTEGER)
    PRINT "    Inner called with "; num
END SUB
