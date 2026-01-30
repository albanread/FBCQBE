1000 REM Debug version of Levenshtein to check array initialization
1010 DIM d(5, 5) AS INTEGER
1020 DIM i AS INTEGER
1030 DIM j AS INTEGER
1040
1050 REM Check if array is zero-initialized
1060 PRINT "Array initialization check:"
1070 FOR i = 1 TO 5
1080     FOR j = 1 TO 5
1090         PRINT d(i, j); " ";
1100     NEXT j
1110     PRINT ""
1120 NEXT i
1130 PRINT ""
1140
1150 REM Simple initialization test
1160 FOR i = 0 TO 3
1170     d(i + 1, 1) = i
1180     PRINT "d("; i + 1; ", 1) = "; d(i + 1, 1)
1190 NEXT i
1200 PRINT ""
1210
1220 FOR j = 0 TO 3
1230     d(1, j + 1) = j
1240     PRINT "d(1, "; j + 1; ") = "; d(1, j + 1)
1250 NEXT j
1260 PRINT ""
1270
1280 REM Print full matrix
1290 PRINT "Full matrix after initialization:"
1300 FOR i = 1 TO 4
1310     FOR j = 1 TO 4
1320         PRINT d(i, j); " ";
1330     NEXT j
1340     PRINT ""
1350 NEXT i
1360
1370 END
