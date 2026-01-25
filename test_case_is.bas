REM Test CASE IS syntax in SELECT CASE
DIM score AS INTEGER
score = 85

SELECT CASE score
    CASE IS < 60
        PRINT "Failing grade"
    CASE IS >= 90
        PRINT "Excellent!"
    CASE IS >= 80
        PRINT "Good job!"
    CASE IS >= 70
        PRINT "Passing"
    CASE ELSE
        PRINT "Below passing"
END SELECT

REM Test with different values
score = 95
SELECT CASE score
    CASE IS < 60
        PRINT "Failing grade"
    CASE IS >= 90
        PRINT "Excellent!"
    CASE IS >= 80
        PRINT "Good job!"
    CASE IS >= 70
        PRINT "Passing"
    CASE ELSE
        PRINT "Below passing"
END SELECT

score = 45
SELECT CASE score
    CASE IS < 60
        PRINT "Failing grade"
    CASE IS >= 90
        PRINT "Excellent!"
    CASE IS >= 80
        PRINT "Good job!"
    CASE IS >= 70
        PRINT "Passing"
    CASE ELSE
        PRINT "Below passing"
END SELECT