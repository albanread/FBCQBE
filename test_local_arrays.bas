' Test: Local Arrays in Functions with Cleanup
' Tests that local arrays in functions are properly allocated and freed

TYPE Point
    x AS DOUBLE
    y AS DOUBLE
END TYPE

TYPE RGB
    r AS INTEGER
    g AS INTEGER
    b AS INTEGER
END TYPE

' Global counter to verify cleanup behavior
DIM cleanup_count AS INTEGER
cleanup_count = 0

' Test 1: Simple function with local array
FUNCTION TestSimpleLocal() AS INTEGER
    DIM points(5) AS Point

    points(0).x = 10.0
    points(0).y = 20.0
    points(1).x = 30.0
    points(1).y = 40.0

    PRINT "Test 1: Simple local array"
    PRINT "Point 0: ("; points(0).x; ", "; points(0).y; ")"
    PRINT "Point 1: ("; points(1).x; ", "; points(1).y; ")"

    TestSimpleLocal = 1
END FUNCTION

' Test 2: Function with early EXIT
FUNCTION TestEarlyExit(condition AS INTEGER) AS INTEGER
    DIM colors(3) AS RGB

    colors(0).r = 255
    colors(0).g = 128
    colors(0).b = 64

    PRINT "Test 2: Early exit test"
    PRINT "Color: RGB("; colors(0).r; ", "; colors(0).g; ", "; colors(0).b; ")"

    IF condition = 1 THEN
        PRINT "Taking early exit"
        TestEarlyExit = 99
        EXIT FUNCTION
    END IF

    PRINT "Normal exit"
    TestEarlyExit = 100
END FUNCTION

' Test 3: SUB with local array
SUB TestSubLocal()
    DIM temp(2) AS Point

    temp(0).x = 1.5
    temp(0).y = 2.5
    temp(1).x = 3.5
    temp(1).y = 4.5

    PRINT "Test 3: SUB with local array"
    PRINT "Temp 0: ("; temp(0).x; ", "; temp(0).y; ")"
    PRINT "Temp 1: ("; temp(1).x; ", "; temp(1).y; ")"
END SUB

' Test 4: Nested function calls with local arrays
FUNCTION Outer() AS INTEGER
    DIM outer_arr(2) AS Point

    outer_arr(0).x = 100.0
    outer_arr(0).y = 200.0

    PRINT "Test 4: Nested calls - Outer"
    PRINT "Outer array: ("; outer_arr(0).x; ", "; outer_arr(0).y; ")"

    CALL Inner()

    PRINT "Back in Outer"
    Outer = 42
END FUNCTION

SUB Inner()
    DIM inner_arr(3) AS RGB

    inner_arr(0).r = 10
    inner_arr(0).g = 20
    inner_arr(0).b = 30

    PRINT "  Inner: RGB("; inner_arr(0).r; ", "; inner_arr(0).g; ", "; inner_arr(0).b; ")"
END SUB

' Test 5: Multiple local arrays in same function
FUNCTION TestMultipleArrays() AS INTEGER
    DIM arr1(2) AS Point
    DIM arr2(2) AS RGB

    arr1(0).x = 5.0
    arr1(0).y = 10.0

    arr2(0).r = 50
    arr2(0).g = 100
    arr2(0).b = 150

    PRINT "Test 5: Multiple local arrays"
    PRINT "Array 1: ("; arr1(0).x; ", "; arr1(0).y; ")"
    PRINT "Array 2: RGB("; arr2(0).r; ", "; arr2(0).g; ", "; arr2(0).b; ")"

    TestMultipleArrays = 5
END FUNCTION

' Main program
PRINT "=== Local Array Tests ==="
PRINT ""

DIM result AS INTEGER

result = TestSimpleLocal()
PRINT "Result: "; result
PRINT ""

result = TestEarlyExit(1)
PRINT "Result with early exit: "; result
PRINT ""

result = TestEarlyExit(0)
PRINT "Result without early exit: "; result
PRINT ""

CALL TestSubLocal()
PRINT ""

result = Outer()
PRINT "Outer result: "; result
PRINT ""

result = TestMultipleArrays()
PRINT "Multiple arrays result: "; result
PRINT ""

PRINT "=== All tests completed ==="
END
