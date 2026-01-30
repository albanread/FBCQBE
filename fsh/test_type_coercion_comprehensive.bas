' test_type_coercion_comprehensive.bas
' Comprehensive test for type coercion on assignment
' Tests all combinations of source types → destination types
' to ensure proper conversion code is generated

PRINT "=== Comprehensive Type Coercion Assignment Test ==="
PRINT ""

' =============================================================================
' Test 1: INTEGER (word) → DOUBLE
' =============================================================================
PRINT "Test 1: INTEGER → DOUBLE"
DIM iw AS INTEGER
DIM d1 AS DOUBLE

iw = 42
d1 = iw
PRINT "  INT variable to DOUBLE: "; d1
IF d1 = 42.0 THEN PRINT "  ✓ PASS" ELSE PRINT "  ✗ FAIL"

d1 = 100
PRINT "  INT literal to DOUBLE: "; d1
IF d1 = 100.0 THEN PRINT "  ✓ PASS" ELSE PRINT "  ✗ FAIL"

d1 = ASC("A")
PRINT "  ASC() to DOUBLE: "; d1
IF d1 = 65.0 THEN PRINT "  ✓ PASS" ELSE PRINT "  ✗ FAIL"

d1 = LEN("Hello")
PRINT "  LEN() to DOUBLE: "; d1
IF d1 = 5.0 THEN PRINT "  ✓ PASS" ELSE PRINT "  ✗ FAIL"

d1 = 10 + 20
PRINT "  INT arithmetic to DOUBLE: "; d1
IF d1 = 30.0 THEN PRINT "  ✓ PASS" ELSE PRINT "  ✗ FAIL"
PRINT ""

' =============================================================================
' Test 2: INTEGER (word) → SINGLE (float)
' =============================================================================
PRINT "Test 2: INTEGER → SINGLE"
DIM s1 AS SINGLE

s1 = iw
PRINT "  INT variable to SINGLE: "; s1
IF s1 = 42.0 THEN PRINT "  ✓ PASS" ELSE PRINT "  ✗ FAIL"

s1 = 99
PRINT "  INT literal to SINGLE: "; s1
IF s1 = 99.0 THEN PRINT "  ✓ PASS" ELSE PRINT "  ✗ FAIL"

s1 = ASC("Z")
PRINT "  ASC() to SINGLE: "; s1
IF s1 = 90.0 THEN PRINT "  ✓ PASS" ELSE PRINT "  ✗ FAIL"

s1 = 5 * 6
PRINT "  INT arithmetic to SINGLE: "; s1
IF s1 = 30.0 THEN PRINT "  ✓ PASS" ELSE PRINT "  ✗ FAIL"
PRINT ""

' =============================================================================
' Test 3: LONG (long) → DOUBLE
' =============================================================================
PRINT "Test 3: LONG → DOUBLE"
DIM lng AS LONG

lng = 1000000
d1 = lng
PRINT "  LONG variable to DOUBLE: "; d1
IF d1 = 1000000.0 THEN PRINT "  ✓ PASS" ELSE PRINT "  ✗ FAIL"

d1 = 9999999&
PRINT "  LONG literal to DOUBLE: "; d1
IF d1 = 9999999.0 THEN PRINT "  ✓ PASS" ELSE PRINT "  ✗ FAIL"
PRINT ""

' =============================================================================
' Test 4: LONG (long) → SINGLE
' =============================================================================
PRINT "Test 4: LONG → SINGLE"

s1 = lng
PRINT "  LONG variable to SINGLE: "; s1
IF s1 = 1000000.0 THEN PRINT "  ✓ PASS" ELSE PRINT "  ✗ FAIL"

s1 = 123456&
PRINT "  LONG literal to SINGLE: "; s1
IF s1 = 123456.0 THEN PRINT "  ✓ PASS" ELSE PRINT "  ✗ FAIL"
PRINT ""

' =============================================================================
' Test 5: DOUBLE → INTEGER (truncation)
' =============================================================================
PRINT "Test 5: DOUBLE → INTEGER (truncation)"
DIM d2 AS DOUBLE

