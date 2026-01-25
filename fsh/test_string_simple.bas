REM Test simple string operations
DIM s1$, s2$, s3$

s1$ = "Hello"
s2$ = "World"
s3$ = s1$ + " " + s2$

PRINT s3$
PRINT "Length: "; LEN(s3$)
PRINT "First char: "; CHR$(ASC(s3$))
END
