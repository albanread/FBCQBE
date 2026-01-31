# FasterBASIC Documentation Index

Complete guide to all documentation in the project. Start here to find what you need!

---

## 🚀 Quick Start

**New to the project?** Read these in order:
1. `README.md` - Project overview and features
2. `START_HERE.md` - Build, compile, and run your first program
3. `BUILD.md` - Detailed build instructions

**Just want to compile something?**
```bash
cd qbe_basic_integrated && ./build_qbe_basic.sh
cd ..
./qbe_basic -o myprogram myprogram.bas
./myprogram
```

---

## 📚 Core Documentation

### Essential Reading

| Document | Purpose | Audience |
|----------|---------|----------|
| `README.md` | Project overview, features, status | Everyone |
| `START_HERE.md` | Developer guide, type system, workflow | Developers |
| `BUILD.md` | Build system, dependencies, troubleshooting | Contributors |
| `LICENSE` | MIT License terms | Everyone |

---

## 🎯 Feature Documentation

### SELECT CASE (Recent - 2024)

**Complete implementation with type handling and superiority analysis**

| Document | Lines | Description |
|----------|-------|-------------|
| `docs/SELECT_CASE_INDEX.md` | 180 | **START HERE** - Complete guide to all SELECT CASE docs |
| `docs/SELECT_CASE_COMPLETE.md` | 150 | Overview of implementation, tests, and docs |
| `docs/SELECT_CASE_FIX_SUMMARY.md` | 80 | Quick technical summary of the bug fix |
| `docs/SELECT_CASE_VS_SWITCH.md` | 660 | ⭐ Why SELECT CASE is superior to switch |
| `docs/SELECT_CASE_TYPE_GLYPH_ANALYSIS.md` | 450 | Deep dive into type handling |
| `docs/TEST_SUITE_UPDATE.md` | 315 | Test suite addition details |
| `docs/VERIFICATION_COMPLETE.md` | 120 | Final verification report |
| `tests/conditionals/README_SELECT_CASE.md` | 220 | Test documentation |

**Quick access:** `docs/SELECT_CASE_INDEX.md` for complete navigation

**Test runner:** `scripts/test_select_cases.sh`

---

## 🔧 Implementation Documentation

### In `docs/` Directory

#### Design Documents
- `docs/design/ControlFlowGraph.md` - CFG implementation and design
- `docs/PROJECT_STRUCTURE.md` - Directory layout and file organization
- `docs/design/` - Various design decisions

#### Optimization
- `docs/OPTIMIZATION_SUMMARY.md` - Compiler optimizations overview
- `docs/SPARSE_RETURN_OPTIMIZATION.md` - GOSUB return stack optimization
- `docs/VARIABLE_ACCESS_PERFORMANCE.md` - Variable access analysis

#### SELECT CASE Implementation
- `docs/SELECT_CASE_INDEX.md` - Navigation guide for all SELECT CASE docs
- `docs/SELECT_CASE_COMPLETE.md` - Implementation summary
- `docs/SELECT_CASE_FIX_SUMMARY.md` - Bug fix details
- `docs/SELECT_CASE_VS_SWITCH.md` - Superiority analysis
- `docs/SELECT_CASE_TYPE_GLYPH_ANALYSIS.md` - Type handling deep dive
- `docs/TEST_SUITE_UPDATE.md` - Test suite additions
- `docs/VERIFICATION_COMPLETE.md` - Verification report

#### Session Notes
- `docs/session_notes/` - Development session summaries
- `docs/session_notes/GOSUB_FIX_VERIFICATION.md` - GOSUB implementation
- `docs/session_notes/BUG_FIX_SUMMARY.md` - Bug fix history
- `docs/session_notes/CFG_FIX_SESSION_COMPLETE.md` - Control flow fixes
- `docs/session_notes/NESTED_WHILE_IF_FIX_SUMMARY.md` - Nested control flow
- And many more...

#### Rosetta Code
- `docs/ROSETTA_MERSENNE_SOLUTION.md` - Mersenne prime factorization

#### Build System
- `docs/BUILD_CONSOLIDATION.md` - Build system consolidation notes

---

## 🧪 Testing Documentation

### Test Organization

