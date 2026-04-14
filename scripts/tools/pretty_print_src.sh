#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
TARGET_DIR="${1:-src}"
MODE="${MODE:-fix}"
CLANG_FORMAT_CONFIG="${CLANG_FORMAT_CONFIG:-${SCRIPT_DIR}/.clang-format}"

find_clang_format() {
    if [[ -n "${CLANG_FORMAT_BIN:-}" ]]; then
        printf '%s\n' "${CLANG_FORMAT_BIN}"
        return 0
    fi

    if command -v clang-format >/dev/null 2>&1; then
        command -v clang-format
        return 0
    fi

    if [[ -x "/opt/homebrew/opt/llvm/bin/clang-format" ]]; then
        printf '%s\n' "/opt/homebrew/opt/llvm/bin/clang-format"
        return 0
    fi

    echo "Error: clang-format not found. Install llvm or set CLANG_FORMAT_BIN." >&2
    exit 1
}

collect_files() {
    (
        cd "${PROJECT_ROOT}"
        rg --files "${TARGET_DIR}" \
            -g '*.c' \
            -g '*.cc' \
            -g '*.cpp' \
            -g '*.cxx' \
            -g '*.h' \
            -g '*.hh' \
            -g '*.hpp' \
            -g '*.hxx' | sort
    )
}

main() {
    local clang_format_bin
    clang_format_bin="$(find_clang_format)"

    if [[ ! -d "${PROJECT_ROOT}/${TARGET_DIR}" ]]; then
        echo "Error: directory not found: ${TARGET_DIR}" >&2
        exit 1
    fi

    if [[ ! -f "${CLANG_FORMAT_CONFIG}" ]]; then
        echo "Error: clang-format config not found: ${CLANG_FORMAT_CONFIG}" >&2
        exit 1
    fi

    files=()
    while IFS= read -r file; do
        files+=("${file}")
    done < <(collect_files)

    if [[ "${#files[@]}" -eq 0 ]]; then
        echo "No C/C++ source files found under ${TARGET_DIR}."
        exit 0
    fi

    echo "Formatting ${#files[@]} files under ${TARGET_DIR}"

    case "${MODE}" in
        fix)
            (
                cd "${PROJECT_ROOT}"
                "${clang_format_bin}" -i --style="file:${CLANG_FORMAT_CONFIG}" "${files[@]}"
            )
            ;;
        check)
            (
                cd "${PROJECT_ROOT}"
                "${clang_format_bin}" --dry-run --Werror --style="file:${CLANG_FORMAT_CONFIG}" "${files[@]}"
            )
            ;;
        *)
            echo "Error: unsupported MODE='${MODE}'. Use 'fix' or 'check'." >&2
            exit 1
            ;;
    esac
}

main "$@"
