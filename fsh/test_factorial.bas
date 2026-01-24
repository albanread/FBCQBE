10 REM Test recursive factorial function
20 FUNCTION Factorial(N AS INTEGER) AS INTEGER
30   IF N <= 1 THEN RETURN 1
40   RETURN N * Factorial(N - 1)
50 END FUNCTION
60 REM
70 PRINT "Factorial tests:"
80 PRINT "Factorial(0) = "; Factorial(0)
90 PRINT "Factorial(1) = "; Factorial(1)
100 PRINT "Factorial(5) = "; Factorial(5)
110 PRINT "Factorial(10) = "; Factorial(10)
120 END