```
tests/
├── arithmetic/         # Math operators, bitwise, MOD
├── arrays/            # Array operations, 1D, 2D
├── comparisons/       # Numeric comparisons
├── conditionals/      # IF/THEN/ELSE, SELECT CASE
│   ├── README_SELECT_CASE.md  # ⭐ SELECT CASE test docs
│   ├── test_select_case.bas
│   ├── test_select_types.bas
│   ├── test_select_demo.bas
│   └── test_select_advanced.bas
├── data/              # DATA/READ/RESTORE
├── exceptions/        # TRY/CATCH/FINALLY
├── functions/         # GOSUB, math intrinsics
├── io/                # PRINT formatting, file I/O
├── loops/             # FOR, WHILE, DO, REPEAT
├── rosetta/           # Rosetta Code implementations
│   ├── ADDITION_CHAIN_ARM_ANALYSIS.md
│   ├── EULER_METHOD_SUMMARY.md
│   └── EULER_V1_VS_V2_COMPARISON.md
├── strings/           # String operations
└── types/             # Type conversions, UDTs
```

### Test Runners

- `scripts/run_tests_simple.sh` - Main test runner
- `scripts/test_select_cases.sh` - SELECT CASE regression tests
- `qbe_basic_integrated/run_tests.sh` - QBE-specific tests

### Test Documentation

- `TEST_SUITE_UPDATE.md` - Recent test suite additions
- `tests/conditionals/README_SELECT_CASE.md` - SELECT CASE tests
- `tests/rosetta/*.md` - Rosetta Code analysis docs

---

## 📦 Archived Materials

### `archived_tests/`

Old test files from development sessions, kept for historical reference:

- `archived_tests/gosub_debugging/` - GOSUB implementation tests
- `archived_tests/rosetta_mersenne/` - Mersenne prime iterations
- `archived_tests/division_tests/` - Division operator tests
- `archived_tests/README.md` - What's archived and why

**Note:** Use `tests/` for current tests, not `archived_tests/`

---

## 📖 Documentation by Topic

### Type System
- `START_HERE.md` - Type System section
- `docs/SELECT_CASE_TYPE_GLYPH_ANALYSIS.md` - Type handling in SELECT CASE
- Tests: `tests/types/`

### Control Flow
- `docs/design/ControlFlowGraph.md` - CFG design
- `docs/SELECT_CASE_VS_SWITCH.md` - SELECT CASE features
- `docs/session_notes/CFG_FIX_SESSION_COMPLETE.md` - Bug fixes
- Tests: `tests/conditionals/`, `tests/loops/`

### GOSUB/RETURN
- `docs/session_notes/GOSUB_*.md` - Multiple implementation docs
- `docs/session_notes/SIMPLE_GOSUB_DESIGN.md` - Design
- `archived_tests/gosub_debugging/` - Development tests

### Optimization
- `docs/OPTIMIZATION_SUMMARY.md` - Overview
- `docs/SPARSE_RETURN_OPTIMIZATION.md` - GOSUB optimization
- `docs/VARIABLE_ACCESS_PERFORMANCE.md` - Variable access
- `tests/rosetta/EULER_V1_VS_V2_COMPARISON.md` - Real-world optimization

### Arrays
- Tests: `tests/arrays/`
- Various session notes on array implementation

### Exceptions
- `docs/EXCEPTION_*.md` - Multiple phase documents
- Tests: `tests/exceptions/`

### Rosetta Code
- `docs/ROSETTA_MERSENNE_SOLUTION.md` - Mersenne primes
- `tests/rosetta/` - Implementations and analysis
- `archived_tests/rosetta_mersenne/` - Historical iterations

---

## 🎓 Learning Paths

### Path 1: User Learning to Program in FasterBASIC
1. `README.md` - What is FasterBASIC?
2. `START_HERE.md` - Quick Start section
3. `START_HERE.md` - Type System section
4. Write your first program
5. `docs/fasterbasicquickref.md` - Language reference

### Path 2: Developer Contributing to Compiler
1. `START_HERE.md` - Full read
2. `BUILD.md` - Build system
3. `PROJECT_STRUCTURE.md` - Code organization
4. `docs/design/ControlFlowGraph.md` - Core design
5. `docs/session_notes/` - Browse session notes
6. Start with small fixes/features

### Path 3: Understanding SELECT CASE Implementation
1. `docs/SELECT_CASE_INDEX.md` - Navigation guide
2. `docs/SELECT_CASE_FIX_SUMMARY.md` - What was fixed
3. `docs/SELECT_CASE_VS_SWITCH.md` - Why it's powerful
4. `docs/SELECT_CASE_TYPE_GLYPH_ANALYSIS.md` - Deep dive
5. `tests/conditionals/test_select_*.bas` - See it working

