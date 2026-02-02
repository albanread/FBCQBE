10 REM Test: Comprehensive FOR Loop Tests Inside SUB
20 REM Tests: Basic FOR, STEP, negative STEP, nested loops, EXIT FOR - all in SUB
30 REM FOR loop variables are implicitly declared - NO LOCAL needed!
40 PRINT "=== FOR Loop in SUB Comprehensive Tests ==="
50 PRINT ""
60 CALL TestBasicFor
70 CALL TestForStep2
80 CALL TestForStep3
90 CALL TestForCountDown
100 CALL TestForStepNeg2
110 CALL TestForZeroIter
120 CALL TestForSingleIter
130 CALL TestNestedFor
140 CALL TestExitFor
150 CALL TestLargeRange
160 CALL TestLoopVarAfter
170 PRINT ""
180 PRINT "=== All FOR Loop in SUB Tests PASSED ==="
190 END
200
210 SUB TestBasicFor()
220   PRINT "Test 1: Basic FOR loop 1 TO 5 (in SUB)"
230   LOCAL SUM AS INTEGER
240   SUM = 0
250   FOR I = 1 TO 5
260     PRINT I;
270     SUM = SUM + I
280   NEXT I
290   PRINT ""
300   IF SUM <> 15 THEN PRINT "ERROR: Basic FOR loop failed" : END
310   PRINT "PASS: Sum = "; SUM; " (expected 15)"
320   PRINT ""
330 END SUB
340
350 SUB TestForStep2()
360   PRINT "Test 2: FOR loop 1 TO 10 STEP 2 (in SUB)"
370   LOCAL COUNT AS INTEGER
380   COUNT = 0
390   FOR J = 1 TO 10 STEP 2
400     PRINT J;
410     COUNT = COUNT + 1
420   NEXT J
430   PRINT ""
440   IF COUNT <> 5 THEN PRINT "ERROR: STEP 2 loop failed" : END
450   PRINT "PASS: Iterations = "; COUNT; " (expected 5)"
460   PRINT ""
470 END SUB
480
490 SUB TestForStep3()
500   PRINT "Test 3: FOR loop 0 TO 12 STEP 3 (in SUB)"
510   LOCAL RESULT AS INTEGER
520   RESULT = 0
530   FOR K = 0 TO 12 STEP 3
540     PRINT K;
550     RESULT = RESULT + K
560   NEXT K
570   PRINT ""
580   IF RESULT <> 30 THEN PRINT "ERROR: STEP 3 loop failed, got "; RESULT : END
590   PRINT "PASS: Sum = "; RESULT; " (expected 30)"
600   PRINT ""
610 END SUB
620
630 SUB TestForCountDown()
640   PRINT "Test 4: FOR loop 10 TO 1 STEP -1 (in SUB)"
650   LOCAL DOWN_SUM AS INTEGER
660   DOWN_SUM = 0
670   FOR L = 10 TO 1 STEP -1
680     PRINT L;
690     DOWN_SUM = DOWN_SUM + L
700   NEXT L
710   PRINT ""
720   IF DOWN_SUM <> 55 THEN PRINT "ERROR: Negative STEP loop failed" : END
730   PRINT "PASS: Sum = "; DOWN_SUM; " (expected 55)"
740   PRINT ""
750 END SUB
760
770 SUB TestForStepNeg2()
780   PRINT "Test 5: FOR loop 10 TO 2 STEP -2 (in SUB)"
790   LOCAL COUNT2 AS INTEGER
800   COUNT2 = 0
810   FOR M = 10 TO 2 STEP -2
820     PRINT M;
830     COUNT2 = COUNT2 + 1
840   NEXT M
850   PRINT ""
860   IF COUNT2 <> 5 THEN PRINT "ERROR: STEP -2 loop failed" : END
870   PRINT "PASS: Iterations = "; COUNT2; " (expected 5)"
880   PRINT ""
890 END SUB
900
910 SUB TestForZeroIter()
920   PRINT "Test 6: FOR loop 10 TO 1 STEP 1 (should not execute, in SUB)"
930   LOCAL ZERO_COUNT AS INTEGER
940   ZERO_COUNT = 0
950   FOR N = 10 TO 1 STEP 1
960     ZERO_COUNT = ZERO_COUNT + 1
970     PRINT "ERROR: This should not execute"
980   NEXT N
990   IF ZERO_COUNT <> 0 THEN PRINT "ERROR: Zero iteration loop failed" : END
1000   PRINT "PASS: Loop correctly skipped (0 iterations)"
1010   PRINT ""
1020 END SUB
1030
1040 SUB TestForSingleIter()
1050   PRINT "Test 7: FOR loop 5 TO 5 (single iteration, in SUB)"
1060   LOCAL ONEVAL AS INTEGER
1070   ONEVAL = 0
1080   FOR O = 5 TO 5
1090     ONEVAL = O
1100     PRINT O
1110   NEXT O
1120   IF ONEVAL <> 5 THEN PRINT "ERROR: Single iteration loop failed" : END
1130   PRINT "PASS: Single iteration = "; ONEVAL
1140   PRINT ""
1150 END SUB
1160
1170 SUB TestNestedFor()
1180   PRINT "Test 8: Nested FOR loops (multiplication table, in SUB)"
1190   LOCAL NESTED_SUM AS INTEGER
1200   LOCAL PROD AS INTEGER
1210   NESTED_SUM = 0
1220   FOR P = 1 TO 3
1230     FOR Q = 1 TO 3
1240       PROD = P * Q
1250       PRINT P; "*"; Q; "="; PROD; " ";
1260       NESTED_SUM = NESTED_SUM + PROD
1270     NEXT Q
1280     PRINT ""
1290   NEXT P
1300   IF NESTED_SUM <> 36 THEN PRINT "ERROR: Nested loops failed" : END
1310   PRINT "PASS: Nested sum = "; NESTED_SUM; " (expected 36)"
1320   PRINT ""
1330 END SUB
1340
1350 SUB TestExitFor()
1360   PRINT "Test 9: EXIT FOR when S = 3 (in SUB)"
1370   LOCAL EXIT_COUNT AS INTEGER
1380   EXIT_COUNT = 0
1390   FOR S = 1 TO 10
1400     PRINT S;
1410     EXIT_COUNT = EXIT_COUNT + 1
1420     IF S = 3 THEN EXIT FOR
1430   NEXT S
1440   PRINT ""
1450   IF EXIT_COUNT <> 3 THEN PRINT "ERROR: EXIT FOR failed" : END
1460   PRINT "PASS: Exited after "; EXIT_COUNT; " iterations"
1470   PRINT ""
1480 END SUB
1490
1500 SUB TestLargeRange()
1510   PRINT "Test 10: FOR loop 1 TO 100 (in SUB)"
1520   LOCAL LARGE_SUM AS INTEGER
1530   LARGE_SUM = 0
1540   FOR T = 1 TO 100
1550     LARGE_SUM = LARGE_SUM + T
1560   NEXT T
1570   PRINT "Sum of 1 to 100 = "; LARGE_SUM
1580   IF LARGE_SUM <> 5050 THEN PRINT "ERROR: Large range failed" : END
1590   PRINT "PASS: Large sum = "; LARGE_SUM; " (expected 5050)"
1600   PRINT ""
1610 END SUB
1620
1630 SUB TestLoopVarAfter()
1640   PRINT "Test 11: Loop variable value after completion (in SUB)"
1650   FOR U = 1 TO 5
1660   NEXT U
1670   PRINT "U after loop = "; U
1680   IF U <> 6 THEN PRINT "ERROR: Loop variable wrong after loop" : END
1690   PRINT "PASS: Loop variable = "; U; " (expected 6)"
1700   PRINT ""
1710 END SUB
