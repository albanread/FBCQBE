REM Test TIMER function
PRINT "Program started at: "; TIMER; " seconds"
FOR I = 1 TO 5
  PRINT "Loop "; I; " at "; TIMER; " seconds"
  WAIT_MS 1000  ' Wait 1 second
NEXT I
PRINT "Program finished at: "; TIMER; " seconds"
PRINT "Total runtime: "; TIMER; " seconds"
END