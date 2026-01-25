10 REM Test ON GOSUB with labels
20 X = 2
30 ON X GOSUB sub1, sub2, sub3
40 PRINT "Back from GOSUB"
50 END

sub1:
60 PRINT "Subroutine 1"
70 RETURN

sub2:
80 PRINT "Subroutine 2"
90 RETURN

sub3:
100 PRINT "Subroutine 3"
110 RETURN