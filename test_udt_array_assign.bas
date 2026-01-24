' Test: Comprehensive UDT Array Assignment Patterns
' Tests various ways to assign to and from array elements

TYPE Point
    x AS DOUBLE
    y AS DOUBLE
END TYPE

TYPE RGB
    r AS INTEGER
    g AS INTEGER
    b AS INTEGER
END TYPE

' Test 1: Assign scalar variable to array element (member by member)
DIM p AS Point
DIM points(2) AS Point

p.x = 100.5
p.y = 200.5

points(0).x = p.x
points(0).y = p.y

PRINT "Test 1: Scalar to array element"
PRINT "Original: ("; p.x; ", "; p.y; ")"
PRINT "Copied:   ("; points(0).x; ", "; points(0).y; ")"

' Test 2: Assign from one array element to another
points(1).x = 50.0
points(1).y = 75.0

points(2).x = points(1).x
points(2).y = points(1).y

PRINT "Test 2: Array element to array element"
PRINT "Source: ("; points(1).x; ", "; points(1).y; ")"
PRINT "Dest:   ("; points(2).x; ", "; points(2).y; ")"

' Test 3: Assign array element to scalar variable
DIM q AS Point
q.x = points(0).x
q.y = points(0).y

PRINT "Test 3: Array element to scalar"
PRINT "Array:  ("; points(0).x; ", "; points(0).y; ")"
PRINT "Scalar: ("; q.x; ", "; q.y; ")"

' Test 4: Integer type array assignment
DIM colors(1) AS RGB

colors(0).r = 255
colors(0).g = 128
colors(0).b = 64

colors(1).r = colors(0).r
colors(1).g = colors(0).g
colors(1).b = colors(0).b

PRINT "Test 4: Integer array element copying"
PRINT "Color[0]: RGB("; colors(0).r; ", "; colors(0).g; ", "; colors(0).b; ")"
PRINT "Color[1]: RGB("; colors(1).r; ", "; colors(1).g; ", "; colors(1).b; ")"

' Test 5: Modify array element in place
points(1).x = points(1).x + 10.0
points(1).y = points(1).y * 2.0

PRINT "Test 5: In-place modification"
PRINT "Modified: ("; points(1).x; ", "; points(1).y; ")"

' Test 6: Mixed reads and writes (swap r and g)
DIM temp AS INTEGER
temp = colors(0).r
colors(0).r = colors(0).g
colors(0).g = temp

PRINT "Test 6: Swap fields using temp variable"
PRINT "After swap: RGB("; colors(0).r; ", "; colors(0).g; ", "; colors(0).b; ")"

END
