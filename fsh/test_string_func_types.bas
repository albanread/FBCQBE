' Test that string functions like TRIM$, LTRIM$, RTRIM$, UCASE$, LCASE$
' correctly return STRING type, not DOUBLE/FLOAT
' This tests the fix for the parser name mangling issue where TRIM$ becomes TRIM_STRING

DIM s$ AS STRING
DIM result$ AS STRING

' Test TRIM$ - should return STRING, not FLOAT
s$ = "  hello  "
result$ = TRIM$(s$)
PRINT "TRIM$: '"; result$; "'"

' Test LTRIM$ - should return STRING, not FLOAT
s$ = "  left spaces"
result$ = LTRIM$(s$)
PRINT "LTRIM$: '"; result$; "'"

' Test RTRIM$ - should return STRING, not FLOAT
s$ = "right spaces  "
result$ = RTRIM$(s$)
PRINT "RTRIM$: '"; result$; "'"

' Test UCASE$ - should return STRING, not FLOAT
s$ = "lowercase"
result$ = UCASE$(s$)
PRINT "UCASE$: '"; result$; "'"

' Test LCASE$ - should return STRING, not FLOAT
s$ = "UPPERCASE"
result$ = LCASE$(s$)
PRINT "LCASE$: '"; result$; "'"

' Test chaining these functions
s$ = "  Mixed Case  "
result$ = UCASE$(TRIM$(s$))
PRINT "UCASE$(TRIM$()): '"; result$; "'"

' Test assignment directly (no intermediate variable)
DIM direct$ AS STRING
direct$ = TRIM$("  spaces  ")
PRINT "Direct: '"; direct$; "'"

PRINT "All string function type tests passed!"
END
