10 REM Test EXIT FUNCTION and RETURN
20 FUNCTION TestExit(N)
30   IF N < 0 THEN EXIT FUNCTION
40   TestExit = N * N
50 END FUNCTION
60 REM
70 FUNCTION TestReturn(N)
80   IF N = 0 THEN RETURN 999
90   RETURN N + 100
100 END FUNCTION
110 REM
120 PRINT "TestExit(7) = "; TestExit(7)
130 PRINT "TestExit(-1) = "; TestExit(-1)
140 PRINT "TestReturn(0) = "; TestReturn(0)
150 PRINT "TestReturn(5) = "; TestReturn(5)
160 END
