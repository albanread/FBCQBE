' Test float and double array assignments with FOR loops

' Single precision (float) array
DIM floats!(5) AS SINGLE
DIM i% AS INTEGER
FOR i% = 0 TO 5
    floats!(i%) = i% * 1.5
NEXT i%

' Double precision array
DIM doubles#(4) AS DOUBLE
DIM j% AS INTEGER
FOR j% = 0 TO 4
    doubles#(j%) = j% * 3.14159
NEXT j%

' Print results
PRINT "Float array:"
FOR i% = 0 TO 5
    PRINT floats!(i%)
NEXT i%

PRINT ""
PRINT "Double array:"
FOR j% = 0 TO 4
    PRINT doubles#(j%)
NEXT j%
