10 REM Debug comparison operators
20 LET A% = 30
30 PRINT "A% = "; A%
40 PRINT "Testing: A% = 30"
50 IF A% = 30 THEN PRINT "  PASS: A% = 30 is TRUE"
60 IF A% <> 30 THEN PRINT "  FAIL: A% <> 30 is TRUE (should be FALSE)"
70 PRINT ""
80 PRINT "Testing: A% = 31"
90 IF A% = 31 THEN PRINT "  FAIL: A% = 31 is TRUE (should be FALSE)"
100 IF A% <> 31 THEN PRINT "  PASS: A% <> 31 is TRUE"
110 PRINT ""
120 LET B% = 10
130 LET C% = 20
140 LET D% = B% + C%
150 PRINT "B% = "; B%
160 PRINT "C% = "; C%
170 PRINT "D% = B% + C% = "; D%
180 PRINT "Testing: D% = 30"
190 IF D% = 30 THEN PRINT "  PASS: D% = 30 is TRUE"
200 IF D% <> 30 THEN PRINT "  FAIL: D% <> 30 is TRUE (should be FALSE)"
210 PRINT ""
220 PRINT "Testing: D% <> 30"
230 IF D% <> 30 THEN PRINT "  FAIL: Should not print this"
240 IF D% = 30 THEN PRINT "  PASS: Confirmed D% equals 30"
250 END
