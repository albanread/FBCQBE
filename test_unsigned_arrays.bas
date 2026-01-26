' Test unsigned array operations to verify unsigned load/store instructions

' Unsigned byte array
DIM ubuffer@(5) AS UBYTE
DIM i@ AS BYTE
FOR i@ = 0 TO 5
    ubuffer@(i@) = i@ * 50
NEXT i@

' Unsigned short array
DIM uvalues^(3) AS USHORT
DIM j^ AS SHORT
FOR j^ = 0 TO 3
    uvalues^(j^) = j^ * 20000
NEXT j^

' Unsigned integer array
DIM ucounts%(2) AS UINTEGER
DIM k% AS INTEGER
FOR k% = 0 TO 2
    ucounts%(k%) = k% * 1000000000
NEXT k%

' Read back and print
PRINT "Unsigned byte array:"
FOR i@ = 0 TO 5
    PRINT ubuffer@(i@)
NEXT i@

PRINT "Unsigned short array:"
FOR j^ = 0 TO 3
    PRINT uvalues^(j^)
NEXT j^

PRINT "Unsigned integer array:"
FOR k% = 0 TO 2
    PRINT ucounts%(k%)
NEXT k%

' Test boundary values
DIM maxbyte@ AS UBYTE
DIM maxshort^ AS USHORT
maxbyte@ = 255
maxshort^ = 65535
ubuffer@(0) = maxbyte@
uvalues^(0) = maxshort^

PRINT "Max byte: "; ubuffer@(0)
PRINT "Max short: "; uvalues^(0)
