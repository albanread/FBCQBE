REM Test PRINT USING
DIM name$ AS STRING
name$ = "Alice"
PRINT name$
PRINT USING "Name: @@@@@"; name$
PRINT USING "Val: #"; 123
PRINT USING "Pi= #.##"; 3.14159
