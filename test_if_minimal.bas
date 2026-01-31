10 REM Minimal test for nested IF statements
20 DIM x AS INTEGER
30 DIM result AS INTEGER
40
50 x = 5
60 result = 0
70
80 IF x > 0 THEN
90     result = 1
100 END IF
110
120 PRINT "Result = "; result; " (expected 1)"
130
140 END
