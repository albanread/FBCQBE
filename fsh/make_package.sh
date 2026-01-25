#!/bin/bash

# Script to create distributable package for BASIC compiler
# Creates package/ directory with compiler, QBE, and runtime files

set -e  # Exit on error

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "=== Creating BASIC Compiler Package ==="
echo

# Check if basic executable exists
if [ ! -f "basic" ]; then
    echo "Error: basic compiler not found. Run ./build_fbc_qbe.sh first."
    exit 1
fi

# Check if qbe exists
QBE_PATH=""
if [ -f "qbe/qbe" ]; then
    QBE_PATH="qbe/qbe"
elif command -v qbe &> /dev/null; then
    QBE_PATH="$(command -v qbe)"
else
    echo "Warning: qbe not found. Package will not include QBE backend."
fi

# Create package structure
echo "Creating package directory structure..."
rm -rf package
mkdir -p package/qbe
mkdir -p package/runtime

# Copy compiler executable
echo "Copying basic compiler..."
cp basic package/

# Copy QBE if available
if [ -n "$QBE_PATH" ]; then
    echo "Copying QBE backend..."
    cp "$QBE_PATH" package/qbe/
fi

# Copy runtime files
echo "Copying runtime files..."
if [ -d "FasterBASICT/runtime_c" ]; then
    cp FasterBASICT/runtime_c/*.c package/runtime/ 2>/dev/null || true
    cp FasterBASICT/runtime_c/*.h package/runtime/ 2>/dev/null || true
fi

# Copy README if exists
if [ -f "package/README.md" ]; then
    echo "README.md already in package/"
elif [ -f "../README.md" ]; then
    cp ../README.md package/
fi

# Create version file
VERSION="1.0.0"
if [ -f "package/basic" ]; then
    VERSION=$(./package/basic --version 2>/dev/null | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' || echo "1.0.0")
fi
echo "$VERSION" > package/VERSION

echo
echo "Package created in: $SCRIPT_DIR/package"
echo

# Create archives
ARCHIVE_NAME="basic-compiler-$(uname -s)-$(uname -m)-${VERSION}"

echo "Creating distribution archives..."
echo

# Create tar.gz
if command -v tar &> /dev/null; then
    echo "Creating ${ARCHIVE_NAME}.tar.gz..."
    cd "$SCRIPT_DIR"
    tar czf "${ARCHIVE_NAME}.tar.gz" package/
    echo "  ✓ ${ARCHIVE_NAME}.tar.gz ($(du -h "${ARCHIVE_NAME}.tar.gz" | cut -f1))"
fi

# Create zip
if command -v zip &> /dev/null; then
    echo "Creating ${ARCHIVE_NAME}.zip..."
    cd "$SCRIPT_DIR"
    zip -qr "${ARCHIVE_NAME}.zip" package/
    echo "  ✓ ${ARCHIVE_NAME}.zip ($(du -h "${ARCHIVE_NAME}.zip" | cut -f1))"
fi

echo
echo "=== Package Complete ==="
echo
echo "Distribution files:"
ls -lh "$SCRIPT_DIR"/*.tar.gz "$SCRIPT_DIR"/*.zip 2>/dev/null || true
echo
echo "To use:"
echo "  tar xzf ${ARCHIVE_NAME}.tar.gz"
echo "  cd package"
echo "  ./basic program.bas -o program"
