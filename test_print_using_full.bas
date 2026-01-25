REM Comprehensive PRINT USING tests
REM Current implementation: basic placeholder substitution
REM @ and # placeholders are replaced with string values

PRINT "=== Basic String Formatting ==="
DIM name$ AS STRING
name$ = "Alice"
PRINT USING "Name: @@@@@"; name$
PRINT USING "Hello, @@@@@!"; name$

PRINT ""
PRINT "=== Numeric Formatting ==="
PRINT USING "Value: #"; 42
PRINT USING "Amount: #"; 123.45
PRINT USING "Pi = #"; 3.14159

PRINT ""
PRINT "=== Multiple Placeholders ==="
DIM first$ AS STRING, last$ AS STRING
first$ = "John"
last$ = "Smith"
PRINT USING "@@@@@ @@@@@"; first$; last$

PRINT ""
PRINT "=== Mixed Types ==="
PRINT USING "Name: @@@@, Age: #"; name$; 25
