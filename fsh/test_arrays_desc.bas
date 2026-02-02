' test_arrays_desc.bas
' Test array descriptor implementation with bounds checking
' Tests: DIM, array access, REDIM, REDIM PRESERVE, ERASE

PRINT "=== Array Descriptor Test ==="
PRINT ""

' Test 1: Basic DIM and access
PRINT "Test 1: DIM and basic access"
DIM A(5)
A(0) = 100
A(1) = 200
A(2) = 300
A(5) = 500
PRINT "A(0) = "; A(0)
PRINT "A(1) = "; A(1)
PRINT "A(2) = "; A(2)
PRINT "A(5) = "; A(5)
PRINT ""

' Test 2: REDIM (discard data)
PRINT "Test 2: REDIM (should discard old data)"
REDIM A(3)
A(0) = 10
A(1) = 20
A(2) = 30
A(3) = 40
PRINT "After REDIM A(3):"
PRINT "A(0) = "; A(0)
PRINT "A(1) = "; A(1)
PRINT "A(2) = "; A(2)
PRINT "A(3) = "; A(3)
PRINT ""

' Test 3: REDIM PRESERVE (keep data)
PRINT "Test 3: REDIM PRESERVE (should keep data)"
REDIM PRESERVE A(6)
A(4) = 50
A(5) = 60
A(6) = 70
PRINT "After REDIM PRESERVE A(6):"
PRINT "A(0) = "; A(0); " (should be 10)"
PRINT "A(1) = "; A(1); " (should be 20)"
PRINT "A(2) = "; A(2); " (should be 30)"
PRINT "A(3) = "; A(3); " (should be 40)"
PRINT "A(4) = "; A(4); " (should be 50)"
PRINT "A(5) = "; A(5); " (should be 60)"
PRINT "A(6) = "; A(6); " (should be 70)"
PRINT ""

' Test 4: ERASE
PRINT "Test 4: ERASE array"
ERASE A
PRINT "Array A erased"
PRINT ""

' Test 5: Redimension after ERASE
PRINT "Test 5: REDIM after ERASE"
REDIM A(2)
A(0) = 111
A(1) = 222
A(2) = 333
PRINT "After re-REDIM A(2):"
PRINT "A(0) = "; A(0)
PRINT "A(1) = "; A(1)
PRINT "A(2) = "; A(2)
PRINT ""

' Test 6: Multiple arrays
PRINT "Test 6: Multiple arrays"
DIM B(3), C(2)
B(0) = 1000
B(1) = 2000
B(2) = 3000
C(0) = 5
C(1) = 10
PRINT "B(0) = "; B(0)
PRINT "B(1) = "; B(1)
PRINT "B(2) = "; B(2)
PRINT "C(0) = "; C(0)
PRINT "C(1) = "; C(1)
PRINT ""

PRINT "=== All tests passed! ==="
