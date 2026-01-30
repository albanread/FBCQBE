1000 REM Test MID$ function
1010 DIM s AS STRING
1020 DIM result AS STRING
1030 DIM i AS INTEGER
1040
1050 s = "cat"
1060 PRINT "Testing MID$ with string: '"; s; "'"
1070 PRINT "Length: "; LEN(s)
1080 PRINT ""
1090
1100 PRINT "MID$(s, 1, 1) = '"; MID$(s, 1, 1); "' (should be 'c')"
1110 PRINT "MID$(s, 2, 1) = '"; MID$(s, 2, 1); "' (should be 'a')"
1120 PRINT "MID$(s, 3, 1) = '"; MID$(s, 3, 1); "' (should be 't')"
1130 PRINT ""
1140
1150 s = "bat"
1160 PRINT "Testing MID$ with string: '"; s; "'"
1170 PRINT "MID$(s, 1, 1) = '"; MID$(s, 1, 1); "' (should be 'b')"
1180 PRINT "MID$(s, 2, 1) = '"; MID$(s, 2, 1); "' (should be 'a')"
1190 PRINT "MID$(s, 3, 1) = '"; MID$(s, 3, 1); "' (should be 't')"
1200 PRINT ""
1210
1220 REM Test in a loop
1230 s = "hello"
1240 PRINT "Testing MID$ in loop with '"; s; "':"
1250 FOR i = 1 TO LEN(s)
1260     result = MID$(s, i, 1)
1270     PRINT "  Position "; i; ": '"; result; "'"
1280 NEXT i
1290
1300 END
