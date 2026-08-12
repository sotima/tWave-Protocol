#!/usr/bin/env bash
#
# tWave Protocol — Wire-Format-Test ausführen
#
# Übersetzt test_wire_layout.cpp mit dem AVR- und dem ESP32-Compiler.
# Der Test besteht ausschliesslich aus static_asserts: kompiliert es, stimmt
# das Layout. Es wird weder gelinkt noch geflasht, Hardware ist nicht nötig.
#
#   ./test/run_tests.sh
#
# Die Compiler kommen aus den PlatformIO-Paketen. Ist eine Toolchain nicht
# installiert, wird sie übersprungen — mindestens eine muss laufen, und der
# ESP32-Build ist der aussagekräftige (nur dort gilt natürliches Alignment).

set -u

cd "$(dirname "$0")"

PIO_PACKAGES="${PLATFORMIO_CORE_DIR:-$HOME/.platformio}/packages"
SRC="test_wire_layout.cpp"
STD="-std=gnu++11"

ran=0
failed=0

run_case() {
    local label="$1" cxx="$2"
    shift 2

    if [ ! -x "$cxx" ]; then
        printf '  %-28s uebersprungen (Compiler nicht gefunden)\n' "$label"
        return
    fi

    ran=$((ran + 1))
    local out
    if out=$("$cxx" $STD "$@" -fsyntax-only "$SRC" 2>&1); then
        printf '  %-28s OK\n' "$label"
    else
        printf '  %-28s FEHLGESCHLAGEN\n' "$label"
        printf '%s\n' "$out" | grep -E 'error|static assertion' | sed 's/^/      /'
        failed=$((failed + 1))
    fi
}

# Erster Treffer gewinnt; die Pfade unterscheiden sich je nach PlatformIO-Version.
find_cxx() {
    local name="$1"
    shift
    for dir in "$@"; do
        if [ -x "$PIO_PACKAGES/$dir/bin/$name" ]; then
            echo "$PIO_PACKAGES/$dir/bin/$name"
            return
        fi
    done
    echo ""
}

AVR_CXX=$(find_cxx avr-g++ toolchain-atmelavr)
ESP_CXX=$(find_cxx xtensa-esp32-elf-g++ toolchain-xtensa-esp32 toolchain-xtensa32)

echo
echo "tWave Protocol — Wire-Format-Test"
echo "---------------------------------"

run_case "ATmega328P (avr-g++)"    "${AVR_CXX:-/nonexistent}" -mmcu=atmega328p
run_case "ESP32 (xtensa-esp32-g++)" "${ESP_CXX:-/nonexistent}"

# Host-Compiler ist optional und rein informativ — er sagt nichts über die
# Zielplattformen aus, findet aber Tippfehler ohne installierte Toolchain.
for host in g++ clang++; do
    if command -v "$host" >/dev/null 2>&1; then
        run_case "Host ($host, informativ)" "$(command -v "$host")"
        break
    fi
done

echo
if [ "$ran" -eq 0 ]; then
    echo "Keine Toolchain gefunden — Test konnte nicht ausgefuehrt werden."
    echo "PlatformIO-Pakete erwartet unter: $PIO_PACKAGES"
    exit 2
fi

if [ "$failed" -gt 0 ]; then
    echo "FEHLGESCHLAGEN: $failed von $ran Builds. Das Wire-Format hat sich geaendert."
    echo "Ist die Aenderung gewollt, muessen die Sollwerte in $SRC angepasst werden"
    echo "und alle Nodes im Feld neu geflasht werden."
    exit 1
fi

echo "Bestanden: $ran/$ran Builds, Layout unveraendert."
exit 0
