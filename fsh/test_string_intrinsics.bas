REM ============================================================================
REM test_string_intrinsics.bas
REM Test Suite for FasterBASIC String Intrinsic Functions
REM Tests: LEN, ASC, CHR$, VAL with ASCII/UTF-32 dual encoding
REM ============================================================================

PRINT "=== FasterBASIC String Intrinsics Test Suite ==="
PRINT ""

REM ============================================================================
REM Test 1: LEN() - String Length (Intrinsic)
REM ============================================================================
PRINT "TEST 1: LEN() Function"
PRINT "---------------------"

DIM s1 AS STRING
s1 = "Hello"
PRINT "LEN(""Hello"") = "; LEN(s1); " (expected: 5)"

s1 = ""
PRINT "LEN("""") = "; LEN(s1); " (expected: 0)"

s1 = "A"
PRINT "LEN(""A"") = "; LEN(s1); " (expected: 1)"

s1 = "This is a longer string with spaces"
PRINT "LEN(long string) = "; LEN(s1); " (expected: 36)"

PRINT ""

REM ============================================================================
REM Test 2: ASC() - Get Character Code (Intrinsic)
REM ============================================================================
PRINT "TEST 2: ASC() Function"
PRINT "---------------------"

s1 = "A"
PRINT "ASC(""A"") = "; ASC(s1); " (expected: 65)"

s1 = "Z"
PRINT "ASC(""Z"") = "; ASC(s1); " (expected: 90)"

s1 = "a"
PRINT "ASC(""a"") = "; ASC(s1); " (expected: 97)"

s1 = "0"
PRINT "ASC(""0"") = "; ASC(s1); " (expected: 48)"

s1 = "9"
PRINT "ASC(""9"") = "; ASC(s1); " (expected: 57)"

s1 = " "
PRINT "ASC("" "") = "; ASC(s1); " (expected: 32)"

s1 = ""
PRINT "ASC("""") = "; ASC(s1); " (expected: 0)"

s1 = "Hello"
PRINT "ASC(""Hello"") = "; ASC(s1); " (expected: 72 - first char)"

PRINT ""

REM ============================================================================
REM Test 3: CHR$() - Create String from Code Point (Runtime, optimized)
REM ============================================================================
PRINT "TEST 3: CHR$() Function"
PRINT "-----------------------"

DIM c AS STRING

c = CHR$(65)
PRINT "CHR$(65) = """; c; """ (expected: ""A"")"

c = CHR$(90)
PRINT "CHR$(90) = """; c; """ (expected: ""Z"")"

c = CHR$(97)
PRINT "CHR$(97) = """; c; """ (expected: ""a"")"

c = CHR$(32)
PRINT "CHR$(32) = """; c; """ (expected: "" "")"

c = CHR$(48)
PRINT "CHR$(48) = """; c; """ (expected: ""0"")"

REM Test ASCII range (< 128) - should create ASCII string
c = CHR$(127)
PRINT "CHR$(127) = code"; ASC(c); " (expected: 127 - ASCII encoding)"

PRINT ""

REM ============================================================================
REM Test 4: VAL() - String to Number Conversion (Runtime)
REM ============================================================================
PRINT "TEST 4: VAL() Function"
PRINT "---------------------"

DIM n AS DOUBLE

s1 = "42"
n = VAL(s1)
PRINT "VAL(""42"") = "; n; " (expected: 42)"

s1 = "123.456"
n = VAL(s1)
PRINT "VAL(""123.456"") = "; n; " (expected: 123.456)"

s1 = "-99"
n = VAL(s1)
PRINT "VAL(""-99"") = "; n; " (expected: -99)"

s1 = "0"
n = VAL(s1)
PRINT "VAL(""0"") = "; n; " (expected: 0)"

s1 = "   456   "
n = VAL(s1)
PRINT "VAL(""   456   "") = "; n; " (expected: 456 - leading spaces)"

s1 = "12abc"
n = VAL(s1)
PRINT "VAL(""12abc"") = "; n; " (expected: 12 - stops at non-digit)"

s1 = "abc"
n = VAL(s1)
PRINT "VAL(""abc"") = "; n; " (expected: 0 - no digits)"

s1 = ""
n = VAL(s1)
PRINT "VAL("""") = "; n; " (expected: 0)"

