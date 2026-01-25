REM Test string slice assignment
DIM s$, result$

s$ = "Hello World"
PRINT "Original: "; s$

REM Test slice assignment with full range
s$(1 TO 5) = "Hi"
PRINT "After s$(1 TO 5) = 'Hi': "; s$

REM Reset for next test
s$ = "Hello World"
PRINT "Reset to: "; s$

REM Test slice assignment with implied end
s$(7 TO) = "Universe"
PRINT "After s$(7 TO) = 'Universe': "; s$

REM Reset for next test
s$ = "Hello World"
PRINT "Reset to: "; s$

REM Test slice assignment with implied start
s$(TO 5) = "Hi"
PRINT "After s$(TO 5) = 'Hi': "; s$

REM Test single character replacement
s$(1 TO 1) = "J"
PRINT "After s$(1 TO 1) = 'J': "; s$

END