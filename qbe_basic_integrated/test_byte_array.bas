REM Test BYTE array operations to verify sign-extended loads
REM This should generate loadsb (sign-extend byte) operations

DIM buffer(5) AS BYTE

REM Store some byte values
buffer(0) = 10
buffer(1) = 20
buffer(2) = -1
buffer(3) = 127
buffer(4) = -128

REM Load and print (should use loadsb for sign extension)
PRINT "buffer(0) = "; buffer(0)
PRINT "buffer(1) = "; buffer(1)
PRINT "buffer(2) = "; buffer(2)
PRINT "buffer(3) = "; buffer(3)
PRINT "buffer(4) = "; buffer(4)

END