PRINT ""

REM ============================================================================
REM Test 5: Combined Operations
REM ============================================================================
PRINT "TEST 5: Combined Operations"
PRINT "---------------------------"

s1 = "BASIC"
PRINT "String: "; s1
PRINT "  Length: "; LEN(s1)
PRINT "  First char code: "; ASC(s1)
PRINT "  First char: "; CHR$(ASC(s1))

DIM s2 AS STRING
s2 = CHR$(66)
s2 = s2 + CHR$(65)
s2 = s2 + CHR$(83)
s2 = s2 + CHR$(73)
s2 = s2 + CHR$(67)
PRINT "Built from CHR$: "; s2; " (expected: BASIC)"

PRINT ""

REM ============================================================================
REM Test 6: Edge Cases
REM ============================================================================
PRINT "TEST 6: Edge Cases"
PRINT "------------------"

REM Empty string tests
s1 = ""
PRINT "Empty string: LEN="""; LEN(s1); """ ASC="""; ASC(s1); """"

REM Single character
s1 = "X"
PRINT "Single char: LEN="""; LEN(s1); """ ASC="""; ASC(s1); """"

REM Numbers as strings
s1 = "12345"
PRINT "Numeric string: LEN="""; LEN(s1); """ VAL="""; VAL(s1); """"

PRINT ""

REM ============================================================================
REM Test 7: ASCII Encoding Tests (< 128)
REM ============================================================================
PRINT "TEST 7: ASCII Encoding Tests"
PRINT "----------------------------"

REM All ASCII characters should use 1-byte encoding
s1 = "Hello, World!"
PRINT "ASCII string: "; s1
PRINT "  Length: "; LEN(s1); " (expected: 13)"
PRINT "  Codes: "; ASC(CHR$(72)); ASC(CHR$(101)); ASC(CHR$(108)); " (H e l)"

REM Create pure ASCII string character by character
s1 = CHR$(65) + CHR$(66) + CHR$(67)
PRINT "ABC built: "; s1; " LEN="; LEN(s1)

PRINT ""

REM ============================================================================
REM Test 8: String Building with CHR$ and Concatenation
REM ============================================================================
PRINT "TEST 8: String Building"
PRINT "----------------------"

s1 = ""
s1 = s1 + CHR$(72)   REM H
s1 = s1 + CHR$(101)  REM e
s1 = s1 + CHR$(108)  REM l
s1 = s1 + CHR$(108)  REM l
s1 = s1 + CHR$(111)  REM o
PRINT "Built ""Hello"": "; s1
PRINT "  Final length: "; LEN(s1)
PRINT "  First code: "; ASC(s1)

PRINT ""

REM ============================================================================
REM Test 9: VAL() Edge Cases
REM ============================================================================
PRINT "TEST 9: VAL() Edge Cases"
PRINT "-----------------------"

s1 = "+123"
PRINT "VAL(""+123"") = "; VAL(s1)

s1 = "  -456.78  "
PRINT "VAL(""  -456.78  "") = "; VAL(s1)

s1 = "1e3"
PRINT "VAL(""1e3"") = "; VAL(s1); " (scientific notation)"

s1 = ".5"
PRINT "VAL("".5"") = "; VAL(s1); " (decimal without leading 0)"

PRINT ""

REM ============================================================================
REM Summary
REM ============================================================================
PRINT "=== Test Suite Complete ==="
PRINT ""
PRINT "Tested Functions:"
PRINT "  LEN(s$)  - Intrinsic: Direct descriptor field load"
PRINT "  ASC(s$)  - Intrinsic: Encoding-aware character load"
PRINT "  CHR$(n)  - Runtime:   Fast character creation"
PRINT "  VAL(s$)  - Runtime:   String to number parsing"
PRINT ""
PRINT "Encoding Support:"
PRINT "  ASCII:   1 byte/char (codes 0-127)"
PRINT "  UTF-32:  4 bytes/char (codes >= 128)"
PRINT "  Auto-promotion when Unicode detected"
PRINT ""

END
