REM Comprehensive test of all array types
DIM byteArr(3) AS BYTE
DIM ubyteArr(3) AS UBYTE
DIM shortArr(3) AS SHORT
DIM ushortArr(3) AS USHORT
DIM intArr(3) AS INTEGER
DIM longArr(3) AS LONG

byteArr(0) = -128
ubyteArr(0) = 255
shortArr(0) = -32000
ushortArr(0) = 65000
intArr(0) = 100000
longArr(0) = 1000000

PRINT byteArr(0)
PRINT ubyteArr(0)
PRINT shortArr(0)
PRINT ushortArr(0)
PRINT intArr(0)
PRINT longArr(0)
END
