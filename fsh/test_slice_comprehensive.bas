REM Comprehensive string slicing test
DIM s$, result$

s$ = "Hello World"
PRINT "Original string: "; s$
PRINT "Length: "; LEN(s$)
PRINT

PRINT "=== Full slicing syntax ==="
result$ = s$(1 TO 11)
PRINT "s$(1 TO 11): "; result$

result$ = s$(7 TO 11)
PRINT "s$(7 TO 11): "; result$

result$ = s$(1 TO 5)
PRINT "s$(1 TO 5): "; result$

PRINT
PRINT "=== Implied start/end ==="
result$ = s$(TO 5)
PRINT "s$(TO 5): "; result$

result$ = s$(7 TO)
PRINT "s$(7 TO): "; result$

PRINT
PRINT "=== Edge cases ==="
result$ = s$(1 TO 1)
PRINT "s$(1 TO 1): "; result$

result$ = s$(11 TO 11)
PRINT "s$(11 TO 11): "; result$

result$ = s$(TO 1)
PRINT "s$(TO 1): "; result$

result$ = s$(11 TO)
PRINT "s$(11 TO): "; result$

END