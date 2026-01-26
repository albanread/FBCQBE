REM Test unsigned types
DIM flags(10) AS UBYTE
DIM ports(5) AS USHORT
flags(0) = 255
ports(0) = 65535
PRINT flags(0)
PRINT ports(0)
END
