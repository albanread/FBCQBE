10 REM Test EXIT FOR
20 PRINT "Finding first number > 50:"
30 FOR I = 1 TO 100
40   PRINT "Checking "; I
50   IF I * I > 50 THEN EXIT FOR
60 NEXT I
70 PRINT "Found: "; I; " ("; I; " * "; I; " = "; I * I; ")"
80 END
