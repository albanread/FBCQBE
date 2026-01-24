10 REM Test nested WHILE...WEND loops
20 LET I% = 1
30 WHILE I% <= 3
40   PRINT "Outer loop: "; I%
50   LET J% = 1
60   WHILE J% <= 2
70     PRINT "  Inner loop: "; J%
80     LET J% = J% + 1
90   WEND
100  LET I% = I% + 1
110 WEND
120 PRINT "Done!"
130 END
