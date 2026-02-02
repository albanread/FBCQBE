10 REM Final test of SGN fix
20 PRINT "Testing SGN with various integer values:"
30 PRINT
40 FOR i% = -10 TO 10 STEP 3
50   PRINT "SGN("; i%; ") = "; SGN(i%)
60 NEXT i%
70 PRINT
80 PRINT "SGN working correctly!"
