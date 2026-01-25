REM Test MID$ function
DIM s$, m$

s$ = "Hello World"
PRINT "Original: "; s$

m$ = MID$(s$, 7, 5)
PRINT "MID$(s$, 7, 5): "; m$

m$ = LEFT$(s$, 5)
PRINT "LEFT$(s$, 5): "; m$

m$ = RIGHT$(s$, 5)
PRINT "RIGHT$(s$, 5): "; m$

END