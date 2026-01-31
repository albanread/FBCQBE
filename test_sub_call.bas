' Test SUB calls with and without CALL keyword

PRINT "Test 1: Using CALL keyword"
CALL PrintHello()
PRINT

PRINT "Test 2: Without CALL keyword"
PrintHello
PRINT

PRINT "Done"
END

SUB PrintHello()
  PRINT "Hello from SUB"
END SUB
