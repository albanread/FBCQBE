# FBCQBE Documentation Index

**Last Updated:** February 1, 2025

---

## 🚀 START HERE

**New to the project?** Start with:
- **[START_HERE.md](START_HERE.md)** - Main developer guide (root)
- **[docs/CODEGEN_V2_START_HERE.md](docs/CODEGEN_V2_START_HERE.md)** - Code generator quick start
- **[docs/STATUS_AND_NEXT_STEPS.md](docs/STATUS_AND_NEXT_STEPS.md)** - Current status overview

---

## 📋 Code Generator V2 (Current Work)

### Getting Started
- **[docs/CODEGEN_V2_START_HERE.md](docs/CODEGEN_V2_START_HERE.md)** ⭐ READ FIRST
- **[docs/STATUS_AND_NEXT_STEPS.md](docs/STATUS_AND_NEXT_STEPS.md)** - Status & roadmap
- **[docs/CODEGEN_V2_KICKOFF.md](docs/CODEGEN_V2_KICKOFF.md)** - Quick kickoff

### Implementation Guides
- **[docs/CODEGEN_V2_ACTION_PLAN.md](docs/CODEGEN_V2_ACTION_PLAN.md)** - Step-by-step (720 lines)
- **[docs/CODEGEN_V2_DESIGN_PLAN.md](docs/CODEGEN_V2_DESIGN_PLAN.md)** - Complete design (942 lines)
- **[docs/CODEGEN_REFACTOR_SUMMARY.md](docs/CODEGEN_REFACTOR_SUMMARY.md)** - Executive summary

### Reference
- **[fsh/FasterBASICT/src/oldcodegen/README_ARCHIVE.md](fsh/FasterBASICT/src/oldcodegen/README_ARCHIVE.md)** - Old generator (reference only)

---

## ✅ CFG v2 Documentation (Complete)

### Status Reports
- **[docs/CFG_V2_COMPLETION_STATUS.md](docs/CFG_V2_COMPLETION_STATUS.md)** - Final status
- **[docs/CFG_V2_STATUS.md](docs/CFG_V2_STATUS.md)** - Detailed report
- **[docs/CFG_STATUS_AFTER_RECOVERY.md](docs/CFG_STATUS_AFTER_RECOVERY.md)** - After fixes
- **[docs/CFG_FIXES_SUMMARY.md](docs/CFG_FIXES_SUMMARY.md)** - Fix summary

### Test Results
- **[docs/CFG_TEST_RESULTS_2026_02_01.md](docs/CFG_TEST_RESULTS_2026_02_01.md)** - Latest results
- **[CFG_TEST_RESULTS.md](CFG_TEST_RESULTS.md)** - Root copy

### Implementation Details
- **[docs/CFG_V2_MODULAR_IMPLEMENTATION.md](docs/CFG_V2_MODULAR_IMPLEMENTATION.md)** - Modular structure
- **[docs/CFG_V2_PROGRESS.md](docs/CFG_V2_PROGRESS.md)** - Progress tracking
- **[docs/CFG_MODULAR_SPLIT_PLAN.md](docs/CFG_MODULAR_SPLIT_PLAN.md)** - Split plan
- **[docs/CFG_REFACTORING_PLAN.md](docs/CFG_REFACTORING_PLAN.md)** - Refactoring plan
- **[docs/CFG_RECOVERY_PLAN.md](docs/CFG_RECOVERY_PLAN.md)** - Recovery plan

---

## 📊 Unreachable Code Analysis

### Main Documents
- **[docs/unreachable_code_analysis.md](docs/unreachable_code_analysis.md)** - Complete (588 lines, 18KB)
- **[docs/unreachable_warnings_summary.md](docs/unreachable_warnings_summary.md)** - Quick reference
- **[docs/unreachable_patterns_diagram.md](docs/unreachable_patterns_diagram.md)** - Visual diagrams
- **[docs/unreachable_trace_examples.md](docs/unreachable_trace_examples.md)** - Execution traces
- **[docs/README_UNREACHABLE_ANALYSIS.md](docs/README_UNREACHABLE_ANALYSIS.md)** - Index

### Key Findings
- 12/125 tests have unreachable warnings (9.6%)
- All warnings are legitimate (3 patterns identified)
- Unreachable blocks ARE compiled (safety verified)

---

## 📚 General Documentation

### Build & Development
- **[START_HERE.md](START_HERE.md)** - Main developer guide
- **[BUILD.md](BUILD.md)** - Build instructions
- **[README.md](README.md)** - Project overview

### Session Notes
- **[docs/session_notes/](docs/session_notes/)** - Development session logs

---

## 📂 Directory Structure

```
FBCQBE/
├── START_HERE.md                        ⭐ Start here
├── DOCUMENTATION_INDEX.md               ⭐ This file
├── docs/
│   ├── CODEGEN_V2_START_HERE.md        ⭐ Code gen quick start
│   ├── STATUS_AND_NEXT_STEPS.md        ⭐ Current status
│   ├── CODEGEN_V2_ACTION_PLAN.md       📋 Implementation guide
│   ├── CODEGEN_V2_DESIGN_PLAN.md       📋 Architecture
│   ├── CFG_V2_COMPLETION_STATUS.md     ✅ CFG status
│   ├── unreachable_code_analysis.md    📊 Analysis
│   └── [other docs...]
├── fsh/FasterBASICT/src/
│   ├── codegen_v2/                     🚧 Create this (new)
│   └── oldcodegen/                     📚 Reference only
└── tests/                              🧪 Test suite (125 tests)
```

---

## 🎯 Quick Navigation

### I want to...

**...understand the current status**
→ Read [docs/STATUS_AND_NEXT_STEPS.md](docs/STATUS_AND_NEXT_STEPS.md)

**...work on code generation**
→ Read [docs/CODEGEN_V2_START_HERE.md](docs/CODEGEN_V2_START_HERE.md)

**...understand CFG v2**
→ Read [docs/CFG_V2_COMPLETION_STATUS.md](docs/CFG_V2_COMPLETION_STATUS.md)

**...understand unreachable warnings**
→ Read [docs/unreachable_warnings_summary.md](docs/unreachable_warnings_summary.md)

**...build and test**
→ Read [START_HERE.md](START_HERE.md)

**...see implementation details**
→ Read [docs/CODEGEN_V2_ACTION_PLAN.md](docs/CODEGEN_V2_ACTION_PLAN.md)

---

## 📊 Documentation Statistics

- **Total docs:** ~30 markdown files
- **Code generator docs:** 6 files (~3,000 lines)
- **CFG v2 docs:** 10 files
- **Unreachable analysis:** 5 files (67KB)
- **Test suite:** 125 tests
- **CFG v2 status:** 123/125 valid (98.4%)

---

**Status:** ✅ Documentation Complete - Ready for Code Generator V2 Implementation
