' Test Constants
' Verify compile-time constant folding and inlining

PRINT "Test 1: Numeric constants"
CONSTANT PI = 3.14159265
CONSTANT MAX_PLAYERS = 4
CONSTANT ZERO = 0

PRINT "PI = "; PI
PRINT "MAX_PLAYERS = "; MAX_PLAYERS
PRINT "ZERO = "; ZERO

PRINT "Test 2: String constants"
CONSTANT APP_NAME = "My Game"
CONSTANT VERSION = "1.0"
CONSTANT EMPTY = ""

PRINT "APP_NAME = "; APP_NAME
PRINT "VERSION = "; VERSION

PRINT "Test 3: Using constants in expressions"
radius = 5
area = PI * radius * radius
PRINT "Circle area (r=5): "; area

PRINT "Test 4: Constants in comparisons"
players = 3
IF players < MAX_PLAYERS THEN
    PRINT "Can add more players"
END IF

PRINT "Test 5: Constants in loops"
FOR i = 1 TO MAX_PLAYERS
    PRINT "Player "; i
NEXT i

PRINT "Test 6: Mixed constant expressions"
CONSTANT HALF_PI = PI / 2
CONSTANT DOUBLE_MAX = MAX_PLAYERS * 2
PRINT "HALF_PI = "; HALF_PI
PRINT "DOUBLE_MAX = "; DOUBLE_MAX

PRINT "Test 7: Constants with type inference"
x = MAX_PLAYERS + 10
y = PI + 1.5
PRINT "x (int context) = "; x
PRINT "y (double context) = "; y

PRINT "All constant tests completed!"
