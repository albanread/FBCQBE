10 REM Test REPEAT/UNTIL loop
20 LET X% = 0
30 REPEAT
40   PRINT "X = "; X%
50   LET X% = X% + 1
60 UNTIL X% >= 5
70 PRINT "Loop finished"
80 END
