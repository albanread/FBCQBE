' Test SUB with parameters vs no parameters

PRINT "Test 1: SUB with parameters (using CALL)"
CALL PrintMessage("Hello", 42)
PRINT

PRINT "Test 2: SUB with no parameters (using CALL)"
CALL PrintHello()
PRINT

PRINT "Test 3: SUB with no parameters (no CALL, no parens)"
PrintSimple
PRINT

PRINT "Done"
END

SUB PrintMessage(msg$, num%)
  PRINT msg$; " - Number: "; num%
END SUB

SUB PrintHello()
  PRINT "Hello from SUB"
END SUB

SUB PrintSimple()
  PRINT "Simple SUB"
END SUB
