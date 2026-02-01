10 REM Test nested GOSUB (subroutine calling another subroutine)
20 DIM x AS INTEGER
30 DIM y AS INTEGER
40
50 x = 5
60 y = 10
70 PRINT "Main: x="; x; " y="; y
80 GOSUB 1000
90 PRINT "Main: Back from outer subroutine"
100 PRINT "Main: x="; x; " y="; y
110 END
120
1000 REM Outer subroutine
1010 PRINT "Outer: x="; x; " y="; y
1020 x = x + 1
1030 PRINT "Outer: Calling inner subroutine"
1040 GOSUB 2000
1050 PRINT "Outer: Back from inner subroutine"
1060 PRINT "Outer: x="; x; " y="; y
1070 RETURN
1080
2000 REM Inner subroutine
2010 PRINT "Inner: x="; x; " y="; y
2020 y = y + 1
2030 x = x * 2
2040 PRINT "Inner: x="; x; " y="; y
2050 RETURN
