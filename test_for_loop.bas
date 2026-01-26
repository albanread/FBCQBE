' Test FOR-NEXT loops with different type suffixes

' Byte loop
DIM sum@ AS BYTE
DIM i@ AS BYTE
sum@ = 0
FOR i@ = 1 TO 10
    sum@ = sum@ + i@
NEXT i@
PRINT "Byte sum: "; sum@

' Short loop
DIM total^ AS SHORT
DIM j^ AS SHORT
total^ = 0
FOR j^ = 1 TO 100 STEP 10
    total^ = total^ + j^
NEXT j^
PRINT "Short total: "; total^

' Integer loop (standard)
DIM count% AS INTEGER
DIM k% AS INTEGER
count% = 0
FOR k% = 10 TO 1 STEP -1
    count% = count% + 1
NEXT k%
PRINT "Count: "; count%

' Long loop
DIM big& AS LONG
DIM m& AS LONG
big& = 0
FOR m& = 1 TO 5
    big& = big& + m&
NEXT m&
PRINT "Big: "; big&
