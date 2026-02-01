10 REM Test multiple GOSUB call sites
20 DIM x AS INTEGER
30 DIM result AS INTEGER
40
50 x = 10
60 PRINT "First call:"
70 GOSUB 1000
80 PRINT "Returned, result="; result
90
100 x = 20
110 PRINT "Second call:"
120 GOSUB 1000
130 PRINT "Returned, result="; result
140
150 x = 30
160 PRINT "Third call:"
170 GOSUB 1000
180 PRINT "Returned, result="; result
190
200 END
210
1000 REM Subroutine: double x and store in result
1010 PRINT "  In subroutine, x="; x
1020 result = x * 2
1030 PRINT "  Computed result="; result
1040 RETURN
