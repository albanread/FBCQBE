10 REM Test LOCAL, SHARED, EXIT FUNCTION, RETURN
20 REM Using all integers to avoid type conversions
30 DIM GlobalX AS Integer
40 DIM GlobalY AS Integer
50 GlobalX = 100
60 GlobalY = 200
70 REM
80 FUNCTION TestLocal(N AS Integer) AS Integer
90   LOCAL Result AS Integer
100  Result = N * 2
110  TestLocal = Result
120 END FUNCTION
130 REM
140 FUNCTION TestShared(N AS Integer) AS Integer
150  SHARED GlobalX
160  TestShared = GlobalX + N
170 END FUNCTION
180 REM
190 FUNCTION TestExit(N AS Integer) AS Integer
200  IF N < 0 THEN EXIT FUNCTION
210  TestExit = N * N
220 END FUNCTION
230 REM
240 FUNCTION TestReturn(N AS Integer) AS Integer
250  IF N = 0 THEN RETURN 999
260  RETURN N + 100
270 END FUNCTION
280 REM
290 DIM A AS Integer
300 DIM B AS Integer
310 DIM C AS Integer
320 DIM D AS Integer
330 DIM E AS Integer
340 DIM F AS Integer
350 A = TestLocal(5)
360 B = TestShared(50)
370 C = TestExit(7)
380 D = TestExit(-1)
390 E = TestReturn(0)
400 F = TestReturn(5)
410 PRINT "TestLocal(5) = "; A
420 PRINT "TestShared(50) = "; B
430 PRINT "TestExit(7) = "; C
440 PRINT "TestExit(-1) = "; D
450 PRINT "TestReturn(0) = "; E
460 PRINT "TestReturn(5) = "; F
470 END
