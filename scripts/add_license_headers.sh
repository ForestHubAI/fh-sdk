#!/usr/bin/env bash
# Inserts SPDX license headers from LICENSE_HEADER into all source files.
# Usage: ./scripts/add_license_headers.sh [--force]
#   --force: Replace existing SPDX header blocks with the correct header.
#   Default: Skip files that already contain SPDX-License-Identifier.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
HEADER_FILE="$REPO_ROOT/LICENSE_HEADER"

if [[ ! -f "$HEADER_FILE" ]]; then
    echo "ERROR: $HEADER_FILE not found"
    exit 1
fi

HEADER_CONTENT="$(cat "$HEADER_FILE")"

FORCE=false
if [[ "${1:-}" == "--force" ]]; then
    FORCE=true
fi

INSERTED=0
SKIPPED=0
REPLACED=0
TOTAL=0

while IFS= read -r file; do
    TOTAL=$((TOTAL + 1))

    if grep -q "SPDX-License-Identifier" "$file"; then
        if [[ "$FORCE" == true ]]; then
            # Remove existing SPDX block (consecutive // comment lines at file start
            # that contain SPDX, Copyright, or commercial licensing)
            tmpfile="$(mktemp)"
            in_header=true
            while IFS= read -r line; do
                if $in_header; then
                    if [[ "$line" =~ ^// ]] && [[ "$line" =~ (SPDX|Copyright|commercial\ licensing) ]]; then
                        continue
                    fi
                    # Skip one blank line after SPDX block
                    if [[ -z "$line" ]]; then
                        in_header=false
                        continue
                    fi
                    in_header=false
                fi
                printf '%s\n' "$line"
            done < "$file" > "$tmpfile"

            # Prepend correct header
            {
                printf '%s\n\n' "$HEADER_CONTENT"
                cat "$tmpfile"
            } > "$file"
            rm -f "$tmpfile"

            REPLACED=$((REPLACED + 1))
            echo "REPLACE: $file"
        else
            SKIPPED=$((SKIPPED + 1))
            echo "SKIP: $file"
        fi
    else
        # Prepend header + blank line
        tmpfile="$(mktemp)"
        {
            printf '%s\n\n' "$HEADER_CONTENT"
            cat "$file"
        } > "$tmpfile"
        mv "$tmpfile" "$file"

        INSERTED=$((INSERTED + 1))
        echo "INSERT: $file"
    fi
done < <(find "$REPO_ROOT/include" "$REPO_ROOT/src" "$REPO_ROOT/tests" "$REPO_ROOT/examples" \
    -type f \( -name "*.hpp" -o -name "*.cpp" \) \
    ! -path "*/third_party/*" \
    ! -path "*/.pio/*" \
    ! -path "*/build/*" \
    | sort)

echo ""
echo "=== Summary ==="
echo "Total:    $TOTAL"
echo "Inserted: $INSERTED"
echo "Skipped:  $SKIPPED"
echo "Replaced: $REPLACED"
