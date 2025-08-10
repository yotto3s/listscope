#!/bin/bash

# Usage: ./format-code.sh /path/to/source
set -euo pipefail

# Set the directory to format
TARGET_DIR="${1:-}"

# Check if directory is passed
if [ -z "${TARGET_DIR}" ]; then
    echo "Usage: $0 /path/to/source"
    exit 1
fi

# Check if clang-format is installed
if ! command -v clang-format &> /dev/null; then
    echo "Error: clang-format is not installed."
    exit 1
fi

# Find and format all .cpp and .hpp files
find "$TARGET_DIR" -type f \( -name "*.cpp" -o -name "*.hpp" \) -print0 | while IFS= read -r -d '' file; do
    echo "Formatting $file"
    clang-format -i "$file"
done

echo "Formatting complete."
