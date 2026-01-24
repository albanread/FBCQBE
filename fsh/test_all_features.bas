10 REM Comprehensive test of all new function features
20 REM Using simple integer math to avoid type conversion issues
30 REM
40 REM Test 1: EXIT FUNCTION
50 FUNCTION TestExit(N)
60   IF N < 0 THEN EXIT FUNCTION
70   TestExit = N * N
80 END FUNCTION
90 REM
100 REM Test 2: RETURN with value
110 FUNCTION TestReturn(N)
120  IF N = 0 THEN RETURN 999
130  RETURN N + 100
140 END FUNCTION
150 REM
160 REM Test 3: LOCAL variable
170 FUNCTION TestLocal(N)
180  LOCAL Temp
190  Temp = N * 2
200  TestLocal = Temp + 1
210 END FUNCTION
220 REM
230 PRINT "=== Function Features Test ==="
240 PRINT ""
250 PRINT "EXIT FUNCTION:"
260 PRINT "  TestExit(7) = "; TestExit(7); " (expect 49)"
270 PRINT "  TestExit(-1) = "; TestExit(-1); " (expect 0)"
280 PRINT ""
290 PRINT "RETURN expr:"
300 PRINT "  TestReturn(0) = "; TestReturn(0); " (expect 999)"
310 PRINT "  TestReturn(5) = "; TestReturn(5); " (expect 105)"
320 PRINT ""
330 PRINT "LOCAL variable:"
340 PRINT "  TestLocal(10) = "; TestLocal(10); " (expect 21)"
350 PRINT ""
360 PRINT "All tests complete!"
370 END
