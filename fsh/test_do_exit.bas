10 REM Test DO...LOOP with EXIT
20 LET X% = 0
30 DO
40   PRINT "X = "; X%
50   LET X% = X% + 1
60   IF X% >= 5 THEN EXIT DO
70 LOOP
80 PRINT "Exited loop at X = "; X%
90 END
