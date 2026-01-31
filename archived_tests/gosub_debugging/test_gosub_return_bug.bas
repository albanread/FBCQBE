' ============================================================================
' BUG REPORT: GOSUB/RETURN in Multiline IF Blocks
' ============================================================================
' When GOSUB is called from within a multiline IF...END IF block,
' the RETURN statement appears to jump to after the END IF instead of
' returning to the statement immediately following the GOSUB.
' ============================================================================

PRINT "=== GOSUB/RETURN Bug Investigation ==="
PRINT ""

' Test 1: Single-line IF (this should work)
PRINT "Test 1: Single-line IF with GOSUB"
LET test1% = 1
IF test1% = 1 THEN GOSUB Sub1
PRINT "  After single-line IF"
PRINT ""

' Test 2: Multiline IF with GOSUB (this is buggy)
PRINT "Test 2: Multiline IF with GOSUB"
LET test2% = 1
IF test2% = 1 THEN
    PRINT "  Before GOSUB in multiline IF"
    GOSUB Sub2
    PRINT "  After GOSUB in multiline IF (BUG: THIS WON'T PRINT!)"
    PRINT "  Still in IF block (BUG: THIS WON'T PRINT EITHER!)"
END IF
PRINT "  After END IF"
PRINT ""

' Test 3: Nested multiline IF with GOSUB
PRINT "Test 3: Nested IF with GOSUB"
LET test3a% = 1
LET test3b% = 1
IF test3a% = 1 THEN
    PRINT "  In outer IF"
    IF test3b% = 1 THEN
        PRINT "    In inner IF, before GOSUB"
        GOSUB Sub3
        PRINT "    After GOSUB (BUG: THIS WON'T PRINT!)"
    END IF
    PRINT "  After inner END IF (BUG: THIS WON'T PRINT!)"
END IF
PRINT "  After outer END IF"
PRINT ""

' Test 4: WHILE loop with GOSUB in multiline IF
PRINT "Test 4: WHILE with GOSUB in multiline IF"
LET counter% = 1
WHILE counter% <= 3
    PRINT "  Loop "; counter%
    LET check% = counter% MOD 2
    IF check% = 1 THEN
        PRINT "    Before GOSUB"
        LET val% = counter%
        GOSUB Sub4
        PRINT "    After GOSUB, result="; res% ; " (BUG: THIS WON'T PRINT!)"
    END IF
    PRINT "  After IF block"
    LET counter% = counter% + 1
WEND
PRINT "  After WHILE"

END

Sub1:
    PRINT "  In Sub1"
    RETURN

Sub2:
    PRINT "    In Sub2"
    RETURN

Sub3:
    PRINT "      In Sub3"
    RETURN

Sub4:
    PRINT "      In Sub4, val="; val%
    LET res% = val% * 100
    RETURN
