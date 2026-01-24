//
// math_ops.c
// FasterBASIC QBE Runtime Library - Math Operations
//
// This file implements mathematical functions for BASIC programs.
//

#include "basic_runtime.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

// =============================================================================
// Absolute Value
// =============================================================================

int32_t basic_abs_int(int32_t x) {
    return x < 0 ? -x : x;
}

double basic_abs_double(double x) {
    return fabs(x);
}

// =============================================================================
// Square Root
// =============================================================================

double basic_sqrt(double x) {
    if (x < 0.0) {
        basic_error_msg("Square root of negative number");
        return 0.0;
    }
    return sqrt(x);
}

// =============================================================================
// Power
// =============================================================================

double basic_pow(double base, double exponent) {
    // Handle special cases
    if (base == 0.0 && exponent < 0.0) {
        basic_error_msg("Division by zero in power operation");
        return 0.0;
    }
    
    return pow(base, exponent);
}

// =============================================================================
// Trigonometric Functions
// =============================================================================

double basic_sin(double x) {
    return sin(x);
}

double basic_cos(double x) {
    return cos(x);
}

double basic_tan(double x) {
    return tan(x);
}

double basic_atan(double x) {
    return atan(x);
}

double basic_atan2(double y, double x) {
    return atan2(y, x);
}

// =============================================================================
// Logarithm and Exponential
// =============================================================================

double basic_log(double x) {
    if (x <= 0.0) {
        basic_error_msg("Logarithm of non-positive number");
        return 0.0;
    }
    return log(x);
}

double basic_exp(double x) {
    return exp(x);
}

// =============================================================================
// Random Number Generation
// =============================================================================

static bool rng_initialized = false;

double basic_rnd(void) {
    if (!rng_initialized) {
        srand((unsigned int)time(NULL));
        rng_initialized = true;
    }
    
    // Return random number between 0.0 and 1.0
    return (double)rand() / (double)RAND_MAX;
}

int32_t basic_rnd_int(int32_t min, int32_t max) {
    if (!rng_initialized) {
        srand((unsigned int)time(NULL));
        rng_initialized = true;
    }
    
    if (min > max) {
        int32_t temp = min;
        min = max;
        max = temp;
    }
    
    // Generate random integer in range [min, max]
    int32_t range = max - min + 1;
    return min + (rand() % range);
}

void basic_randomize(int32_t seed) {
    srand((unsigned int)seed);
    rng_initialized = true;
}

// =============================================================================
// Integer and Sign Functions
// =============================================================================

int32_t basic_int(double x) {
    // INT() in BASIC truncates towards negative infinity (floor)
    return (int32_t)floor(x);
}

int32_t basic_sgn(double x) {
    if (x < 0.0) return -1;
    if (x > 0.0) return 1;
    return 0;
}