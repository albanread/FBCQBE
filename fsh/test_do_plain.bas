10 REM Test plain DO...LOOP with counter and GOTO
20 LET X% = 0
30 DO
40   PRINT "X = "; X%
50   LET X% = X% + 1
60   IF X% >= 5 THEN GOTO 100
70 LOOP
100 PRINT "Exited at X = "; X%
110 END
