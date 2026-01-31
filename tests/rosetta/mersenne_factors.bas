' Mersenne Number Factor Finder
' Rosetta Code Challenge: Find factors of Mersenne numbers (2^P - 1)
'
' This program finds factors of Mersenne numbers using efficient modular
' exponentiation and the properties of Mersenne number factors.
'
' Key properties used:
' 1. Any factor q of 2^P-1 must be of the form 2kP+1 (k >= 0)
' 2. q must be 1 or 7 (mod 8)
' 3. q must be prime
'
' Test case: Find a factor of 2^929 - 1 (M929)

PRINT "=== Mersenne Number Factor Finder ==="
PRINT ""
PRINT "Finding factors of 2^929 - 1 (M929)"
PRINT ""

' Target exponent
LET P% = 929

' Find a factor using trial division with Mersenne properties
GOSUB FindFactor
END

' ============================================================================
' Subroutine: FindFactor
' Finds a factor of 2^P - 1 using optimized trial division
' ============================================================================
FindFactor:
    LET k% = 1
    LET limit% = 100000
    LET found% = 0

    PRINT "Searching for factors of the form q = 2kP + 1..."
    PRINT ""

    WHILE k% < limit% AND found% = 0
        ' Calculate potential factor q = 2kP + 1
        LET temp% = 2 * k%
        LET temp% = temp% * P%
        LET q% = temp% + 1

        ' Check if q is 1 or 7 (mod 8)
        LET mod8% = q% MOD 8

        LET check1% = 0
        IF mod8% = 1 THEN
            check1% = 1
        END IF
        LET check7% = 0
        IF mod8% = 7 THEN
            check7% = 1
        END IF

        IF check1% = 1 OR check7% = 1 THEN
            ' Check if q is prime
            GOSUB IsPrime

            IF is_prime% = 1 THEN
                ' Test if q divides 2^P - 1 by checking if 2^P mod q = 1
                GOSUB ModPow

                IF modpow_result% = 1 THEN
                    ' Found a factor!
                    PRINT "Found factor: "; q%
                    PRINT "  k = "; k%
                    PRINT "  q = 2 * "; k%; " * "; P%; " + 1 = "; q%
                    PRINT "  Verification: 2^"; P%; " mod "; q%; " = "; modpow_result%
                    PRINT ""
                    PRINT "Therefore, "; q%; " is a factor of 2^"; P%; " - 1"
                    found% = 1
                END IF
            END IF
        END IF

        LET k% = k% + 1

        ' Progress indicator every 1000 iterations
        LET check_mod% = k% MOD 1000
        IF check_mod% = 0 THEN
            PRINT "Checked k = "; k%; " (q = "; q%; ")..."
        END IF
    WEND

    IF found% = 0 THEN
        PRINT "No factor found in search range (k = 1 to "; limit%; ")"
    END IF

    RETURN

' ============================================================================
' Subroutine: ModPow
' Computes 2^P mod q using binary exponentiation
' Input: P%, q%
' Output: modpow_result%
' ============================================================================
ModPow:
    LET mp_base% = 2
    LET mp_exp% = P%
    LET mp_mod% = q%

    ' Binary exponentiation algorithm
    LET modpow_result% = 1
    LET mp_b% = mp_base% MOD mp_mod%
    LET mp_e% = mp_exp%
