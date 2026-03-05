#!/bin/bash
set -e  # Abort the script immediately if a command fails

# -----------------------------------------------------------------------------
# 0. Environment Setup & OS Detection
# -----------------------------------------------------------------------------
GREEN='\033[0;32m'
BLUE='\033[0;34m'
RED='\033[0;31m'
NC='\033[0m' # No Color

OS="$(uname -s)"

# Determine the command to open the HTML report based on the operating system
if [ "$OS" == "Darwin" ]; then
    OPEN_CMD="open"
elif [ "$OS" == "Linux" ]; then
    if command -v xdg-open &> /dev/null; then
        OPEN_CMD="xdg-open"
    else
        OPEN_CMD="echo Report available at:"
    fi
else
    # Fallback (Git Bash on Windows)
    OPEN_CMD="echo Report available at:"
fi

echo -e "${BLUE}=========================================${NC}"
echo -e "${BLUE}   Starting Coverage Workflow ($OS)      ${NC}"
echo -e "${BLUE}=========================================${NC}"

# -----------------------------------------------------------------------------
# 1. Clean Build Directory
# -----------------------------------------------------------------------------
echo -e "${BLUE}[1/8] Cleaning build directory...${NC}"
rm -rf build
mkdir build
cd build

# -----------------------------------------------------------------------------
# 2. CMake Configuration
# -----------------------------------------------------------------------------
echo -e "${BLUE}[2/8] Configuring CMake...${NC}"
cmake -DENABLE_COVERAGE=ON -DBUILD_TESTING=ON -DCMAKE_BUILD_TYPE=Debug ..

# -----------------------------------------------------------------------------
# 3. Compile
# -----------------------------------------------------------------------------
echo -e "${BLUE}[3/8] Compiling (using 4 cores)...${NC}"
make -j4

# -----------------------------------------------------------------------------
# 4. Run Tests
# -----------------------------------------------------------------------------
echo -e "${BLUE}[4/8] Running Tests...${NC}"
ctest --output-on-failure

# -----------------------------------------------------------------------------
# 5. Capture Coverage (Raw Data)
# -----------------------------------------------------------------------------
echo -e "${BLUE}[5/8] Capturing coverage data...${NC}"
lcov --capture --directory . --output-file coverage.info \
    --ignore-errors mismatch,inconsistent,format

# -----------------------------------------------------------------------------
# 6. Filter Data (Remove External Libs, Tests & Excluded Platforms)
# -----------------------------------------------------------------------------
echo -e "${BLUE}[6/8] Filtering unwanted files...${NC}"
# Exclude paths for both macOS (/opt/homebrew) and generic Linux (/usr/*).
# Platform exclusions:
#   - platform/arduino/: not compilable on PC
#   - platform/common/tls_certificates*: data-only (embedded root certs)
#   - platform/pc/http_client*: thin CPR/libcurl wrapper, requires real network
#   - platform/pc/console*: blocking stdin I/O (Read, ReadLine)
# The 'unused' ignore flag ensures this doesn't fail if a path doesn't exist.
lcov --remove coverage.info \
    '/usr/*' \
    '*/tests/*' \
    '*/third_party/*' \
    '*/nlohmann/*' \
    '*/gtest/*' \
    '*/_deps/*' \
    '/opt/homebrew/*' \
    '*/platform/arduino/*' \
    '*/platform/common/tls_certificates*' \
    '*/platform/pc/http_client*' \
    '*/platform/pc/console*' \
    --output-file coverage_clean.info \
    --ignore-errors mismatch,inconsistent,format,unused

# -----------------------------------------------------------------------------
# 7. Generate HTML Report
# -----------------------------------------------------------------------------
echo -e "${BLUE}[7/8] Generating HTML Report...${NC}"
genhtml coverage_clean.info \
    --output-directory coverage_report \
    --ignore-errors mismatch,inconsistent,format,category

# -----------------------------------------------------------------------------
# 8. Print Summary & Finish
# -----------------------------------------------------------------------------
echo ""
echo -e "${BLUE}[8/8] Coverage Summary:${NC}"
lcov --summary coverage_clean.info \
    --ignore-errors mismatch,inconsistent,format 2>&1 | tail -4

echo ""
echo -e "${GREEN}=========================================${NC}"
echo -e "${GREEN}   Success!                              ${NC}"
echo -e "${GREEN}=========================================${NC}"

# Open the report
$OPEN_CMD coverage_report/index.html
