' Test Type System - Mixed INT and DOUBLE operations
' This tests the default-to-DOUBLE type system

' Test 1: Default numeric type is DOUBLE
PRINT "Test 1: Default numeric type"
x = 10
y = 20
z = x + y
PRINT "10 + 20 = "; z

' Test 2: Explicit integer type
PRINT "Test 2: Explicit integer type"
a% = 10
b% = 20
c% = a% + b%
PRINT "10% + 20% = "; c%

' Test 3: Mixed int and double
PRINT "Test 3: Mixed int/double arithmetic"
i% = 5
d = 2.5
result = i% + d
PRINT "5% + 2.5 = "; result

' Test 4: Division
PRINT "Test 4: Division (always double)"
x = 10 / 3
PRINT "10 / 3 = "; x
x% = 10
y% = 3
z = x% / y%
PRINT "10% / 3% = "; z

' Test 5: MOD operation (requires integers)
PRINT "Test 5: MOD (integer operation)"
m = 10.7
n = 3.2
r = m MOD n
PRINT "10.7 MOD 3.2 = "; r

' Test 6: Comparisons
PRINT "Test 6: Comparisons"
IF 10.5 = 10.5 THEN PRINT "10.5 = 10.5: TRUE"
IF 10 > 5 THEN PRINT "10 > 5: TRUE"
IF 3.14 < 3.15 THEN PRINT "3.14 < 3.15: TRUE"

' Test 7: FOR loop (always integer counter)
PRINT "Test 7: FOR loop counter (always INT)"
FOR i = 1 TO 5
    PRINT i;
NEXT i
PRINT

' Test 8: Bitwise operations (require integers)
PRINT "Test 8: Bitwise operations"
x = 12.9
y = 5.7
z = x AND y
PRINT "12.9 AND 5.7 = "; z
z = x OR y
PRINT "12.9 OR 5.7 = "; z

' Test 9: Unary operations
PRINT "Test 9: Unary operations"
x = 5.5
y = -x
PRINT "-5.5 = "; y

' Test 10: Type mixing in expressions
PRINT "Test 10: Complex expression"
a% = 10
b = 2.5
c% = 3
result = a% + b * c%
PRINT "10% + 2.5 * 3% = "; result

PRINT "All type tests completed!"
