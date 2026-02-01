REM Simple EXIT DO test
DIM i AS INTEGER
DIM result AS INTEGER

i = 0
DO WHILE i < 10
    i = i + 1
    IF i = 5 THEN EXIT DO
LOOP

result = i
PRINT "After loop: i="; result
PRINT "Expected: i=5"

END
