1000 REM Detailed trace of Levenshtein algorithm
1010 REM Testing with "cat" -> "bat" (should be 1)
1020
1030 DIM str1 AS STRING
1040 DIM str2 AS STRING
1050 DIM d(101, 101) AS INTEGER
1060 DIM len1 AS INTEGER
1070 DIM len2 AS INTEGER
1080 DIM i AS INTEGER
1090 DIM j AS INTEGER
1100 DIM cost AS INTEGER
1110 DIM min1 AS INTEGER
1120 DIM min2 AS INTEGER
1130 DIM min3 AS INTEGER
1140 DIM i1 AS INTEGER
1150 DIM j1 AS INTEGER
1160 DIM dist AS INTEGER
1170 DIM c1 AS STRING
1180 DIM c2 AS STRING
1190
1200 str1 = "cat"
1210 str2 = "bat"
1220
1230 PRINT "Computing Levenshtein distance:"
1240 PRINT "str1 = '"; str1; "'"
1250 PRINT "str2 = '"; str2; "'"
1260 PRINT ""
1270
1280 len1 = LEN(str1)
1290 len2 = LEN(str2)
1300 PRINT "len1 = "; len1
1310 PRINT "len2 = "; len2
1320 PRINT ""
1330
1340 REM Initialize first column
1350 PRINT "Initializing first column:"
1360 FOR i = 0 TO len1
1370     d(i + 1, 1) = i
1380     PRINT "  d("; i + 1; ", 1) = "; i
1390 NEXT i
1400 PRINT ""
1410
1420 REM Initialize first row
1430 PRINT "Initializing first row:"
1440 FOR j = 0 TO len2
1450     d(1, j + 1) = j
1460     PRINT "  d(1, "; j + 1; ") = "; j
1470 NEXT j
1480 PRINT ""
1490
1500 REM Print initial matrix
1510 PRINT "Initial matrix:"
1520 PRINT "    ";
1530 FOR j = 1 TO len2 + 1
1540     PRINT j; " ";
1550 NEXT j
1560 PRINT ""
1570 FOR i = 1 TO len1 + 1
1580     PRINT i; ": ";
1590     FOR j = 1 TO len2 + 1
1600         PRINT d(i, j); " ";
1610     NEXT j
1620     PRINT ""
1630 NEXT i
1640 PRINT ""
1650
1660 REM Main DP loop
1670 PRINT "Main DP computation:"
1680 FOR i = 1 TO len1
1690     FOR j = 1 TO len2
1700         c1 = MID$(str1, i, 1)
1710         c2 = MID$(str2, j, 1)
1720
1730         IF c1 = c2 THEN
1740             cost = 0
1750         ELSE
1760             cost = 1
1770         END IF
1780
1790         PRINT "i="; i; " j="; j; " char1='"; c1; "' char2='"; c2; "' cost="; cost
1800
1810         i1 = i - 1
1820         j1 = j - 1
1830
1840         min1 = d(i1 + 1, j + 1) + 1
1850         min2 = d(i + 1, j1 + 1) + 1
1860         min3 = d(i1 + 1, j1 + 1) + cost
1870
1880         PRINT "  d("; i1 + 1; ","; j + 1; ")="; d(i1 + 1, j + 1); " -> deletion: "; min1
1890         PRINT "  d("; i + 1; ","; j1 + 1; ")="; d(i + 1, j1 + 1); " -> insertion: "; min2
1900         PRINT "  d("; i1 + 1; ","; j1 + 1; ")="; d(i1 + 1, j1 + 1); " -> subst: "; min3
1910
1920         IF min1 < min2 THEN
1930             IF min1 < min3 THEN
1940                 d(i + 1, j + 1) = min1
1950                 PRINT "  -> choosing min1 = "; min1
1960             ELSE
1970                 d(i + 1, j + 1) = min3
1980                 PRINT "  -> choosing min3 = "; min3
1990             END IF
2000         ELSE
2010             IF min2 < min3 THEN
2020                 d(i + 1, j + 1) = min2
2030                 PRINT "  -> choosing min2 = "; min2
2040             ELSE
2050                 d(i + 1, j + 1) = min3
2060                 PRINT "  -> choosing min3 = "; min3
2070             END IF
2080         END IF
2090
2100         PRINT "  d("; i + 1; ","; j + 1; ") = "; d(i + 1, j + 1)
2110         PRINT ""
2120     NEXT j
2130 NEXT i
2140
2150 REM Print final matrix
2160 PRINT "Final matrix:"
2170 PRINT "    ";
2180 FOR j = 1 TO len2 + 1
2190     PRINT j; " ";
2200 NEXT j
2210 PRINT ""
2220 FOR i = 1 TO len1 + 1
2230     PRINT i; ": ";
2240     FOR j = 1 TO len2 + 1
2250         PRINT d(i, j); " ";
2260     NEXT j
2270     PRINT ""
2280 NEXT i
2290 PRINT ""
2300
2310 dist = d(len1 + 1, len2 + 1)
2320 PRINT "Final distance = "; dist
2330 PRINT "Expected: 1"
2340
2350 END