d2 = 42.7
iw = d2
PRINT "  DOUBLE variable to INT: "; iw
IF iw = 42 THEN PRINT "  ✓ PASS" ELSE PRINT "  ✗ FAIL"

iw = 99.9
PRINT "  DOUBLE literal to INT: "; iw
IF iw = 99 THEN PRINT "  ✓ PASS" ELSE PRINT "  ✗ FAIL"

iw = 3.14 + 2.86
PRINT "  DOUBLE arithmetic to INT: "; iw
IF iw = 6 THEN PRINT "  ✓ PASS" ELSE PRINT "  ✗ FAIL"

d2 = -42.7
iw = d2
PRINT "  Negative DOUBLE to INT: "; iw
IF iw = -42 THEN PRINT "  ✓ PASS" ELSE PRINT "  ✗ FAIL"
PRINT ""

' =============================================================================
' Test 6: DOUBLE → LONG (truncation)
' =============================================================================
PRINT "Test 6: DOUBLE → LONG (truncation)"

d2 = 1234567.89
lng = d2
PRINT "  DOUBLE variable to LONG: "; lng
IF lng = 1234567 THEN PRINT "  ✓ PASS" ELSE PRINT "  ✗ FAIL"

lng = 9876543.21
PRINT "  DOUBLE literal to LONG: "; lng
IF lng = 9876543 THEN PRINT "  ✓ PASS" ELSE PRINT "  ✗ FAIL"
PRINT ""

' =============================================================================
' Test 7: SINGLE → DOUBLE (precision extension)
' =============================================================================
PRINT "Test 7: SINGLE → DOUBLE"
DIM s2 AS SINGLE

s2 = 3.14
d1 = s2
PRINT "  SINGLE variable to DOUBLE: "; d1
IF d1 > 3.13 AND d1 < 3.15 THEN PRINT "  ✓ PASS" ELSE PRINT "  ✗ FAIL"

d1 = 2.71!
PRINT "  SINGLE literal to DOUBLE: "; d1
IF d1 > 2.70 AND d1 < 2.72 THEN PRINT "  ✓ PASS" ELSE PRINT "  ✗ FAIL"
PRINT ""

' =============================================================================
' Test 8: SINGLE → INTEGER
' =============================================================================
PRINT "Test 8: SINGLE → INTEGER"

s2 = 77.5
iw = s2
PRINT "  SINGLE variable to INT: "; iw
IF iw = 77 THEN PRINT "  ✓ PASS" ELSE PRINT "  ✗ FAIL"

iw = 88.9!
PRINT "  SINGLE literal to INT: "; iw
IF iw = 88 THEN PRINT "  ✓ PASS" ELSE PRINT "  ✗ FAIL"
PRINT ""

' =============================================================================
' Test 9: DOUBLE → SINGLE (precision truncation)
' =============================================================================
PRINT "Test 9: DOUBLE → SINGLE"

d2 = 1.23456789
s1 = d2
PRINT "  DOUBLE variable to SINGLE: "; s1
IF s1 > 1.2 AND s1 < 1.3 THEN PRINT "  ✓ PASS" ELSE PRINT "  ✗ FAIL"

s1 = 9.87654321
PRINT "  DOUBLE literal to SINGLE: "; s1
IF s1 > 9.8 AND s1 < 9.9 THEN PRINT "  ✓ PASS" ELSE PRINT "  ✗ FAIL"
PRINT ""

' =============================================================================
' Test 10: INTEGER → LONG (extension)
' =============================================================================
PRINT "Test 10: INTEGER → LONG"

iw = 32767
lng = iw
PRINT "  INT variable to LONG: "; lng
IF lng = 32767 THEN PRINT "  ✓ PASS" ELSE PRINT "  ✗ FAIL"

lng = 12345
PRINT "  INT literal to LONG: "; lng
IF lng = 12345 THEN PRINT "  ✓ PASS" ELSE PRINT "  ✗ FAIL"

