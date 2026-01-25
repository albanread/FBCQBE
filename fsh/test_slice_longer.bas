REM Test replacing with longer string
DIM s$

s$ = "Hello"
PRINT "Original: "; s$

s$(1 TO 1) = "Hi"
PRINT "After s$(1 TO 1) = 'Hi': "; s$

END