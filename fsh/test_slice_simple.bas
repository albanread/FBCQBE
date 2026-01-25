REM Simple slice assignment test
DIM s$

s$ = "Hello"
PRINT "Original: "; s$

s$(1 TO 1) = "J"
PRINT "After s$(1 TO 1) = 'J': "; s$

END