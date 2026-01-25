10 REM Test ON GOTO with labels
20 X = 2
30 ON X GOTO target1, target2, target3
40 PRINT "Should not reach here"
50 END

target1:
60 PRINT "Target 1 reached"
70 END

target2:
80 PRINT "Target 2 reached"
90 END

target3:
100 PRINT "Target 3 reached"
110 END