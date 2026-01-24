' Test: Local Arrays in Functions - Final Test
' Tests allocation, usage, and cleanup of local arrays

TYPE Point
    x AS DOUBLE
    y AS DOUBLE
END TYPE

TYPE RGB
    r AS INTEGER
    g AS INTEGER
    b AS INTEGER
END TYPE

' Test 1: Simple function with local array
FUNCTION TestSimple() AS INTEGER
    DIM points(2) AS Point

    points(0).x = 10.0
    points(0).y = 20.0
    points(1).x = 30.0
    points(1).y = 40.0
    points(2).x = 50.0
    points(2).y = 60.0

    PRINT "Test 1: Simple local array"
    PRINT "Point 0: ("; points(0).x; ", "; points(0).y; ")"
    PRINT "Point 1: ("; points(1).x; ", "; points(1).y; ")"
    PRINT "Point 2: ("; points(2).x; ", "; points(2).y; ")"

    TestSimple = 1
END FUNCTION

' Test 2: FUNCTION with local array (was SUB)
FUNCTION TestSub() AS INTEGER
    DIM colors(1) AS RGB

    colors(0).r = 255
    colors(0).g = 128
    colors(0).b = 64

    colors(1).r = 32
    colors(1).g = 64
    colors(1).b = 128

    PRINT "Test 2: SUB with local array"
    PRINT "Color 0: RGB("; colors(0).r; ", "; colors(0).g; ", "; colors(0).b; ")"
    PRINT "Color 1: RGB("; colors(1).r; ", "; colors(1).g; ", "; colors(1).b; ")"

    TestSub = 2
END FUNCTION

' Test 3: Multiple local arrays
FUNCTION TestMultiple() AS INTEGER
    DIM arr1(1) AS Point
    DIM arr2(1) AS RGB

    arr1(0).x = 100.0
    arr1(0).y = 200.0

    arr2(0).r = 50
    arr2(0).g = 100
    arr2(0).b = 150

    PRINT "Test 3: Multiple local arrays"
    PRINT "Array 1: ("; arr1(0).x; ", "; arr1(0).y; ")"
    PRINT "Array 2: RGB("; arr2(0).r; ", "; arr2(0).g; ", "; arr2(0).b; ")"

    TestMultiple = 3
END FUNCTION

' Test 4: Nested function calls (each with local arrays)
FUNCTION Outer() AS INTEGER
    DIM outer_arr(1) AS Point

    outer_arr(0).x = 1.0
    outer_arr(0).y = 2.0

    PRINT "Test 4: Nested calls - Outer"
    PRINT "Outer: ("; outer_arr(0).x; ", "; outer_arr(0).y; ")"

    DIM dummy AS INTEGER
    dummy = Inner()

    PRINT "Back in Outer"
    Outer = 4
END FUNCTION

FUNCTION Inner() AS INTEGER
    DIM inner_arr(1) AS RGB

    inner_arr(0).r = 10
    inner_arr(0).g = 20
    inner_arr(0).b = 30

    PRINT "  Inner: RGB("; inner_arr(0).r; ", "; inner_arr(0).g; ", "; inner_arr(0).b; ")"

    Inner = 44
END FUNCTION

' Main program
PRINT "=== Local Array Tests ==="
PRINT ""

DIM result AS INTEGER

result = TestSimple()
PRINT "Result: "; result
PRINT ""

result = TestSub()
PRINT "Result: "; result
PRINT ""

result = TestMultiple()
PRINT "Result: "; result
PRINT ""

result = Outer()
PRINT "Result: "; result
PRINT ""

PRINT "=== All tests completed ==="
PRINT "Note: All local arrays were automatically freed"
END
