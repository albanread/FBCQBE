10 REM Minimal test case for FOR loop in SUB
20 REM This should help debug the FOR variable allocation issue
30 PRINT "Testing FOR loop in SUB..."
40 CALL TestForLoop
50 PRINT "Done!"
60 END
70
80 SUB TestForLoop()
90   LOCAL i AS INTEGER
100   PRINT "Loop starting:"
110   FOR i = 1 TO 5
120     PRINT "i = "; i
130   NEXT i
140   PRINT "Loop finished"
150 END SUB
