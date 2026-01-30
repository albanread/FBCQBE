1000 REM Comprehensive MID$ function test
1010 DIM s AS STRING
1020 DIM s2 AS STRING
1030 DIM result AS STRING
1040 DIM i AS INTEGER
1050 DIM c1 AS STRING
1060 DIM c2 AS STRING
1070
1080 PRINT "=== Comprehensive MID$ Test Suite ==="
1090 PRINT ""
1100
1110 REM Test 1: Simple literal string
1120 PRINT "Test 1: MID$ with string variable"
1130 s = "cat"
1140 PRINT "s = '"; s; "'"
1150 PRINT "MID$(s, 1, 1) = '"; MID$(s, 1, 1); "' (should be 'c')"
1160 PRINT "MID$(s, 2, 1) = '"; MID$(s, 2, 1); "' (should be 'a')"
1170 PRINT "MID$(s, 3, 1) = '"; MID$(s, 3, 1); "' (should be 't')"
1180 PRINT ""
1190
1200 REM Test 2: Different string variable
1210 PRINT "Test 2: MID$ with different string variable"
1220 s = "hello"
1230 PRINT "s = '"; s; "'"
1240 PRINT "MID$(s, 1, 1) = '"; MID$(s, 1, 1); "' (should be 'h')"
1250 PRINT "MID$(s, 2, 1) = '"; MID$(s, 2, 1); "' (should be 'e')"
1260 PRINT "MID$(s, 3, 1) = '"; MID$(s, 3, 1); "' (should be 'l')"
1270 PRINT "MID$(s, 4, 1) = '"; MID$(s, 4, 1); "' (should be 'l')"
1280 PRINT "MID$(s, 5, 1) = '"; MID$(s, 5, 1); "' (should be 'o')"
1290 PRINT ""
1300
1310 REM Test 3: Store MID$ result in variable
1320 PRINT "Test 3: Store MID$ result in variable"
1330 s = "test"
1340 result = MID$(s, 1, 1)
1350 PRINT "s = '"; s; "'"
1360 PRINT "result = MID$(s, 1, 1)"
1370 PRINT "result = '"; result; "' (should be 't')"
1380 PRINT ""
1390
1400 REM Test 4: Multiple length extractions
1410 PRINT "Test 4: Different length extractions"
1420 s = "abcdef"
1430 PRINT "s = '"; s; "'"
1440 PRINT "MID$(s, 1, 1) = '"; MID$(s, 1, 1); "' (should be 'a')"
1450 PRINT "MID$(s, 1, 2) = '"; MID$(s, 1, 2); "' (should be 'ab')"
1460 PRINT "MID$(s, 1, 3) = '"; MID$(s, 1, 3); "' (should be 'abc')"
1470 PRINT "MID$(s, 2, 2) = '"; MID$(s, 2, 2); "' (should be 'bc')"
1480 PRINT "MID$(s, 3, 2) = '"; MID$(s, 3, 2); "' (should be 'cd')"
1490 PRINT ""
1500
1510 REM Test 5: Two different strings
1520 PRINT "Test 5: Two different string variables"
1530 s = "cat"
1540 s2 = "dog"
1550 PRINT "s = '"; s; "', s2 = '"; s2; "'"
1560 c1 = MID$(s, 1, 1)
1570 c2 = MID$(s2, 1, 1)
1580 PRINT "MID$(s, 1, 1) = '"; c1; "' (should be 'c')"
1590 PRINT "MID$(s2, 1, 1) = '"; c2; "' (should be 'd')"
1600 PRINT ""
1610
1620 REM Test 6: Comparison test
1630 PRINT "Test 6: String comparison with MID$ results"
1640 s = "abc"
1650 c1 = MID$(s, 1, 1)
1660 IF c1 = "a" THEN
1670     PRINT "MID$(s, 1, 1) = 'a' : TRUE (correct!)"
1680 ELSE
1690     PRINT "MID$(s, 1, 1) = 'a' : FALSE (incorrect!)"
1700 END IF
1710 c1 = MID$(s, 2, 1)
1720 IF c1 = "b" THEN
1730     PRINT "MID$(s, 2, 1) = 'b' : TRUE (correct!)"
1740 ELSE
1750     PRINT "MID$(s, 2, 1) = 'b' : FALSE (incorrect!)"
1760 END IF
1770 PRINT ""
1780
1790 REM Test 7: Character comparison like in Levenshtein
1800 PRINT "Test 7: Character-by-character comparison (Levenshtein style)"
1810 s = "cat"
1820 s2 = "bat"
1830 PRINT "Comparing '"; s; "' with '"; s2; "'"
1840 FOR i = 1 TO 3
1850     c1 = MID$(s, i, 1)
1860     c2 = MID$(s2, i, 1)
1870     PRINT "Position "; i; ": '"; c1; "' vs '"; c2; "'";
1880     IF c1 = c2 THEN
1890         PRINT " - MATCH"
1900     ELSE
1910         PRINT " - DIFFER"
1920     END IF
1930 NEXT i
1940 PRINT ""
1950
1960 REM Test 8: Edge cases
1970 PRINT "Test 8: Edge cases"
1980 s = "x"
1990 PRINT "Single character string: '"; s; "'"
2000 PRINT "MID$(s, 1, 1) = '"; MID$(s, 1, 1); "' (should be 'x')"
2010 PRINT ""
2020
2030 PRINT "=== All tests completed ==="
2040 END
