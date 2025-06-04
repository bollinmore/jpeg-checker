#!/bin/bash
set -e

# Navigate to repository root
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR/.."

# Build the jpeg_parser
echo "Compiling jpeg_parser..."
gcc jpeg_parser.c -o jpeg_parser

check_color_space() {
    local img="$1"
    local expect="$2"
    echo "Running on $img"
    ./jpeg_parser "$img" > /tmp/jpeg_out.txt
    local rc=$?
    echo "Return code: $rc"
    if [ $rc -eq 0 ] && grep -q "$expect" /tmp/jpeg_out.txt; then
        echo "PASS: '$expect' found in output"
    else
        echo "FAIL: '$expect' not found in output"
    fi
    rm -f /tmp/jpeg_out.txt
}

check_color_space images/rgb.jpeg "Possibly RGB"
check_color_space images/cmyk.jpeg "Unknown 4-component"
