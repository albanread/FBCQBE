REM Test DEF FN - classic BASIC single-line functions

REM Simple function
DEF FN Double(X) = X * 2

REM Function with type suffix
DEF FN Square%(N) = N * N

REM Multiple parameters
DEF FN Add(A, B) = A + B

REM More complex expression
DEF FN Hypotenuse(A, B) = SQR(A * A + B * B)

REM Function using another function
DEF FN Average(X, Y) = FN Add(X, Y) / 2

REM Test the functions
PRINT "Test 1: FN Double(5) = "; FN Double(5)
PRINT "Expected: 10"
PRINT ""

PRINT "Test 2: FN Square%(7) = "; FN Square%(7)
PRINT "Expected: 49"
PRINT ""

PRINT "Test 3: FN Add(3, 4) = "; FN Add(3, 4)
PRINT "Expected: 7"
PRINT ""

PRINT "Test 4: FN Hypotenuse(3, 4) = "; FN Hypotenuse(3, 4)
PRINT "Expected: 5"
PRINT ""

PRINT "Test 5: FN Average(10, 20) = "; FN Average(10, 20)
PRINT "Expected: 15"
PRINT ""

REM Test with variables
X% = 6
Y% = 8
PRINT "Test 6: FN Hypotenuse("; X%; ", "; Y%; ") = "; FN Hypotenuse(X%, Y%)
PRINT "Expected: 10"
PRINT ""

PRINT "All DEF FN tests completed!"
END
