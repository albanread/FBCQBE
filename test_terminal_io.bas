REM Terminal I/O Test - CLS, COLOR, LOCATE
REM Tests all terminal control commands

CLS
PRINT "=== Terminal I/O Test ==="
PRINT ""

REM Test COLOR command
COLOR 2, 0
PRINT "This is green text"
COLOR 7, 0
PRINT "Back to white"
PRINT ""

REM Test LOCATE command
LOCATE 10, 20
PRINT "Text at row 10, col 20"
LOCATE 12, 1
PRINT "Back to left side"
PRINT ""

REM Test CSRLIN and POS
LOCATE 15, 1
PRINT "Current position: Row "; CSRLIN; ", Col "; POS(0)

REM Test INKEY$ for keyboard input
LOCATE 17, 1
PRINT "Press any key to continue (or wait)..."
DIM k$ AS STRING
k$ = INKEY$
IF k$ <> "" THEN
    PRINT "You pressed: "; k$
ELSE
    PRINT "No key pressed yet"
END IF

REM Test WIDTH
WIDTH 80
PRINT "Terminal width set to 80 columns"

LOCATE 20, 1
COLOR 7, 0
PRINT "Test complete!"
