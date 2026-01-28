10 REM Minimal math operators test
20 PRINT "=== Minimal Math Test ==="
30 PRINT ""
40 REM Test 1: Power
50 PRINT "--- Power Test ---"
60 LET A% = 2
70 LET B% = 3
80 LET C% = A% ^ B%
90 PRINT "2 ^ 3 = "; C%
100 IF C% <> 8 THEN PRINT "ERROR: 2^3 failed" : END
110 PRINT "PASS: Power"
120 PRINT ""
130 REM Test 2: Integer Division
140 PRINT "--- Integer Division Test ---"
150 LET D% = 20
160 LET E% = 4
170 LET F% = D% \ E%
180 PRINT "20 \ 4 = "; F%
190 IF F% <> 5 THEN PRINT "ERROR: 20\4 failed" : END
200 PRINT "PASS: Integer Division"
210 PRINT ""
220 REM Test 3: Modulo
230 PRINT "--- Modulo Test ---"
240 LET G% = 17
250 LET H% = 5
260 LET I% = G% MOD H%
270 PRINT "17 MOD 5 = "; I%
280 IF I% <> 2 THEN PRINT "ERROR: 17 MOD 5 failed" : END
290 PRINT "PASS: Modulo"
300 PRINT ""
310 PRINT "=== All Tests PASSED ==="
320 END