### Path 4: Understanding Optimizations
1. `docs/OPTIMIZATION_SUMMARY.md` - Overview
2. `tests/rosetta/ADDITION_CHAIN_ARM_ANALYSIS.md` - Real example
3. `tests/rosetta/EULER_V1_VS_V2_COMPARISON.md` - Before/after
4. `docs/VARIABLE_ACCESS_PERFORMANCE.md` - Specific optimization
5. Assembly output analysis

---

## 📊 Documentation Statistics

**Total Documentation:** ~10,000+ lines across all docs

**Major Topics:**
- SELECT CASE: ~2,300 lines (8 documents)
- GOSUB/RETURN: ~1,500 lines (7+ documents)
- Session Notes: ~5,000+ lines (20+ documents)
- Rosetta Code Analysis: ~800 lines (5 documents)
- Test Documentation: ~600 lines

**Most Comprehensive:**
- `docs/SELECT_CASE_VS_SWITCH.md` - 660 lines
- `docs/SELECT_CASE_TYPE_GLYPH_ANALYSIS.md` - 450 lines
- `START_HERE.md` - 940 lines

---

## 🔍 Finding Specific Information

### "How do I...?"

**Build the compiler?**
- `BUILD.md` or `START_HERE.md` - Building the Compiler section

**Use SELECT CASE?**
- `START_HERE.md` - Type System section (examples)
- `docs/SELECT_CASE_VS_SWITCH.md` - Complete guide

**Understand the type system?**
- `START_HERE.md` - Type System section
- `docs/SELECT_CASE_TYPE_GLYPH_ANALYSIS.md` - Advanced topics

**Run tests?**
- `START_HERE.md` - Running Tests section
- `docs/TEST_SUITE_UPDATE.md` - Recent test additions

**Optimize code?**
- `docs/OPTIMIZATION_SUMMARY.md`
- `tests/rosetta/EULER_V1_VS_V2_COMPARISON.md` - Example

**Report a bug?**
- `README.md` - Getting Help section
- Look at `docs/session_notes/BUG_*.md` for examples

---

## 📝 Contributing Documentation

When adding documentation:

1. **User docs:** Update `START_HERE.md` or `README.md`
2. **Feature docs:** Create in `docs/` with descriptive name
3. **Session notes:** Add to `docs/session_notes/`
4. **Test docs:** Add README to test directory
5. **Update this index:** Add entry to appropriate section

**Documentation Style:**
- Use markdown with clear headers
- Include examples and code snippets
- Add links to related docs
- Keep a "See Also" section
- Date major documents

---

## 🗂️ Document Categories Quick Reference

### Root Level (Essential Only)
- `README.md` - Project overview
- `START_HERE.md` - Getting started guide
- `BUILD.md` - Build instructions
- `LICENSE` - MIT License
- `DOCUMENTATION_INDEX.md` - This file

### `docs/` (All Implementation Details)
- SELECT CASE documentation (SELECT_CASE_*.md)
- Design documents (PROJECT_STRUCTURE.md, design/)
- Design documents
- Optimization analysis
- Feature deep-dives
- Rosetta Code solutions

### `docs/session_notes/` (Development History)
- Bug fixes
- Implementation sessions
- Design iterations

### `tests/` (Test Documentation)
- Test suite organization
- Feature-specific test docs
- Rosetta Code analysis

### `archived_tests/` (Historical)
- Old development tests
- Debugging artifacts
- Experimental code

---

## 📮 Getting Help

Can't find what you need?

1. Check this index first
2. Search docs: `grep -r "your topic" docs/`
3. Check `START_HERE.md` - most common questions answered
4. Browse `docs/session_notes/` - lots of detailed examples
5. See `README.md` - Getting Help section

---

## 🎉 Recent Additions (2024)

**SELECT CASE Documentation Suite**
- Complete implementation, testing, and analysis
- 8 documents totaling 2,300+ lines
- 39+ test cases across 4 test files
- Proof that SELECT CASE > switch statements

**See:** `docs/SELECT_CASE_INDEX.md` for complete details

---

**Last Updated:** 2024  
**Maintained By:** FasterBASIC Project  
**Documentation Version:** 2.0

---

**Quick Links:**
- Start: `README.md` → `START_HERE.md`
- Build: `BUILD.md`
- Tests: `tests/` + `docs/TEST_SUITE_UPDATE.md`
- Features: `docs/SELECT_CASE_INDEX.md`
- History: `docs/session_notes/`
