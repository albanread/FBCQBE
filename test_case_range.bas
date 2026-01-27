10 REM Simple test for CASE x TO y range syntax
20 PRINT "Testing CASE ranges"
30 LET X% = 5
40 SELECT CASE X%
50   CASE 1 TO 3
60     PRINT "X is 1-3"
70   CASE 4 TO 6
80     PRINT "X is 4-6"
90   CASE 7 TO 10
100    PRINT "X is 7-10"
110  CASE ELSE
120    PRINT "X is something else"
130 END SELECT
140 PRINT "Done"
150 END
