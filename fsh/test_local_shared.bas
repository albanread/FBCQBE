10 REM Test LOCAL and SHARED
20 Counter = 0
30 FUNCTION TestLocal(N)
40   LOCAL Temp
50   Temp = N * 2
60   TestLocal = Temp
70 END FUNCTION
80 REM
90 FUNCTION TestShared(N)
100  SHARED Counter
110  Counter = Counter + N
120  TestShared = Counter
130 END FUNCTION
140 REM
150 PRINT "Counter = "; Counter
160 PRINT "TestLocal(5) = "; TestLocal(5)
170 PRINT "Counter still = "; Counter
180 PRINT "TestShared(10) = "; TestShared(10)
190 PRINT "Counter now = "; Counter
200 PRINT "TestShared(20) = "; TestShared(20)
210 PRINT "Counter now = "; Counter
220 END
