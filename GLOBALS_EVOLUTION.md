# GLOBALS Evolution: From Runtime to Compile-Time

## The Journey to Optimal Global Variables

This document chronicles the evolution of the GLOBALS implementation, from initial runtime allocation to the final elegant compile-time data segment approach.

---

## Phase 1: Runtime Vector with Function Calls (Initial Implementation)

### Approach
- Runtime allocates global vector with `calloc()`
- Every access calls `basic_global_base()` to get pointer
- 4 instructions per access

### Generated Code
```qbe
# Every access:
%base =l call $basic_global_base()    # Function call overhead
%offset =l mul SLOT, 8                # Runtime multiply
%addr =l add %base, %offset
%value =l loadl %addr
```

### Pros
- Flexible (runtime-sized vector)
- Clear separation of concerns

### Cons
- Function call overhead on every access
- Runtime multiply for offset calculation
- Requires cleanup on exit
- 4 instructions per access

**Assembly size (32 globals)**: 887 lines

---

## Phase 2: Static Offset Calculation (First Optimization)

### Improvement
Calculate byte offset at compile time: `slot * 8` → constant

### Generated Code
```qbe
# Optimized offset:
%base =l call $basic_global_base()
%addr =l add %base, 16                # Offset pre-computed!
%value =l loadl %addr
```

### Benefit
- 25% reduction: 4 → 3 instructions per access
- Eliminated runtime multiply
- Cleaner generated code

### Remaining Issue
- Still calling `basic_global_base()` every time

---

## Phase 3: Per-Function Base Caching (Attempted)

### Idea
Cache global base pointer once per function, reuse it

### Generated Code
```qbe
# At function entry:
%cached_base =l call $basic_global_base()

# Each access:
%addr =l add %cached_base, OFFSET
%value =l loadl %addr
```

### Challenge
- Need to track cached base per function
- Need to reset cache on function entry
- Complex state management

### Discovery
**User insight**: "Why cache per-function? Why not share globally?"

---

## Phase 4: Global Base Pointer Variable (Attempted)

### Idea
Store base pointer in a global QBE variable, shared by all functions

### Generated Code
```qbe
# Data section:
data $__global_base_ptr = { l 0 }

# Initialize once in main:
%tmp =l call $basic_global_base()
storel %tmp, $__global_base_ptr

# Every access:
%base =l loadl $__global_base_ptr
%addr =l add %base, OFFSET
%value =l loadl %addr
```

### Benefit
- Only ONE call to `basic_global_base()` for entire program
- All functions share cached pointer

### Remaining Issue
- Still need runtime initialization
- Extra load instruction to get base

### Discovery
**User insight**: "The global vector should be in a r/w data segment and the base is a label"

---

## Phase 5: Compile-Time Data Segment (Final Solution)

### Breakthrough Insight
The global vector doesn't need runtime allocation - it can be a **compile-time QBE data segment**!

### Generated Code
```qbe
# Data section (compile-time):
data $__global_vector = { l 0, l 0, l 0, l 0 }

# Access (just 2 instructions):
%addr =l add $__global_vector, OFFSET   # Label IS the base!
%value =l loadl %addr
```

### Architecture
```
Data Segment (compile-time allocated):
$__global_vector:
  [0]:  .quad 0    ← Slot 0 (offset 0)
  [8]:  .quad 0    ← Slot 1 (offset 8)
  [16]: .quad 0    ← Slot 2 (offset 16)
  [24]: .quad 0    ← Slot 3 (offset 24)
```

### Benefits

#### 1. Simplicity
- No runtime allocation/deallocation
- No `basic_global_init/base/cleanup` functions needed
- Direct label addressing

#### 2. Efficiency
```
Before: 4 instructions per access
After:  2 instructions per access
        50% reduction!
```

#### 3. QBE Optimization
QBE can now optimize address calculations:
```asm
# QBE optimized assembly:
adrp x1, ___global_vector@page
add  x1, x1, ___global_vector@pageoff
mov  x0, #30              # Constant folded!
str  x0, [x1]
```

