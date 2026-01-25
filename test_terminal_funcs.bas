REM Test terminal functions without CLS
PRINT "=== Terminal Functions Test ==="

REM Test CSRLIN and POS
PRINT "Row: "; CSRLIN; ", Col: "; POS(0)

REM Test INKEY$
PRINT "Testing INKEY$ (non-blocking)..."
DIM k$ AS STRING
k$ = INKEY$
IF k$ = "" THEN
    PRINT "No key pressed (correct!)"
ELSE
    PRINT "Key pressed: "; k$
END IF

PRINT "Test complete!"
