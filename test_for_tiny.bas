REM Tiny nested FOR test
FOR i% = 1 TO 2
    PRINT i%
    FOR j% = 1 TO 2
        PRINT j%
    NEXT j%
NEXT i%
END
