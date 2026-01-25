REM Test MID$ assignment
DIM s$

s$ = "Hello World"
PRINT "Original: "; s$

MID$(s$, 7, 5) = "Universe"
PRINT "After MID$(s$, 7, 5) = 'Universe': "; s$

s$ = "Hello World"
PRINT "Reset to: "; s$

MID$(s$, 1, 5) = "Hi"
PRINT "After MID$(s$, 1, 5) = 'Hi': "; s$

END