lng = ASC("X")
PRINT "  ASC() to LONG: "; lng
IF lng = 88 THEN PRINT "  ✓ PASS" ELSE PRINT "  ✗ FAIL"
PRINT ""

' =============================================================================
' Test 11: LONG → INTEGER (truncation/cast)
' =============================================================================
PRINT "Test 11: LONG → INTEGER"

lng = 1000&
iw = lng
PRINT "  LONG variable to INT: "; iw
IF iw = 1000 THEN PRINT "  ✓ PASS" ELSE PRINT "  ✗ FAIL"

iw = 5000&
PRINT "  LONG literal to INT: "; iw
IF iw = 5000 THEN PRINT "  ✓ PASS" ELSE PRINT "  ✗ FAIL"
PRINT ""

' =============================================================================
' Test 12: Mixed arithmetic expressions
' =============================================================================
PRINT "Test 12: Mixed Arithmetic Expressions"

' INT + INT → DOUBLE
d1 = 10 + 20
PRINT "  (INT + INT) to DOUBLE: "; d1
IF d1 = 30.0 THEN PRINT "  ✓ PASS" ELSE PRINT "  ✗ FAIL"

' INT * INT → DOUBLE
d1 = 7 * 8
PRINT "  (INT * INT) to DOUBLE: "; d1
IF d1 = 56.0 THEN PRINT "  ✓ PASS" ELSE PRINT "  ✗ FAIL"

' DOUBLE + DOUBLE → INT (truncation)
iw = 3.5 + 2.7
PRINT "  (DOUBLE + DOUBLE) to INT: "; iw
IF iw = 6 THEN PRINT "  ✓ PASS" ELSE PRINT "  ✗ FAIL"

' Function result + literal → different type
d1 = ASC("A") + 10
PRINT "  (ASC() + INT) to DOUBLE: "; d1
IF d1 = 75.0 THEN PRINT "  ✓ PASS" ELSE PRINT "  ✗ FAIL"

d1 = LEN("Test") * 2
PRINT "  (LEN() * INT) to DOUBLE: "; d1
IF d1 = 8.0 THEN PRINT "  ✓ PASS" ELSE PRINT "  ✗ FAIL"
PRINT ""

' =============================================================================
' Test 13: Function return type coercions
' =============================================================================
PRINT "Test 13: Function Returns"

' ASC returns word, various destinations
iw = ASC("B")
IF iw = 66 THEN PRINT "  ✓ PASS: ASC() to INT" ELSE PRINT "  ✗ FAIL: ASC() to INT"

lng = ASC("C")
IF lng = 67 THEN PRINT "  ✓ PASS: ASC() to LONG" ELSE PRINT "  ✗ FAIL: ASC() to LONG"

d1 = ASC("D")
IF d1 = 68.0 THEN PRINT "  ✓ PASS: ASC() to DOUBLE" ELSE PRINT "  ✗ FAIL: ASC() to DOUBLE"

s1 = ASC("E")
IF s1 = 69.0 THEN PRINT "  ✓ PASS: ASC() to SINGLE" ELSE PRINT "  ✗ FAIL: ASC() to SINGLE"

' LEN returns word, various destinations
iw = LEN("Hello")
IF iw = 5 THEN PRINT "  ✓ PASS: LEN() to INT" ELSE PRINT "  ✗ FAIL: LEN() to INT"

lng = LEN("World")
IF lng = 5 THEN PRINT "  ✓ PASS: LEN() to LONG" ELSE PRINT "  ✗ FAIL: LEN() to LONG"

d1 = LEN("FasterBASIC")
IF d1 = 11.0 THEN PRINT "  ✓ PASS: LEN() to DOUBLE" ELSE PRINT "  ✗ FAIL: LEN() to DOUBLE"

s1 = LEN("Test")
IF s1 = 4.0 THEN PRINT "  ✓ PASS: LEN() to SINGLE" ELSE PRINT "  ✗ FAIL: LEN() to SINGLE"
PRINT ""

