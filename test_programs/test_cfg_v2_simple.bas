' Test CFG v2 with simple control structures
' This tests: IF, WHILE, FOR, PRINT

PRINT "Testing CFG v2..."

' Test 1: Simple IF
DIM x AS INTEGER
x = 5
IF x > 3 THEN
    PRINT "x is greater than 3"
ELSE
    PRINT "x is not greater than 3"
END IF

' Test 2: WHILE loop
DIM i AS INTEGER
i = 1
WHILE i <= 3
    PRINT "WHILE: i ="; i
    i = i + 1
WEND

' Test 3: FOR loop
DIM j AS INTEGER
FOR j = 1 TO 3
    PRINT "FOR: j ="; j
NEXT j

' Test 4: Nested IF in loop
DIM k AS INTEGER
FOR k = 1 TO 5
    IF k MOD 2 = 0 THEN
        PRINT "k ="; k; " is even"
    ELSE
        PRINT "k ="; k; " is odd"
    END IF
NEXT k

PRINT "CFG v2 test complete!"
