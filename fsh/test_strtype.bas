REM Test STRTYPE() intrinsic function
REM Returns encoding type: 0=ASCII, 1=UTF-32
PRINT "=== STRTYPE() Intrinsic Test ==="
PRINT ""

DIM s AS STRING
DIM t AS INTEGER

REM Test 1: Pure ASCII string
s = "Hello"
t = STRTYPE(s)
PRINT "ASCII string test = "; t

REM Test 2: Single ASCII character
s = "A"
t = STRTYPE(s)
PRINT "Single char test = "; t

REM Test 3: CHR$() with ASCII value < 128
s = CHR$(65)
t = STRTYPE(s)
PRINT "CHR$(65) test = "; t

REM Test 4: CHR$() with Unicode value >= 128
s = CHR$(200)
t = STRTYPE(s)
PRINT "CHR$(200) test = "; t

REM Test 5: CHR$() with high Unicode
s = CHR$(12371)
t = STRTYPE(s)
PRINT "CHR$(12371) test = "; t

REM Test 6: Empty string
s = ""
t = STRTYPE(s)
PRINT "Empty string test = "; t

REM Test 7: Combined test
PRINT ""
PRINT "Summary:"
PRINT "  ASCII encoding: 0"
PRINT "  UTF-32 encoding: 1"
PRINT "  Auto-promotion with Unicode"

PRINT ""
PRINT "Done!"
END
