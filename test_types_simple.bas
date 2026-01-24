' Simple type system test
' Test default DOUBLE and explicit INT

' Test 1: Default to DOUBLE
x = 10
y = 20
z = x + y
PRINT "10 + 20 = "; z

' Test 2: Explicit INT
a% = 5
b% = 3
c% = a% + b%
PRINT "5% + 3% = "; c%

' Test 3: Mixed types
i% = 4
d = 2.5
result = i% + d
PRINT "4% + 2.5 = "; result

' Test 4: MOD with doubles
m = 10.7
n = 3.2
r = m MOD n
PRINT "10.7 MOD 3.2 = "; r

' Test 5: Division
x = 7 / 2
PRINT "7 / 2 = "; x

' Test 6: Comparison
IF 5 > 3 THEN PRINT "5 > 3 is TRUE"

PRINT "Done"
