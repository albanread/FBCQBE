REM Test FOR EACH...IN Loop Syntax (VB-style)
REM Demonstrates array iteration in FasterBASIC

PRINT "=== FOR EACH...IN Tests ==="
PRINT

REM Test 1: Simple FOR EACH...IN (iterate over integer array)
PRINT "Test 1: FOR EACH with integers"
DIM numbers%(5)
numbers%(0) = 10
numbers%(1) = 20
numbers%(2) = 30
numbers%(3) = 40
numbers%(4) = 50

FOR EACH n IN numbers%
    PRINT "Value: "; n
NEXT

PRINT

REM Test 2: FOR EACH with string array
PRINT "Test 2: FOR EACH with strings"
DIM colors$(3)
colors$(0) = "Red"
colors$(1) = "Green"
colors$(2) = "Blue"

FOR EACH col IN colors$
    PRINT "Color: "; col
NEXT

PRINT

REM Test 3: FOR EACH with AS type declaration (VB-style)
PRINT "Test 3: FOR EACH with AS INTEGER"
DIM values%(4)
values%(0) = 100
values%(1) = 200
values%(2) = 300
values%(3) = 400

FOR EACH val AS INTEGER IN values%
    PRINT "Integer value: "; val
NEXT

PRINT

REM Test 4: Nested FOR EACH loops
PRINT "Test 4: Nested FOR EACH"
DIM outer%(3)
DIM inner%(2)
outer%(0) = 1
outer%(1) = 2
outer%(2) = 3
inner%(0) = 10
inner%(1) = 20

FOR EACH x IN outer%
    FOR EACH y IN inner%
        PRINT x; " * "; y; " = "; x * y
    NEXT
NEXT

PRINT

REM Test 5: FOR EACH with floating point
PRINT "Test 5: FOR EACH with doubles"
DIM prices#(3)
prices#(0) = 19.99
prices#(1) = 29.99
prices#(2) = 39.99

FOR EACH prc AS DOUBLE IN prices#
    PRINT "Price: $"; prc
NEXT

PRINT

REM Test 6: Traditional FOR with STEP (for comparison)
PRINT "Test 6: Traditional FOR...TO (comparison)"
FOR i% = 1 TO 5 STEP 1
    PRINT "Index: "; i%
NEXT

PRINT

REM Test 7: Exit from FOR EACH loop
PRINT "Test 7: EXIT FOR in FOR EACH"
DIM biglist%(10)
FOR i% = 0 TO 9
    biglist%(i%) = i% * 10
NEXT

FOR EACH item IN biglist%
    PRINT "Item: "; item
    IF item >= 50 THEN
        PRINT "Found 50 or greater, exiting!"
        EXIT FOR
    END IF
NEXT

PRINT

REM Test 8: Alternative FOR...IN syntax (without EACH keyword)
PRINT "Test 8: Alternative FOR...IN syntax"
DIM nums%(3)
nums%(0) = 5
nums%(1) = 15
nums%(2) = 25

FOR num IN nums%
    PRINT "Number: "; num
NEXT

PRINT
PRINT "=== All FOR EACH tests complete! ==="
END
