REM Test Phase 4: BYTE and SHORT types
REM Simple test to verify new type system works

REM Test 1: BYTE variable
DIM b AS BYTE
b = 100
PRINT "BYTE value: "; b

REM Test 2: SHORT variable
DIM s AS SHORT
s = 20000
PRINT "SHORT value: "; s

REM Test 3: BYTE array
DIM byteArray(10) AS BYTE
byteArray(0) = 50
byteArray(1) = 75
PRINT "Byte array [0]: "; byteArray(0)
PRINT "Byte array [1]: "; byteArray(1)

REM Test 4: SHORT array
DIM shortArray(10) AS SHORT
shortArray(0) = 1000
shortArray(1) = 2000
PRINT "Short array [0]: "; shortArray(0)
PRINT "Short array [1]: "; shortArray(1)

REM Test 5: Type coercion
DIM i AS INTEGER
i = b
PRINT "BYTE to INTEGER: "; i

END
