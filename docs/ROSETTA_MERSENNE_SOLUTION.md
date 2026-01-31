# Rosetta Code: Factors of a Mersenne Number - Solution

## Challenge
Find a factor of 2^929 - 1 (M929) using the optimized trial division method described in the Rosetta Code challenge.

## Solution
**Factor found: 13007**

### Verification
- Formula: q = 2kP + 1 where k=7, P=929
- Calculation: q = 2 × 7 × 929 + 1 = 13007
- Properties satisfied:
  - 13007 ≡ 7 (mod 8) ✓
  - 13007 is prime ✓
  - 2^929 ≡ 1 (mod 13007) ✓

Therefore, 13007 divides 2^929 - 1.

## Implementation
Language: FasterBASIC  
File: `tests/rosetta/mersenne_factors.bas`

The implementation uses:
1. **Modular exponentiation** (binary exponentiation) to efficiently compute 2^P mod q
2. **Mersenne number properties** to optimize the search:
   - Only test q = 2kP + 1
   - Only test q ≡ 1 or 7 (mod 8)
   - Only test prime q
3. **Long integer types** (&) to handle large numbers

## Running the Solution
```bash
# Compile
./qbe_basic -o mersenne_factors tests/rosetta/mersenne_factors.bas

# Run
./mersenne_factors
```

## Output
```
=== Mersenne Number Factor Finder ===

Finding factors of 2^929 - 1 (M929)

Searching for factors of the form q = 2kP + 1...

======================================
FOUND FACTOR!
======================================
Factor: 13007
k = 7
2^929 mod 13007 = 1

Therefore 13007 is a factor of M929
======================================
```

## Bug Fixed
During development, we discovered and fixed a compiler bug where GOSUB/RETURN from within multiline IF blocks would not return to the correct location. See `BUG_FIX_SUMMARY.md` for details.
