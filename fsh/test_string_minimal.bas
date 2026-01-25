REM Simple test of core string intrinsics
PRINT "String Intrinsics Test"
PRINT ""

DIM s AS STRING
DIM n AS INTEGER

s = "Hello"
PRINT "String: "; s

n = LEN(s)
PRINT "Length: "; n

n = ASC(s)
PRINT "First char code: "; n

s = CHR$(72)
PRINT "CHR$(72): "; s

PRINT ""
PRINT "Done!"
END
