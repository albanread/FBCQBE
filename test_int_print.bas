10 REM Test various integer prints to ensure sign extension works
20 DIM a AS INTEGER
30 DIM b AS INTEGER
40 a = -100
50 b = 100
60 PRINT "Negative: "; a
70 PRINT "Positive: "; b
80 PRINT "Expression: "; a + b
90 PRINT "Zero: "; 0
100 PRINT "Max negative: "; -32768
110 PRINT "Max positive: "; 32767
120 END
