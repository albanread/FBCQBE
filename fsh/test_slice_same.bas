REM Test same-length replacement
DIM s$

s$ = "Hello"
PRINT "Original: "; s$

s$(1 TO 1) = "X"
PRINT "After s$(1 TO 1) = 'X': "; s$

END