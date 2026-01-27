REM Test FOR...IN Loop Syntax
REM Demonstrates array iteration in FasterBASIC

REM Test 1: Simple FOR...IN (iterate over values)
PRINT "Test 1: Simple FOR...IN"
DIM numbers%(5)
numbers%(0) = 10
numbers%(1) = 20
numbers%(2) = 30
numbers%(3) = 40
numbers%(4) = 50

FOR n IN numbers%
    PRINT "Value: "; n
NEXT

PRINT

REM Test 2: FOR...IN with index
PRINT "Test 2: FOR...IN with index"
DIM colors$(3)
colors$(0) = "Red"
colors$(1) = "Green"
colors$(2) = "Blue"

FOR color, i IN colors$
    PRINT "Color "; i; ": "; color
NEXT

PRINT

REM Test 3: Nested FOR...IN loops
PRINT "Test 3: Nested FOR...IN"
DIM outer%(3)
DIM inner%(2)
outer%(0) = 1
outer%(1) = 2
outer%(2) = 3
inner%(0) = 10
inner%(1) = 20

FOR x IN outer%
    FOR y IN inner%
        PRINT x; " * "; y; " = "; x * y
    NEXT
NEXT

PRINT "All FOR...IN tests complete!"
END
