REM Test ERASE and REDIM statements
REM Test dynamic array management

PRINT "Testing ERASE and REDIM"
PRINT ""

REM Test 1: Basic array allocation and ERASE
PRINT "Test 1: ERASE"
DIM A(10) AS INTEGER
A(0) = 100
A(5) = 500
PRINT "A(0) = "; A(0)
PRINT "A(5) = "; A(5)
ERASE A
PRINT "Array A erased"
PRINT ""

REM Test 2: REDIM without PRESERVE
PRINT "Test 2: REDIM without PRESERVE"
DIM B(5) AS INTEGER
B(0) = 10
B(5) = 50
PRINT "Before REDIM: B(0) = "; B(0); ", B(5) = "; B(5)
REDIM B(10)
B(0) = 99
B(10) = 999
PRINT "After REDIM: B(0) = "; B(0); ", B(10) = "; B(10)
PRINT ""

REM Test 3: REDIM PRESERVE
PRINT "Test 3: REDIM PRESERVE"
DIM C(5) AS INTEGER
C(0) = 111
C(3) = 333
C(5) = 555
PRINT "Before REDIM PRESERVE: C(0) = "; C(0); ", C(3) = "; C(3); ", C(5) = "; C(5)
REDIM PRESERVE C(10)
C(10) = 1000
PRINT "After REDIM PRESERVE: C(0) = "; C(0); ", C(3) = "; C(3); ", C(10) = "; C(10)
PRINT ""

REM Test 4: Multiple arrays
PRINT "Test 4: ERASE multiple arrays"
DIM X(3) AS INTEGER
DIM Y(3) AS INTEGER
X(0) = 1
Y(0) = 2
PRINT "X(0) = "; X(0); ", Y(0) = "; Y(0)
ERASE X, Y
PRINT "Arrays X and Y erased"
PRINT ""

PRINT "All tests completed!"
END
