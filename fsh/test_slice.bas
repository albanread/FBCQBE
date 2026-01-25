REM Test string slicing syntax
DIM s$, m$

s$ = "Hello World"
PRINT "Original: "; s$

m$ = s$(7 TO 11)
PRINT "s$(7 TO 11): "; m$

m$ = s$(1 TO 5)
PRINT "s$(1 TO 5): "; m$

m$ = s$(7 TO 7)
PRINT "s$(7 TO 7): "; m$

m$ = s$(TO 5)
PRINT "s$(TO 5): "; m$

m$ = s$(7 TO)
PRINT "s$(7 TO): "; m$

END