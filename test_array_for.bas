' Test array assignments with FOR loops to verify store operations

' Byte array
DIM buffer@(10) AS BYTE
DIM i@ AS BYTE
FOR i@ = 0 TO 10
    buffer@(i@) = i@ * 2
NEXT i@

' Short array
DIM values^(5) AS SHORT
DIM j^ AS SHORT
FOR j^ = 0 TO 5
    values^(j^) = j^ * 100
NEXT j^

' Integer array
DIM counts%(3) AS INTEGER
DIM k% AS INTEGER
FOR k% = 0 TO 3
    counts%(k%) = k% * 1000
NEXT k%

' Print results
PRINT "Byte array:"
FOR i@ = 0 TO 10
    PRINT buffer@(i@)
NEXT i@

PRINT "Short array:"
FOR j^ = 0 TO 5
    PRINT values^(j^)
NEXT j^

PRINT "Integer array:"
FOR k% = 0 TO 3
    PRINT counts%(k%)
NEXT k%
