REM Phase 4 Test: New Type Suffixes and AS Type Declarations
REM Tests @ (BYTE), ^ (SHORT) suffixes and new AS keywords

REM Test 1: BYTE suffix (@)
DIM byteVar@
byteVar@ = 127
PRINT "Byte variable: "; byteVar@

REM Test 2: SHORT suffix (^)
DIM shortVar^
shortVar^ = 32000
PRINT "Short variable: "; shortVar^

REM Test 3: AS BYTE declaration
DIM explicitByte AS BYTE
explicitByte = 100
PRINT "Explicit BYTE: "; explicitByte

REM Test 4: AS SHORT declaration
DIM explicitShort AS SHORT
explicitShort = 20000
PRINT "Explicit SHORT: "; explicitShort

REM Test 5: AS LONG declaration
DIM longValue AS LONG
longValue = 2147483647
PRINT "LONG value: "; longValue

REM Test 6: Unsigned types
DIM ubyteVal AS UBYTE
DIM ushortVal AS USHORT
DIM uintVal AS UINTEGER
DIM ulongVal AS ULONG

ubyteVal = 255
ushortVal = 65535
uintVal = 4294967295
ulongVal = 9223372036854775807

PRINT "UBYTE: "; ubyteVal
PRINT "USHORT: "; ushortVal
PRINT "UINTEGER: "; uintVal
PRINT "ULONG: "; ulongVal

REM Test 7: Mixed suffixes and AS declarations
DIM mixedInt% AS INTEGER
DIM mixedByte@ AS BYTE
DIM mixedShort^ AS SHORT

mixedInt% = 42
mixedByte@ = 64
mixedShort^ = 1024

PRINT "Mixed INT: "; mixedInt%
PRINT "Mixed BYTE: "; mixedByte@
PRINT "Mixed SHORT: "; mixedShort^

REM Test 8: Function with new type suffixes
FUNCTION AddBytes@(a@, b@) AS BYTE
    AddBytes@ = a@ + b@
END FUNCTION

FUNCTION AddShorts^(a^, b^) AS SHORT
    AddShorts^ = a^ + b^
END FUNCTION

DIM result1@, result2^
result1@ = AddBytes@(10, 20)
result2^ = AddShorts^(100, 200)

PRINT "AddBytes result: "; result1@
PRINT "AddShorts result: "; result2^

REM Test 9: Arrays with new types
DIM byteArray@(10)
DIM shortArray^(10)
DIM explicitArray(5) AS BYTE

byteArray@(0) = 1
byteArray@(1) = 2
shortArray^(0) = 100
shortArray^(1) = 200
explicitArray(0) = 50

PRINT "Byte array [0]: "; byteArray@(0)
PRINT "Short array [0]: "; shortArray^(0)
PRINT "Explicit array [0]: "; explicitArray(0)

REM Test 10: Type coercion
DIM b@ AS BYTE
DIM s^ AS SHORT
DIM i% AS INTEGER
DIM l AS LONG

b@ = 10
s^ = b@        REM BYTE -> SHORT (widening, safe)
i% = s^        REM SHORT -> INTEGER (widening, safe)
l = i%         REM INTEGER -> LONG (widening, safe)

PRINT "Coercion chain: "; b@; " -> "; s^; " -> "; i%; " -> "; l

PRINT "Phase 4 Test Complete!"
END
