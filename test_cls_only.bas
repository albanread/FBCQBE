REM Test CLS by itself
PRINT "Before CLS - you should see this"
PRINT "Line 2"
PRINT "Line 3"
PRINT ""
PRINT "Clearing in 1 second..."
REM Small delay simulation with a loop
FOR i = 1 TO 100000
NEXT i
CLS
PRINT "After CLS - screen should be cleared"
PRINT "This should be at the top"
