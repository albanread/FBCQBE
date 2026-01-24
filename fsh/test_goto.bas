10 REM Test multiple blocks with GOTO
20 LET X% = 10
30 IF X% > 5 THEN GOTO 100
40 PRINT "X is small"
50 GOTO 200
100 PRINT "X is large"
110 GOTO 200
200 PRINT "Done"
210 END
