' Test Function Parameter Coercion
' Verify that arguments are properly coerced to match function parameter types

PRINT "Test 1: Pass INT to DOUBLE parameter"
FUNCTION AddDouble(x AS DOUBLE, y AS DOUBLE) AS DOUBLE
    AddDouble = x + y
END FUNCTION

a% = 5
b% = 3
result = AddDouble(a%, b%)
PRINT "AddDouble(5%, 3%) = "; result

PRINT "Test 2: Pass DOUBLE to INT parameter"
FUNCTION AddInt(x AS INTEGER, y AS INTEGER) AS INTEGER
    AddInt = x + y
END FUNCTION

c = 5.7
d = 3.2
result% = AddInt(c, d)
PRINT "AddInt(5.7, 3.2) = "; result%

PRINT "Test 3: Mixed parameter types"
FUNCTION MixedAdd(x AS INTEGER, y AS DOUBLE) AS DOUBLE
    MixedAdd = x + y
END FUNCTION

result = MixedAdd(10%, 2.5)
PRINT "MixedAdd(10%, 2.5) = "; result

PRINT "Test 4: Default parameter types (DOUBLE)"
FUNCTION DefaultAdd(x, y)
    DefaultAdd = x + y
END FUNCTION

result = DefaultAdd(1.5, 2.5)
PRINT "DefaultAdd(1.5, 2.5) = "; result

PRINT "All function coercion tests completed!"
