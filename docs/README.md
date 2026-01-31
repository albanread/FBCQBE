# Documentation

This directory contains all project documentation organized by category.

## Directory Structure

- **design/** - Design documents, implementation plans, and architecture notes
  - Type system design
  - QBE integration strategy
  - Feature implementations
  - Language semantics
  
- **session_notes/** - Development session summaries and bug fix logs
  - Session summaries
  - Bug investigation notes
  - Fix verification reports
  - Commit messages

- **testing/** - Test results, verification reports, and test coverage
  - Test results
  - Verification reports
  - Test coverage analysis
  - Status summaries

## Key Documents

### Getting Started
- [../START_HERE.md](../START_HERE.md) - Developer guide and quick start
- [../BUILD.md](../BUILD.md) - Build instructions
- [../README.md](../README.md) - Project overview

### Design Reference
- [design/TYPE_SYSTEM_*.md](design/) - Type system implementation
- [design/QBE_*.md](design/) - QBE backend integration
- [design/GLOBALS_*.md](design/) - Global variable implementation
- [TYPE_SUFFIX_REFERENCE.md](TYPE_SUFFIX_REFERENCE.md) - Variable type suffixes

### Troubleshooting
- [DEBUGGING_GUIDE.md](DEBUGGING_GUIDE.md) - Debugging tips and techniques
- [COMPILER_VERSIONS.md](COMPILER_VERSIONS.md) - Compiler versions and known issues

## Document Naming Conventions

- `*_DESIGN.md` - Design documents and architecture
- `*_IMPLEMENTATION.md` - Implementation details and progress
- `SESSION_*.md` - Development session notes
- `*_FIX.md` - Bug fix reports
- `*_TEST*.md` - Test results and verification
- `*_STATUS.md` - Feature status reports

## Contributing Documentation

When adding new documentation:

1. Choose the appropriate subdirectory (design/session_notes/testing)
2. Use descriptive filenames with appropriate suffixes
3. Include date in session notes
4. Update this README if adding a new category