10 REM Test recursive Fibonacci function
20 FUNCTION Fib(N AS INTEGER) AS INTEGER
30   IF N <= 1 THEN RETURN N
40   RETURN Fib(N - 1) + Fib(N - 2)
50 END FUNCTION
60 REM
70 PRINT "Fibonacci sequence:"
80 PRINT "Fib(0) = "; Fib(0)
90 PRINT "Fib(1) = "; Fib(1)
100 PRINT "Fib(2) = "; Fib(2)
110 PRINT "Fib(3) = "; Fib(3)
120 PRINT "Fib(4) = "; Fib(4)
130 PRINT "Fib(5) = "; Fib(5)
140 PRINT "Fib(6) = "; Fib(6)
150 PRINT "Fib(7) = "; Fib(7)
160 PRINT "Fib(8) = "; Fib(8)
170 PRINT "Fib(9) = "; Fib(9)
180 PRINT "Fib(10) = "; Fib(10)
190 END
