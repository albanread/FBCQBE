' Test showing loop variable reference issue
' The loop counter is internal, but user references the variable

DIM sum AS INTEGER
sum = 0

' Simple loop - user references loop variable
FOR i = 1 TO 5
    PRINT "i = "; i
    sum = sum + i
NEXT i

PRINT "Sum = "; sum
PRINT ""

' Loop with typed variable
DIM j@ AS BYTE
FOR j@ = 1 TO 3
    PRINT "j@ = "; j@
NEXT j@

PRINT "Final j@ = "; j@
