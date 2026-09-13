#!/usr/bin/env bash

set -euo pipefail

PROJECT_ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
PICO_HOME="${PICO_HOME:-${HOME}/.pico-sdk}"

die() {
    printf 'ERROR: %s\n' "$*" >&2
    exit 1
}

newest_match() {
    local pattern="$1"
    local result
    result="$(find ${pattern} -type f -print 2>/dev/null | sort -V | tail -n 1 || true)"
    [[ -n "${result}" ]] && printf '%s\n' "${result}"
}

find_sdk() {
    if [[ -n "${PICO_SDK_PATH:-}" && -f "${PICO_SDK_PATH}/pico_sdk_init.cmake" ]]; then
        printf '%s\n' "${PICO_SDK_PATH}"
        return
    fi
    local init
    init="$(newest_match "${PICO_HOME}/sdk/*/pico_sdk_init.cmake")"
    [[ -n "${init}" ]] || return 1
    dirname "${init}"
}

find_program() {
    local name="$1"
    local managed_pattern="$2"
    if command -v "${name}" >/dev/null 2>&1; then
        command -v "${name}"
        return
    fi
    newest_match "${managed_pattern}"
}

find_toolchain() {
    local triple="$1"
    local explicit="$2"
    if [[ -n "${explicit}" && -x "${explicit}/bin/${triple}-gcc" ]]; then
        printf '%s\n' "${explicit}"
        return
    fi
    local compiler
    compiler="$(newest_match "${PICO_HOME}/toolchain/*/bin/${triple}-gcc")"
    [[ -n "${compiler}" ]] || return 1
    dirname "$(dirname "${compiler}")"
}

find_cmake() {
    find_program cmake "${PICO_HOME}/cmake/*/bin/cmake"
}

find_ninja() {
    find_program ninja "${PICO_HOME}/ninja/*/ninja"
}

find_picotool() {
    if [[ -n "${PICOTOOL:-}" && -x "${PICOTOOL}" ]]; then
        printf '%s\n' "${PICOTOOL}"
        return
    fi
    if command -v picotool >/dev/null 2>&1; then
        command -v picotool
        return
    fi
    local managed
    managed="$(newest_match "${PICO_HOME}/picotool/*/picotool/picotool")"
    if [[ -z "${managed}" ]]; then
        managed="$(newest_match "${PICO_HOME}/picotool/*/picotool")"
    fi
    [[ -n "${managed}" ]] && printf '%s\n' "${managed}"
}
