' Simple test for local arrays in functions

TYPE Point
    x AS DOUBLE
    y AS DOUBLE
END TYPE

' Test function with local array
FUNCTION TestLocal() AS INTEGER
    DIM points(2) AS Point

    points(0).x = 10.0
    points(0).y = 20.0

    PRINT "Point: ("; points(0).x; ", "; points(0).y; ")"

    TestLocal = 1
END FUNCTION

' Main
PRINT "Testing local arrays"
DIM result AS INTEGER
result = TestLocal()
PRINT "Result: "; result
END