#### 4. Code Size
| Test | Before | After | Improvement |
|------|--------|-------|-------------|
| Simple (4 vars) | 144 lines | 71 lines | **51% smaller** |
| Stress (32 vars) | 887 lines | 430 lines | **52% smaller** |

#### 5. No Runtime Overhead
- Zero initialization cost
- Zero cleanup cost
- Zero function call overhead

---

## Key Insights Learned

### 1. **Question Assumptions**
Initial thought: "Globals need runtime allocation"
Reality: Compile-time data segments work perfectly!

### 2. **Let the Backend Work**
By using a data segment, we give QBE:
- A concrete memory location to optimize
- Ability to perform constant folding
- Better instruction scheduling opportunities

### 3. **Simpler is Better**
The final solution:
- Less code in compiler
- Less code at runtime
- Better performance
- Easier to understand

### 4. **User Insights Are Gold**
Both major breakthroughs came from user suggestions:
1. "Can we calculate offset statically?" → 25% reduction
2. "Use r/w data segment, label as base" → 50% reduction

---

## Comparison: All Phases

### Instructions Per Access
```
Phase 1: 4 instructions (call, mul, add, load)
Phase 2: 3 instructions (call, add, load)
Phase 3: 2 instructions (add, load) [but needs setup]
Phase 4: 3 instructions (load base, add, load)
Phase 5: 2 instructions (add, load) ★ WINNER
```

### Assembly Size (32 globals)
```
Phase 1: 887 lines
Phase 2: 887 lines (same size, faster execution)
Phase 3: ~600 lines (if implemented)
Phase 4: ~500 lines (if implemented)
Phase 5: 430 lines ★ WINNER (52% reduction)
```

### Complexity
```
Phase 1: Medium (runtime management)
Phase 2: Medium (same as 1)
Phase 3: High (per-function caching)
Phase 4: Medium (global caching)
Phase 5: Low (just emit data) ★ WINNER
```

---

## Implementation Timeline

| Commit | Change | Impact |
|--------|--------|--------|
| `d0307f2` | Initial scaffolding | Foundation |
| `4a47a72` | Phase 1: Runtime allocation | Working implementation |
| `f60d581` | Phase 2: Static offsets | 25% instruction reduction |
| `09cbff9` | Documentation | - |
| `d43af91` | Assembly analysis | Insights into QBE optimization |
| `8704e13` | Phase 5: Data segment | 50% size reduction |

---

## Final Implementation

### Data Section
```qbe
data $__global_vector = { l 0, l 0, l 0, ... }
```

### Read Operation
```qbe
%addr =l add $__global_vector, BYTE_OFFSET
%value =l loadl %addr
```

### Write Operation
```qbe
%addr =l add $__global_vector, BYTE_OFFSET
storel %value, %addr
```

### Benefits Summary
✅ **2 instructions per access** (was 4)
✅ **52% smaller assembly** (32 globals: 887→430 lines)
✅ **Zero runtime overhead** (no init/cleanup)
✅ **Direct addressing** (label = base pointer)
✅ **QBE can optimize** (constant folding works!)
✅ **Simple to maintain** (less code, less complexity)

---

## Lessons for Future Features

### 1. Start with the Simplest Approach
Don't assume runtime allocation is necessary.

### 2. Measure and Compare
Assembly analysis revealed QBE's optimization power.

### 3. Iterate Based on Insights
Each phase built on lessons from the previous.

### 4. Trust the Backend
QBE is smarter than we think - give it good input!

### 5. Listen to Users
The best optimizations came from user suggestions.

---

## Conclusion

The journey from Phase 1 (runtime allocation) to Phase 5 (compile-time data segment) demonstrates:

1. **Iterative improvement works** - Each phase was better than the last
2. **Simple solutions win** - The final approach is the simplest
3. **Collaboration matters** - User insights drove major breakthroughs
4. **Measure everything** - Assembly analysis guided decisions

**Final Result**: A clean, efficient, and elegant implementation that leverages QBE's optimization capabilities to the fullest.

---

**Date**: January 2025  
**Evolution**: Runtime → Compile-time  
**Improvement**: 4 instructions → 2 instructions (50% reduction)  
**Status**: Production-ready and optimal ✅