10 REM Test TIMER function
20 PRINT "Program started at: "; TIMER; " seconds"
30 FOR I = 1 TO 5
40   PRINT "Loop "; I; " at "; TIMER; " seconds"
50   WAIT_MS 1000  ' Wait 1 second
60 NEXT I
70 PRINT "Program finished at: "; TIMER; " seconds"
80 PRINT "Total runtime: "; TIMER; " seconds"
90 END