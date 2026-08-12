/**
 * tWave Protocol — Wire-Format-Test
 *
 * Reiner Compile-Zeit-Test: es gibt nichts auszuführen und nichts zu flashen.
 * Kompiliert diese Datei durch, stimmt das Layout; schlägt ein static_assert
 * an, nennt die Meldung Struct und Feld.
 *
 * Ausgeführt wird er über test/run_tests.sh, das ihn nacheinander mit dem
 * AVR- und dem ESP32-Compiler übersetzt. Beide Plattformen müssen dasselbe
 * Layout ergeben, sonst reden ATmega-Nodes und Gateway aneinander vorbei.
 *
 * Geprüft wird:
 *   1. Plattform-Annahmen (Byte-Reihenfolge, float-Format)
 *   2. Wire-Format: Grösse und jedes Feld-Offset jedes Funk-Structs
 *   3. EEPROM-Layout von twave_config
 *   4. Gegenprobe gegen die Structs des noch nicht migrierten Gateways
 *
 * Warum feste Sollwerte statt eines Vergleichs untereinander: die Zahlen hier
 * SIND das Protokoll. Sie beschreiben, was die Geräte im Feld senden und
 * erwarten. Wer ein Feld einfügt, umsortiert oder den Typ wechselt, bricht
 * die Kompatibilität zu jedem nicht neu geflashten Node — und genau dann soll
 * dieser Test anschlagen und nicht stillschweigend neue Werte übernehmen.
 *
 * Ändert sich das Protokoll bewusst, müssen die Sollwerte hier mit dem
 * gleichen Commit angepasst werden. Das ist Absicht: die Änderung wird so im
 * Diff sichtbar.
 */

#include <stddef.h>
#include <stdint.h>

#include "../src/tWavePayloads.h"
#include "../src/tWaveConfig.h"
#include "gateway_reference.h"

/* ------------------------------------------------------------------ */
/* Hilfsmakros                                                         */
/* ------------------------------------------------------------------ */

/* Prüft Grösse und Alignment.
 *
 * Das Alignment-Kriterium sichert #pragma pack(1) selbst ab. Ohne pack
 * verlangt der ESP32 für ein Struct mit uint32-Feldern Alignment 4; gepackt
 * ist es 1. Ohne diese Prüfung könnte pack(1) verlorengehen, ohne dass der
 * Test anschlägt — die Structs sind dank ihrer reserved-Felder nämlich auch
 * ungepackt zufällig gleich gross. Das gilt aber nur, solange niemand ein
 * Feld anfasst; die Prüfung hält die Absicherung selbst am Leben.
 * Auf AVR ist die Bedingung trivial erfüllt (dort ist jedes Alignment 1),
 * greifen tut sie im ESP32-Build. */
