' Test scalar assignments with different type suffixes

' Byte types
DIM a@ AS BYTE
DIM b@ AS UBYTE

' Short types
DIM c^ AS SHORT
DIM d^ AS USHORT

' Integer types (default size)
DIM e% AS INTEGER
DIM f% AS UINTEGER

' Long types
DIM g& AS LONG
DIM h& AS ULONG

' Float types
DIM i! AS SINGLE
DIM j# AS DOUBLE

' Test assignments
a@ = 127
b@ = 255
c^ = 32767
d^ = 65535
e% = 2147483647
f% = 4294967295
g& = 9223372036854775807
h& = 18446744073709551615
i! = 3.14159
j# = 2.718281828459045

' Test simple expressions
a@ = a@ + 1
b@ = b@ * 2
c^ = c^ - 100
d^ = d^ / 2

PRINT a@
PRINT b@
PRINT c^
PRINT d^
