10 REM Test FUNCTION and SUB
20 REM
30 REM Define a function to square a number
40 FUNCTION Square(X AS DOUBLE) AS DOUBLE
50   Square = X * X
60 END FUNCTION
70 REM
80 REM Define a function to find max of two numbers
90 FUNCTION Max(A AS INTEGER, B AS INTEGER) AS INTEGER
100   IF A > B THEN
110     Max = A
120   ELSE
130     Max = B
140   END IF
150 END FUNCTION
160 REM
170 REM Define a subroutine to print a message
180 SUB PrintMessage(Msg AS STRING)
190   PRINT "Message: "; Msg
200 END SUB
210 REM
220 REM Main program
230 LET Result = Square(5)
240 PRINT "Square of 5 is "; Result
250 REM
260 LET Largest = Max(10, 20)
270 PRINT "Max of 10 and 20 is "; Largest
280 REM
290 CALL PrintMessage("Hello from SUB!")
300 REM
310 END
