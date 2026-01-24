' Test: Arrays of User-Defined Types
' This test verifies that arrays of UDTs work correctly

TYPE Point
    x AS DOUBLE
    y AS DOUBLE
END TYPE

TYPE RGB
    r AS INTEGER
    g AS INTEGER
    b AS INTEGER
END TYPE

' Test 1: Simple array of UDTs
DIM points(2) AS Point

' Initialize array elements
points(0).x = 1.5
points(0).y = 2.5

points(1).x = 10.0
points(1).y = 20.0

points(2).x = 100.5
points(2).y = 200.5

' Print array elements
PRINT "Point 0: ("; points(0).x; ", "; points(0).y; ")"
PRINT "Point 1: ("; points(1).x; ", "; points(1).y; ")"
PRINT "Point 2: ("; points(2).x; ", "; points(2).y; ")"

' Test 2: Array of UDTs with integers
DIM colors(1) AS RGB

colors(0).r = 255
colors(0).g = 128
colors(0).b = 64

colors(1).r = 32
colors(1).g = 64
colors(1).b = 128

PRINT "Color 0: RGB("; colors(0).r; ", "; colors(0).g; ", "; colors(0).b; ")"
PRINT "Color 1: RGB("; colors(1).r; ", "; colors(1).g; ", "; colors(1).b; ")"

' Test 3: Modify array element in a loop
FOR i% = 0 TO 2
    points(i%).x = points(i%).x * 2
    points(i%).y = points(i%).y * 2
NEXT i%

PRINT "After doubling:"
FOR i% = 0 TO 2
    PRINT "Point "; i%; ": ("; points(i%).x; ", "; points(i%).y; ")"
NEXT i%

END
