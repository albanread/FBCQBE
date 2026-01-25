REM Test string arrays
DIM names$(5)

names$(0) = "Alice"
names$(1) = "Bob"
names$(2) = "Charlie"
names$(3) = "Diana"
names$(4) = "Eve"

PRINT "String array contents:"
FOR i = 0 TO 4
    PRINT "names$("; i; ") = "; names$(i)
NEXT i

REM Test string array assignment with slice
names$(2) = "Charles"
PRINT "After changing names$(2): "; names$(2)

END