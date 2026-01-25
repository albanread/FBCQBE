REM Simple STRTYPE test
DIM s AS STRING
DIM t AS INTEGER

s = "Hello"
t = STRTYPE(s)
PRINT "ASCII: "; t

s = CHR$(200)
t = STRTYPE(s)
PRINT "UTF-32: "; t

END
