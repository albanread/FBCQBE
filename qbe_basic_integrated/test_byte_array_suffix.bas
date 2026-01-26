REM Test BYTE array using suffix notation
REM This should generate byte-sized array operations

DIM buffer@(5)

REM Store some byte values
buffer@(0) = 10
buffer@(1) = 20
buffer@(2) = 100

REM Load and print
PRINT buffer@(0)
PRINT buffer@(1)
PRINT buffer@(2)

END
