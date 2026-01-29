10 REM Test: THROW without TRY handler (should terminate)
20 PRINT "=== THROW Without Handler Test ==="
30 PRINT ""
40 PRINT "This test intentionally throws an error without a handler."
50 PRINT "The program should terminate with a runtime error."
60 PRINT ""
70 PRINT "Throwing error 999..."
80 THROW 999
90 PRINT "ERROR: This line should never be reached!"
100 END