' =============================================================================
' Test 14: Negative numbers
' =============================================================================
PRINT "Test 14: Negative Number Coercions"

iw = -42
d1 = iw
PRINT "  Negative INT to DOUBLE: "; d1
IF d1 = -42.0 THEN PRINT "  ✓ PASS" ELSE PRINT "  ✗ FAIL"

d2 = -3.14
iw = d2
PRINT "  Negative DOUBLE to INT: "; iw
IF iw = -3 THEN PRINT "  ✓ PASS" ELSE PRINT "  ✗ FAIL"

lng = -1000000&
d1 = lng
PRINT "  Negative LONG to DOUBLE: "; d1
IF d1 = -1000000.0 THEN PRINT "  ✓ PASS" ELSE PRINT "  ✗ FAIL"

s2 = -5.5!
iw = s2
PRINT "  Negative SINGLE to INT: "; iw
IF iw = -5 THEN PRINT "  ✓ PASS" ELSE PRINT "  ✗ FAIL"
PRINT ""

' =============================================================================
' Test 15: Zero and edge cases
' =============================================================================
PRINT "Test 15: Zero and Edge Cases"

iw = 0
d1 = iw
PRINT "  Zero INT to DOUBLE: "; d1
IF d1 = 0.0 THEN PRINT "  ✓ PASS" ELSE PRINT "  ✗ FAIL"

d2 = 0.0
iw = d2
PRINT "  Zero DOUBLE to INT: "; iw
IF iw = 0 THEN PRINT "  ✓ PASS" ELSE PRINT "  ✗ FAIL"

' Very small double → int (should truncate to 0)
d2 = 0.9
iw = d2
PRINT "  0.9 DOUBLE to INT: "; iw
IF iw = 0 THEN PRINT "  ✓ PASS" ELSE PRINT "  ✗ FAIL"

' Very small double → int (negative, should truncate to 0)
d2 = -0.9
iw = d2
PRINT "  -0.9 DOUBLE to INT: "; iw
IF iw = 0 THEN PRINT "  ✓ PASS" ELSE PRINT "  ✗ FAIL"
PRINT ""

' =============================================================================
' Test 16: Chained coercions (multiple conversions)
' =============================================================================
PRINT "Test 16: Chained Coercions"

' INT → DOUBLE → INT
iw = 42
d1 = iw
iw = d1
PRINT "  INT → DOUBLE → INT: "; iw
IF iw = 42 THEN PRINT "  ✓ PASS" ELSE PRINT "  ✗ FAIL"

' DOUBLE → INT → DOUBLE
d2 = 77.8
iw = d2
d1 = iw
PRINT "  DOUBLE → INT → DOUBLE: "; d1
IF d1 = 77.0 THEN PRINT "  ✓ PASS" ELSE PRINT "  ✗ FAIL"

' SINGLE → DOUBLE → SINGLE
s2 = 3.14!
d1 = s2
s1 = d1
PRINT "  SINGLE → DOUBLE → SINGLE: "; s1
IF s1 > 3.1 AND s1 < 3.2 THEN PRINT "  ✓ PASS" ELSE PRINT "  ✗ FAIL"
PRINT ""

' =============================================================================
' Summary
' =============================================================================
PRINT "=== Comprehensive Type Coercion Test Complete ==="
PRINT "All assignment type coercions tested:"
PRINT "  - INTEGER ↔ DOUBLE"
PRINT "  - INTEGER ↔ SINGLE"
PRINT "  - INTEGER ↔ LONG"
PRINT "  - LONG ↔ DOUBLE"
PRINT "  - LONG ↔ SINGLE"
PRINT "  - SINGLE ↔ DOUBLE"
PRINT "  - Function returns (ASC, LEN)"
PRINT "  - Arithmetic expressions"
PRINT "  - Negative numbers"
PRINT "  - Edge cases (zero, small values)"
PRINT "  - Chained coercions"
PRINT ""
PRINT "DONE"
