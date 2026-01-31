# Project Structure

This document describes the organization of the FasterBASIC QBE compiler project.

## Root Directory Layout

```
FBCQBE/
├── BUILD.md                    # Build instructions and troubleshooting
├── BUILD_CONSOLIDATION.md      # Build consolidation analysis
├── LICENSE                     # Project license
├── README.md                   # Project overview and status
├── START_HERE.md               # Developer guide and quick start
│
├── qbe_basic                   # Symlink to qbe_basic_integrated/qbe_basic
│
├── build_artifacts/            # Build outputs (gitignored)
│   ├── *.o, *.s, *.qbe        # Compiled objects, assembly, QBE IL
│   └── test executables        # Test program binaries
│
├── docs/                       # All documentation
│   ├── README.md              # Documentation index
│   ├── design/                # Design documents and architecture
│   ├── session_notes/         # Development session summaries
│   └── testing/               # Test results and verification
│
├── fsh/                        # FasterBASIC source code
│   ├── FasterBASICT/          # Main compiler source
│   │   ├── src/               # C++ source files
│   │   │   ├── codegen/       # QBE code generator
│   │   │   ├── *.cpp          # Lexer, parser, semantic, CFG
│   │   │   └── *.h            # Header files
│   │   ├── runtime/           # C++ runtime components
│   │   └── runtime_c/         # C runtime library
│   └── qbe/                   # QBE compiler integration
│
├── qbe_basic_integrated/       # Main build location ⭐
│   ├── build_qbe_basic.sh     # Primary build script
│   ├── qbe_basic              # Compiled executable
│   ├── qbe_source/            # QBE backend source
│   └── runtime/               # Runtime library (copied during build)
│
├── scripts/                    # Utility scripts
│   ├── README.md              # Script documentation
│   ├── run_tests_simple.sh    # Quick test runner
│   ├── verify_implementation.sh # Full verification
│   └── generate_tests.sh      # Test generation
│
├── test_programs/              # Test programs
│   ├── README.md              # Test program guide
│   ├── examples/              # Example BASIC programs
│   │   ├── test_if_minimal.bas
│   │   └── test_nested_if_simple.bas
│   └── scratch/               # Temporary test files (gitignored)
│
└── tests/                      # Official test suite
    ├── arithmetic/            # Arithmetic operator tests
    ├── arrays/                # Array handling tests
    ├── conditionals/          # IF/CASE statement tests
    ├── loops/                 # FOR/WHILE/DO loop tests
    ├── rosetta/               # Rosetta Code implementations
    └── ...                    # Other test categories
```

## Key Directories

### Build Location

**⚠️ Important:** Always build from `qbe_basic_integrated/`:

```bash
cd qbe_basic_integrated
./build_qbe_basic.sh
```

The root-level `qbe_basic` is a **symlink** to `qbe_basic_integrated/qbe_basic`.

### Source Code

All compiler source code is in `fsh/FasterBASICT/src/`:

- **Lexer**: `fasterbasic_lexer.cpp` - Tokenization
- **Parser**: `fasterbasic_parser.cpp` - AST construction
- **Semantic**: `fasterbasic_semantic.cpp` - Type checking and validation
- **CFG**: `fasterbasic_cfg.cpp` - Control flow graph construction
- **Codegen**: `codegen/*.cpp` - QBE IL code generation

### Documentation

Documentation is organized in `docs/`:

- **design/** - Architecture and implementation plans
- **session_notes/** - Development logs and bug fixes
- **testing/** - Test results and verification reports

See [docs/README.md](docs/README.md) for details.

### Test Programs

Two types of test programs:

1. **test_programs/examples/** - Committed example programs
   - Well-documented
   - Demonstrate language features
   - Good for learning

2. **test_programs/scratch/** - Temporary test files (gitignored)
   - Development experiments
   - Debugging aids
   - Not committed

### Official Tests

Comprehensive test suite in `tests/`:

- Organized by feature category
- Used by CI/CD pipeline
- Run with `./scripts/verify_implementation.sh`

## File Types

### Source Files
- `*.cpp`, `*.h` - C++ compiler source
- `*.c` - C runtime library
- `*.bas` - BASIC test programs

### Build Artifacts (gitignored)
- `*.o` - Object files
- `*.s` - Assembly output
- `*.qbe` - QBE intermediate language
- `test_*` - Test executables (in build_artifacts/)

### Documentation
- `*.md` - Markdown documentation
- `*_DESIGN.md` - Design documents
- `*_IMPLEMENTATION.md` - Implementation details
- `SESSION_*.md` - Development session notes
- `*_TEST*.md` - Test results

## Gitignore Policy

The following are **not committed**:

- `build_artifacts/` - All build outputs
- `test_programs/scratch/` - Temporary test files
- `*.o`, `*.s`, `*.qbe` - Build artifacts
- `test_*` executables - Test binaries

Exceptions:
- `test_*.bas` files ARE committed (source code)
- `tests/` directory IS committed (official test suite)

## Navigation

### Quick Access

```bash
# From project root
cd qbe_basic_integrated    # Build location
cd fsh/FasterBASICT/src   # Compiler source
cd docs                    # Documentation
cd tests                   # Test suite
cd test_programs/examples  # Example programs
```

### Building

```bash
# Always build from qbe_basic_integrated/
cd qbe_basic_integrated
./build_qbe_basic.sh
cd ..
./qbe_basic program.bas
```

### Testing

```bash
# From project root
./scripts/run_tests_simple.sh           # Quick tests
./scripts/verify_implementation.sh      # Full verification
./qbe_basic test_programs/examples/*.bas # Run examples
```

### Documentation

```bash
# Start here
cat START_HERE.md
cat BUILD.md

# Browse docs
ls docs/design/
ls docs/session_notes/
ls docs/testing/
```

## Adding New Files

### New Test Program

```bash
# For experiments
vim test_programs/scratch/mytest.bas

# For examples (commit these)
vim test_programs/examples/feature_demo.bas
git add test_programs/examples/feature_demo.bas
```

### New Documentation

```bash
# Design doc
vim docs/design/NEW_FEATURE_DESIGN.md

# Session notes
vim docs/session_notes/SESSION_2026_01_31.md

# Test results
vim docs/testing/FEATURE_TEST_RESULTS.md
```

### New Source File

```bash
# In appropriate source directory
vim fsh/FasterBASICT/src/new_feature.cpp
vim fsh/FasterBASICT/src/new_feature.h

# Update build script if needed
vim qbe_basic_integrated/build_qbe_basic.sh
```

## Maintenance

### Cleaning Build Artifacts

```bash
# Remove all build artifacts
rm -rf build_artifacts/*

# Clean integrated build
cd qbe_basic_integrated
rm -rf obj/*.o qbe_source/*.o qbe_basic
```

### Checking Project Status

```bash
# Show untracked files
git status

# Show directory sizes
du -sh */ | sort -h

# Count files by type
find . -type f -name "*.cpp" | wc -l
find . -type f -name "*.bas" | wc -l
```

## See Also

- [START_HERE.md](START_HERE.md) - Getting started guide
- [BUILD.md](BUILD.md) - Build instructions
- [docs/README.md](docs/README.md) - Documentation index
- [test_programs/README.md](test_programs/README.md) - Test program guide
- [scripts/README.md](scripts/README.md) - Script documentation