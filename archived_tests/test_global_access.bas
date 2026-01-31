GLOBAL x&, y&, sum&
LET sum& = 0
LET x& = 1
WHILE x& <= 10
    LET sum& = sum& + x&
    LET x& = x& + 1
WEND
PRINT sum&
END
