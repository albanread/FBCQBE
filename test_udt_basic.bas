' Test User-Defined Types (UDT)
' Basic struct/record support

PRINT "Test 1: Simple Point type"
TYPE Point
    X AS DOUBLE
    Y AS DOUBLE
END TYPE

DIM P AS Point
P.X = 10.5
P.Y = 20.5
PRINT "Point P: ("; P.X; ", "; P.Y; ")"

PRINT "Test 2: Nested types"
TYPE Vector2D
    X AS DOUBLE
    Y AS DOUBLE
END TYPE

TYPE Entity
    Name AS STRING
    Position AS Vector2D
    Health AS INTEGER
END TYPE

DIM Player AS Entity
Player.Name = "Hero"
Player.Position.X = 100
Player.Position.Y = 200
Player.Health = 100

PRINT "Player: "; Player.Name
PRINT "Position: ("; Player.Position.X; ", "; Player.Position.Y; ")"
PRINT "Health: "; Player.Health

PRINT "Test 3: Mixed types"
TYPE Sprite
    X AS DOUBLE
    Y AS DOUBLE
    Active AS INTEGER
    Name AS STRING
END TYPE

DIM S AS Sprite
S.X = 50
S.Y = 75
S.Active = 1
S.Name = "Enemy"

PRINT "Sprite: "; S.Name
PRINT "Position: ("; S.X; ", "; S.Y; ")"
PRINT "Active: "; S.Active

PRINT "All UDT tests completed!"
