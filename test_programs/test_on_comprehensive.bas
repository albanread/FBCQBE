' Comprehensive test for ON GOTO and ON GOSUB statements
' Tests various edge cases and scenarios

PRINT "Testing ON GOTO and ON GOSUB"
PRINT "============================="
PRINT ""

' Test 1: Basic ON GOTO
PRINT "Test 1: Basic ON GOTO"
FOR i = 1 TO 4
    PRINT "  i ="; i; ": ";
    ON i GOTO label1, label2, label3
    PRINT "Out of range (fallthrough)"
    GOTO continue1
label1:
    PRINT "Label 1"
    GOTO continue1
label2:
    PRINT "Label 2"
    GOTO continue1
label3:
    PRINT "Label 3"
continue1:
NEXT i
PRINT ""

' Test 2: Basic ON GOSUB
PRINT "Test 2: Basic ON GOSUB"
FOR i = 1 TO 4
    PRINT "  i ="; i; ": ";
    ON i GOSUB sub1, sub2, sub3
    PRINT " - Returned"
NEXT i
PRINT ""

' Test 3: ON GOTO with zero selector (should fallthrough)
PRINT "Test 3: ON GOTO with selector = 0"
x = 0
PRINT "  Before ON GOTO"
ON x GOTO err1, err2, err3
PRINT "  After ON GOTO (fallthrough OK)"
PRINT ""

' Test 4: ON GOSUB with zero selector (should fallthrough)
PRINT "Test 4: ON GOSUB with selector = 0"
x = 0
PRINT "  Before ON GOSUB"
ON x GOSUB err1, err2, err3
PRINT "  After ON GOSUB (fallthrough OK)"
PRINT ""

' Test 5: ON GOTO with negative selector
PRINT "Test 5: ON GOTO with negative selector"
x = -1
PRINT "  Before ON GOTO"
ON x GOTO err1, err2, err3
PRINT "  After ON GOTO (fallthrough OK)"
PRINT ""

' Test 6: ON GOSUB with negative selector
PRINT "Test 6: ON GOSUB with negative selector"
x = -1
PRINT "  Before ON GOSUB"
ON x GOSUB err1, err2, err3
PRINT "  After ON GOSUB (fallthrough OK)"
PRINT ""

' Test 7: ON GOTO with floating point selector
PRINT "Test 7: ON GOTO with floating point selector"
f = 2.7
PRINT "  f = 2.7 (should truncate to 2)"
ON f GOTO flt1, flt2, flt3
PRINT "  ERROR: fell through"
GOTO skip_flt
flt1:
    PRINT "  Went to label 1 (ERROR)"
    GOTO skip_flt
flt2:
    PRINT "  Went to label 2 (OK)"
    GOTO skip_flt
flt3:
    PRINT "  Went to label 3 (ERROR)"
skip_flt:
PRINT ""

' Test 8: Nested ON GOSUB
PRINT "Test 8: Nested ON GOSUB"
FOR i = 1 TO 2
    PRINT "  Outer i ="; i
    ON i GOSUB outer1, outer2
NEXT i
PRINT ""

' Test 9: ON GOSUB followed by immediate RETURN
PRINT "Test 9: ON GOSUB with immediate RETURN"
x = 1
ON x GOSUB imm_return
PRINT "  Returned from immediate RETURN"
PRINT ""

PRINT "============================="
PRINT "All tests completed!"
END

' Subroutines for Test 2
sub1:
    PRINT "In sub1";
    RETURN
sub2:
    PRINT "In sub2";
    RETURN
sub3:
    PRINT "In sub3";
    RETURN

' Subroutines for Test 8 (nested)
outer1:
    PRINT "    In outer1"
    FOR j = 1 TO 2
        ON j GOSUB inner1, inner2
    NEXT j
    PRINT "    Exiting outer1"
    RETURN

outer2:
    PRINT "    In outer2"
    RETURN

inner1:
    PRINT "      In inner1"
    RETURN

inner2:
    PRINT "      In inner2"
    RETURN

' Subroutine for Test 9
imm_return:
    RETURN

' Error labels (should not be reached)
err1:
    PRINT "ERROR: Reached err1"
    END
err2:
    PRINT "ERROR: Reached err2"
    END
err3:
    PRINT "ERROR: Reached err3"
    END
