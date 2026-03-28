#pragma once

/**
 * tWave-Protocol — Haupt-Include
 *
 * Bindet alle Protokoll-Komponenten ein.
 * Verwendung in platformio.ini:
 *
 *   lib_deps =
 *     ${PROJECT_DIR}/../tWave-Protocol        ; lokal
 *     ; oder:
 *     https://github.com/sotima/tWave-Protocol.git#v1.0.0
 *
 * Im Code:
 *   #include <tWaveProtocol.h>
 */

#include "tWaveDebug.h"
#include "tWaveConfig.h"
#include "tWavePayloads.h"
