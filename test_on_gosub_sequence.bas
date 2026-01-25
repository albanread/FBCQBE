10 PRINT "ON GOSUB sequence test"
20 FOR I = 1 TO 3
30 ON I GOSUB sub1, sub2, sub3
40 PRINT "Returned from branch"; I
50 NEXT I
60 PRINT "All branches complete"
70 END

sub1:
80 PRINT "Subroutine 1 hit"
90 RETURN

sub2:
100 PRINT "Subroutine 2 hit"
110 RETURN

sub3:
120 PRINT "Subroutine 3 hit"
130 RETURN
