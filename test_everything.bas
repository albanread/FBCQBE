' Comprehensive FasterBASIC Test
' Tests constants, UDTs, type system, and integration

PRINT "=== FasterBASIC Comprehensive Test ==="
PRINT

' ============================================
' Test 1: Constants
' ============================================
PRINT "Test 1: Constants"
CONSTANT MYPI = 3.14159265
CONSTANT MAX_ENTITIES = 100
CONSTANT GAME_NAME = "Epic Quest"

PRINT "PI = "; PI
PRINT "MAX_ENTITIES = "; MAX_ENTITIES
PRINT "GAME_NAME = "; GAME_NAME
PRINT

' ============================================
' Test 2: User-Defined Types
' ============================================
PRINT "Test 2: User-Defined Types"

TYPE Vector2D
    X AS DOUBLE
    Y AS DOUBLE
END TYPE

TYPE Entity
    Name AS STRING
    Position AS Vector2D
    Health AS INTEGER
    Score AS DOUBLE
END TYPE

DIM Player AS Entity
Player.Name = GAME_NAME
Player.Position.X = 100
Player.Position.Y = 200
Player.Health = MAX_ENTITIES
Player.Score = MYPI * 1000

PRINT "Player.Name = "; Player.Name
PRINT "Player.Position.X = "; Player.Position.X
PRINT "Player.Position.Y = "; Player.Position.Y
PRINT "Player.Health = "; Player.Health
PRINT "Player.Score = "; Player.Score
PRINT

' ============================================
' Test 3: Type System (DOUBLE default)
' ============================================
PRINT "Test 3: Type System"

' Default to DOUBLE
x = 10
y = 20
z = x + y
PRINT "x + y (doubles) = "; z

' Explicit INTEGER
a% = 5
b% = 3
c% = a% + b%
PRINT "a% + b% (integers) = "; c%

' Mixed types
i% = 4
d = 2.5
result = i% + d
PRINT "i% + d (mixed) = "; result
PRINT

' ============================================
' Test 4: Constants in Expressions
' ============================================
PRINT "Test 4: Constants in Expressions"

radius = 5
area = MYPI * radius * radius
circumference = 2 * MYPI * radius

PRINT "Circle (r=5):"
PRINT "  Area = "; area
PRINT "  Circumference = "; circumference
PRINT

' ============================================
' Test 5: Constants in Control Flow
' ============================================
PRINT "Test 5: Constants in Control Flow"

CONSTANT THRESHOLD = 50

score = 75
IF score > THRESHOLD THEN
    PRINT "Score "; score; " exceeds threshold "; THRESHOLD
END IF

CONSTANT LOOP_MAX = 5
FOR i = 1 TO LOOP_MAX
    PRINT "Loop "; i; " of "; LOOP_MAX
NEXT i
PRINT

' ============================================
' Test 6: UDT with Constants
' ============================================
PRINT "Test 6: UDT with Constants"

CONSTANT DEFAULT_HEALTH = 100
CONSTANT SPAWN_X = 50
CONSTANT SPAWN_Y = 100

DIM Enemy AS Entity
Enemy.Name = "Goblin"
Enemy.Position.X = SPAWN_X
Enemy.Position.Y = SPAWN_Y
Enemy.Health = DEFAULT_HEALTH
Enemy.Score = 0

PRINT "Enemy spawned at ("; SPAWN_X; ", "; SPAWN_Y; ")"
PRINT "Enemy health: "; DEFAULT_HEALTH
PRINT

' ============================================
' Test 7: Type Coercion
' ============================================
PRINT "Test 7: Type Coercion"

TYPE Stats
    Level AS INTEGER
    Experience AS DOUBLE
    Rating AS DOUBLE
END TYPE

DIM PlayerStats AS Stats
PlayerStats.Level = 10.7
PlayerStats.Experience = 5000
PlayerStats.Rating = MYPI * 100

PRINT "Level (truncated): "; PlayerStats.Level
PRINT "Experience: "; PlayerStats.Experience
PRINT "Rating: "; PlayerStats.Rating
PRINT

' ============================================
' Test 8: Nested UDT with Constants
' ============================================
PRINT "Test 8: Nested UDT with Constants"

CONSTANT GRID_SIZE = 10

TYPE Cell
    Value AS INTEGER
    Active AS INTEGER
END TYPE

TYPE Grid
    Width AS INTEGER
    Height AS INTEGER
END TYPE

DIM GameGrid AS Grid
GameGrid.Width = GRID_SIZE
GameGrid.Height = GRID_SIZE

PRINT "Grid dimensions: "; GRID_SIZE; " x "; GRID_SIZE
PRINT

' ============================================
' Test 9: String Constants
' ============================================
PRINT "Test 9: String Constants"

CONSTANT GREETING = "Hello"
CONSTANT FAREWELL = "Goodbye"

message$ = GREETING + " from " + GAME_NAME
PRINT message$

CONSTANT EMPTY = ""
IF EMPTY = "" THEN
    PRINT "Empty string constant works"
END IF
PRINT

' ============================================
' Final Summary
' ============================================
PRINT "=== All Tests Completed Successfully ==="
PRINT "Features tested:"
PRINT "  - Constants (numeric and string)"
PRINT "  - User-Defined Types (nested)"
PRINT "  - Default DOUBLE type system"
PRINT "  - Type coercion"
PRINT "  - Integration of all features"
