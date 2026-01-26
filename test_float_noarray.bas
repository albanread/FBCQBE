' Test float operations without arrays
DIM f! AS SINGLE
DIM d# AS DOUBLE

f! = 1.5
d# = 3.14159

PRINT "Single float:"
PRINT f!

PRINT ""
PRINT "Double float:"
PRINT d#

PRINT ""
PRINT "After math:"
f! = f! * 2.0
d# = d# * 2.0
PRINT f!
PRINT d#