#define WIRE_SIZE(T, N) \
    static_assert(sizeof(T) == (N), \
        #T ": Groesse weicht ab — Wire-Format gebrochen"); \
    static_assert(alignof(T) == 1, \
        #T ": Alignment != 1 — fehlt #pragma pack(1)?")

#define WIRE_OFF(T, F, N) \
    static_assert(offsetof(T, F) == (N), \
        #T "." #F ": Offset weicht ab — Wire-Format gebrochen")

// Gegenprobe Library-Struct <-> Gateway-Struct
#define GW_SIZE(LIB, GW) \
    static_assert(sizeof(LIB) == sizeof(gw_ref::GW), \
        #LIB " vs Gateway " #GW ": Groesse weicht ab")

#define GW_OFF(LIB, GW, F) \
    static_assert(offsetof(LIB, F) == offsetof(gw_ref::GW, F), \
        #LIB " vs Gateway " #GW "." #F ": Offset weicht ab")

/* ------------------------------------------------------------------ */
/* 1. Plattform-Annahmen                                               */
/* ------------------------------------------------------------------ */

// Structs gehen roh per memcpy über die Luft — ohne Byte-Swap. Beide
// Zielplattformen sind little-endian; auf einer big-endian-Plattform wären
// alle Mehrbyte-Felder vertauscht.
#if defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__)
static_assert(__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__,
    "tWave setzt little-endian voraus — Mehrbyte-Felder waeren vertauscht");
#endif

// twave_payload_E überträgt drei floats binär. Nur mit 4-Byte-IEEE-754-single
// liest die Gegenseite dieselben Zahlen. (AVR erfüllt das; dort ist allerdings
// double ebenfalls 4 Byte — genau deshalb steht im Struct float und nicht double.)
static_assert(sizeof(float) == 4, "float ist nicht 4 Byte — payload_E waere inkompatibel");
#if defined(__FLT_MANT_DIG__)
static_assert(__FLT_MANT_DIG__ == 24, "float ist kein IEEE-754 single precision");
#endif

static_assert(sizeof(uint8_t) == 1, "uint8_t ist nicht 1 Byte");

/* ------------------------------------------------------------------ */
/* 2. Wire-Format                                                      */
/* ------------------------------------------------------------------ */

/* --- 'S' Session-Request --- */
WIRE_SIZE(twave_sessionRequest, 4);
WIRE_OFF(twave_sessionRequest, type,     0);
WIRE_OFF(twave_sessionRequest, reserved, 1);
WIRE_OFF(twave_sessionRequest, magic,    2);

/* --- 'K' Session-Key --- */
WIRE_SIZE(twave_payload_K, 8);
WIRE_OFF(twave_payload_K, type,       0);
WIRE_OFF(twave_payload_K, RECEIVERID, 1);
WIRE_OFF(twave_payload_K, magic,      2);
WIRE_OFF(twave_payload_K, SESSIONKEY, 4);

/* --- 'C' Kommando --- */
WIRE_SIZE(twave_payload_C, 12);
WIRE_OFF(twave_payload_C, type,           0);
WIRE_OFF(twave_payload_C, cmdType,        1);
WIRE_OFF(twave_payload_C, channel,        2);
WIRE_OFF(twave_payload_C, newState,       3);
WIRE_OFF(twave_payload_C, target,         4);
WIRE_OFF(twave_payload_C, statusRequired, 5);
WIRE_OFF(twave_payload_C, serial,         6);
WIRE_OFF(twave_payload_C, SessionKey,     8);

/* --- 'R','r','s','t','l','v','p' Aktor-Status ---
 * reserved2 @6 ist kein Füllsel, sondern hält actPosition auf einer durch 4
 * teilbaren Adresse. Ohne das Feld würde ein nicht gepackter Compiler zwei
 * Bytes einschieben und ein gepackter nicht — das Layout liefe auseinander. */
WIRE_SIZE(twave_payload_actuator, 20);
WIRE_OFF(twave_payload_actuator, type,            0);
WIRE_OFF(twave_payload_actuator, ReceiverAddress, 1);
WIRE_OFF(twave_payload_actuator, channel,         2);
WIRE_OFF(twave_payload_actuator, isMoving,        3);
WIRE_OFF(twave_payload_actuator, success,         4);
WIRE_OFF(twave_payload_actuator, toggleMode,      5);
WIRE_OFF(twave_payload_actuator, reserved2,       6);
WIRE_OFF(twave_payload_actuator, actPosition,     8);
WIRE_OFF(twave_payload_actuator, maxPosition,    12);
WIRE_OFF(twave_payload_actuator, SessionKey,     16);

/* --- 'E' Umgebungsdaten ---
 * voltage @2 hält die drei floats auf 4er-Grenzen, gleiche Begründung. */
WIRE_SIZE(twave_payload_E, 16);
WIRE_OFF(twave_payload_E, type,         0);
WIRE_OFF(twave_payload_E, battery,      1);
WIRE_OFF(twave_payload_E, voltage,      2);
WIRE_OFF(twave_payload_E, temperature,  4);
WIRE_OFF(twave_payload_E, humidity,     8);
WIRE_OFF(twave_payload_E, pressure,    12);

/* --- 'M' Maintenance-Kommando --- */
WIRE_SIZE(twave_payload_M, 4);
WIRE_OFF(twave_payload_M, type,   0);
WIRE_OFF(twave_payload_M, cmd,    1);
WIRE_OFF(twave_payload_M, param1, 2);
WIRE_OFF(twave_payload_M, param2, 3);

/* --- 'N' Maintenance-Antwort --- */
WIRE_SIZE(twave_payload_N, 8);
WIRE_OFF(twave_payload_N, type,      0);
WIRE_OFF(twave_payload_N, cmd,       1);
WIRE_OFF(twave_payload_N, success,   2);
WIRE_OFF(twave_payload_N, param1,    3);
WIRE_OFF(twave_payload_N, param2,    4);
WIRE_OFF(twave_payload_N, reserved1, 5);
WIRE_OFF(twave_payload_N, reserved2, 6);
WIRE_OFF(twave_payload_N, reserved3, 7);

/* --- 'X' Erweiterte Maintenance / Konfigurationsübertragung --- */
WIRE_SIZE(twave_payload_X, 12);
WIRE_OFF(twave_payload_X, type,           0);
WIRE_OFF(twave_payload_X, address,        1);
WIRE_OFF(twave_payload_X, thisNodeID,     2);
WIRE_OFF(twave_payload_X, networkID,      3);
WIRE_OFF(twave_payload_X, powerLevel,     4);
WIRE_OFF(twave_payload_X, noOfRetries,    5);
WIRE_OFF(twave_payload_X, modemConfig,    6);
WIRE_OFF(twave_payload_X, sendStatusTo,   7);
WIRE_OFF(twave_payload_X, sessionTimeout, 8);
WIRE_OFF(twave_payload_X, motorDelay,    10);

/* ------------------------------------------------------------------ */
/* 3. EEPROM-Layout                                                    */
/* ------------------------------------------------------------------ */

/* twave_config geht nicht über Funk, wird aber per EEPROM.put() gespeichert —
 * und die Nodes berechnen die Speicheradresse aus EEPROM.length() minus
 * sizeof(twave_config). Eine geänderte Grösse verschiebt damit nicht nur die
 * Felder, sondern den gesamten Block: jeder Node im Feld liest nach dem
 * Update Müll und fällt auf die Defaults zurück. Deshalb hier genauso
 * festgenagelt wie das Funkprotokoll.
 *
 * twave_payload_X überträgt dieselben acht Felder — die Reihenfolge muss zu
 * twave_config passen, sonst schreibt ein 'X'-Kommando die Werte vertauscht. */
WIRE_SIZE(twave_config, 10);
WIRE_OFF(twave_config, thisNodeID,     0);
WIRE_OFF(twave_config, networkID,      1);
WIRE_OFF(twave_config, powerLevel,     2);
WIRE_OFF(twave_config, noOfRetries,    3);
WIRE_OFF(twave_config, modemConfig,    4);
WIRE_OFF(twave_config, sendStatusTo,   5);
WIRE_OFF(twave_config, sessionTimeout, 6);
WIRE_OFF(twave_config, motorDelay,     8);

/* twave_channelState ist rein intern (Laufzeit-Zustand je Kanal) und geht
 * weder über Funk noch ins EEPROM — hier nur die Grösse, damit ein
 * versehentlich eingefügtes Feld auffällt. */
WIRE_SIZE(twave_channelState, 12);

/* ------------------------------------------------------------------ */
/* 4. Gegenprobe: Gateway (noch nicht migriert)                        */
/* ------------------------------------------------------------------ */

/* Das Gateway definiert seine Structs bislang selbst, ohne #pragma pack.
 * Diese Gegenprobe belegt, dass die gepackten Library-Structs auf dem ESP32
 * exakt dasselbe Layout ergeben — Voraussetzung dafür, das Gateway auf die
 * Library umzustellen, ohne die nicht migrierten Nodes im Feld anzufassen.
 *
 * Aussagekräftig ist dieser Abschnitt vor allem im ESP32-Build: dort gilt
 * natürliches Alignment, und nur dort kann er überhaupt anschlagen. Im
 * AVR-Build ist er trivial erfüllt, weil AVR ohnehin byteweise packt. */

GW_SIZE(twave_sessionRequest, sessionRequest);
GW_OFF(twave_sessionRequest, sessionRequest, type);
GW_OFF(twave_sessionRequest, sessionRequest, reserved);
GW_OFF(twave_sessionRequest, sessionRequest, magic);

GW_SIZE(twave_payload_K, payload_K);
GW_OFF(twave_payload_K, payload_K, type);
GW_OFF(twave_payload_K, payload_K, RECEIVERID);
GW_OFF(twave_payload_K, payload_K, magic);
GW_OFF(twave_payload_K, payload_K, SESSIONKEY);

GW_SIZE(twave_payload_C, payload_C);
GW_OFF(twave_payload_C, payload_C, type);
GW_OFF(twave_payload_C, payload_C, cmdType);
GW_OFF(twave_payload_C, payload_C, channel);
GW_OFF(twave_payload_C, payload_C, newState);
GW_OFF(twave_payload_C, payload_C, target);
GW_OFF(twave_payload_C, payload_C, statusRequired);
GW_OFF(twave_payload_C, payload_C, serial);
GW_OFF(twave_payload_C, payload_C, SessionKey);

GW_SIZE(twave_payload_actuator, payload_R);
GW_OFF(twave_payload_actuator, payload_R, type);
GW_OFF(twave_payload_actuator, payload_R, ReceiverAddress);
GW_OFF(twave_payload_actuator, payload_R, channel);
GW_OFF(twave_payload_actuator, payload_R, isMoving);
GW_OFF(twave_payload_actuator, payload_R, success);
GW_OFF(twave_payload_actuator, payload_R, toggleMode);
GW_OFF(twave_payload_actuator, payload_R, reserved2);
GW_OFF(twave_payload_actuator, payload_R, actPosition);
GW_OFF(twave_payload_actuator, payload_R, maxPosition);
GW_OFF(twave_payload_actuator, payload_R, SessionKey);

GW_SIZE(twave_payload_E, payload_E);
GW_OFF(twave_payload_E, payload_E, type);
GW_OFF(twave_payload_E, payload_E, battery);
GW_OFF(twave_payload_E, payload_E, voltage);
GW_OFF(twave_payload_E, payload_E, temperature);
GW_OFF(twave_payload_E, payload_E, humidity);
GW_OFF(twave_payload_E, payload_E, pressure);

GW_SIZE(twave_payload_M, payload_M);
GW_OFF(twave_payload_M, payload_M, type);
GW_OFF(twave_payload_M, payload_M, cmd);
GW_OFF(twave_payload_M, payload_M, param1);
GW_OFF(twave_payload_M, payload_M, param2);

GW_SIZE(twave_payload_N, payload_N);
GW_OFF(twave_payload_N, payload_N, type);
GW_OFF(twave_payload_N, payload_N, cmd);
GW_OFF(twave_payload_N, payload_N, success);
GW_OFF(twave_payload_N, payload_N, param1);
GW_OFF(twave_payload_N, payload_N, param2);
GW_OFF(twave_payload_N, payload_N, reserved1);
GW_OFF(twave_payload_N, payload_N, reserved2);
GW_OFF(twave_payload_N, payload_N, reserved3);

GW_SIZE(twave_payload_X, payload_X);
GW_OFF(twave_payload_X, payload_X, type);
GW_OFF(twave_payload_X, payload_X, address);
GW_OFF(twave_payload_X, payload_X, thisNodeID);
GW_OFF(twave_payload_X, payload_X, networkID);
GW_OFF(twave_payload_X, payload_X, powerLevel);
GW_OFF(twave_payload_X, payload_X, noOfRetries);
GW_OFF(twave_payload_X, payload_X, modemConfig);
GW_OFF(twave_payload_X, payload_X, sendStatusTo);
GW_OFF(twave_payload_X, payload_X, sessionTimeout);
GW_OFF(twave_payload_X, payload_X, motorDelay);

/* Für 'A', 'B', 'G' und 'J' gibt es in der Library noch kein Struct — das
 * Gateway definiert sie lokal. Bis sie nachgezogen sind, wird hier nur die
 * Grösse festgehalten, damit ein späteres Library-Struct dagegen geprüft
 * werden kann. */
static_assert(sizeof(gw_ref::payload_A) == 6, "Gateway payload_A: Groesse weicht ab");
static_assert(sizeof(gw_ref::payload_B) == 6, "Gateway payload_B: Groesse weicht ab");
static_assert(sizeof(gw_ref::payload_G) == 8, "Gateway payload_G: Groesse weicht ab");
static_assert(sizeof(gw_ref::payload_J) == 8, "Gateway payload_J: Groesse weicht ab");

/* Kein main(): die Datei wird nur übersetzt, nicht gelinkt. */
