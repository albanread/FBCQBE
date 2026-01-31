10 REM Test nested multiline IF/THEN/ELSE/END IF statements
20 REM This isolates the issue found in Levenshtein algorithm
30
40 DIM x AS INTEGER
50 DIM y AS INTEGER
60 DIM z AS INTEGER
70 DIM result AS INTEGER
80
90 REM Test 1: x=1, y=1, z=1
100 x = 1
110 y = 1
120 z = 1
130 result = 0
140
150 IF x < y THEN
160     IF x < z THEN
170         result = 1
180     ELSE
190         result = 2
200     END IF
210 ELSE
220     IF y < z THEN
230         result = 3
240     ELSE
250         result = 4
260     END IF
270 END IF
280
290 PRINT "Test 1: x="; x; " y="; y; " z="; z
300 PRINT "Result = "; result; " (expected 3 or 4)"
310 PRINT ""
320
330 REM Test 2: x=5, y=10, z=7
340 x = 5
350 y = 10
360 z = 7
370 result = 0
380
390 IF x < y THEN
400     IF x < z THEN
410         result = 1
420     ELSE
430         result = 2
440     END IF
450 ELSE
460     IF y < z THEN
470         result = 3
480     ELSE
490         result = 4
500     END IF
510 END IF
520
530 PRINT "Test 2: x="; x; " y="; y; " z="; z
540 PRINT "Result = "; result; " (expected 1)"
550 PRINT ""
560
570 REM Test 3: x=10, y=5, z=7
580 x = 10
590 y = 5
600 z = 7
610 result = 0
620
630 IF x < y THEN
640     IF x < z THEN
650         result = 1
660     ELSE
670         result = 2
680     END IF
690 ELSE
700     IF y < z THEN
710         result = 3
720     ELSE
730         result = 4
740     END IF
750 END IF
760
770 PRINT "Test 3: x="; x; " y="; y; " z="; z
780 PRINT "Result = "; result; " (expected 3)"
790 PRINT ""
800
810 END
