REM Test string concatenation
DIM s1$, s2$, s3$

s1$ = "Hello"
s2$ = "World"
s3$ = s1$ + " " + s2$

PRINT "s1$: "; s1$
PRINT "s2$: "; s2$
PRINT "s3$: "; s3$
PRINT "Length of s3$: "; LEN(s3$)

REM Test ASCII concat
DIM a$, b$, c$
a$ = "ABC"
b$ = "DEF"
c$ = a$ + b$
PRINT "ASCII concat: "; c$
PRINT "Length: "; LEN(c$)

END