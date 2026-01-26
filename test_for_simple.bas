' Simple FOR loop test - no type suffixes
' Loop variables should be plain names

DIM total AS INTEGER

' Simple loop with plain variable
total = 0
FOR i = 1 TO 10
    total = total + i
NEXT i
PRINT "Total: "; total

' Loop with step
FOR j = 0 TO 100 STEP 10
    PRINT j
NEXT j

' Loop with negative step
FOR k = 10 TO 1 STEP -1
    PRINT k
NEXT k
