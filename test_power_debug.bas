10 REM Debug power test
20 PRINT "Testing power operator..."
30 LET A% = 2
40 PRINT "A% = "; A%
50 LET B% = 3
60 PRINT "B% = "; B%
70 PRINT "Computing C% = A% ^ B%..."
80 LET C% = A% ^ B%
90 PRINT "C% = "; C%
100 IF C% = 8 THEN PRINT "PASS" ELSE PRINT "FAIL: Expected 8, got "; C%
110 END
