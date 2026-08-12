#pragma once

#include <stdint.h>

/**
 * tWave Protocol — Payload Structs
 *
 * Alle Structs sind mit #pragma pack(1) definiert, damit das
 * Speicherlayout auf AVR (ATmega328P) und ESP32 identisch ist.
 */

#pragma pack(push, 1)

/* --- Message Type Bytes ---
 *
 * ACHTUNG: Vier Paare unterscheiden sich nur in der Gross-/Kleinschreibung und
 * bedeuten voellig Verschiedenes:
 *
 *   'S' Session-Request   <-> 's' Schalter-Status
 *   'M' Maintenance       <-> 'm' Motion-Sensor
 *   'G' Gaszaehler        <-> 'g' Gateway-Status
 *   'R' Rolllade m. Pos.  <-> 'r' Rolllade o. Pos.
 *
 * Ein vertippter Buchstabe faellt daher nicht auf, sondern wird vom Gateway als
 * anderer Nachrichtentyp interpretiert. Immer diese Konstanten verwenden, nie
 * die Zeichenliterale.
 */

/* Protokoll / Steuerung */
#define TWAVE_MSG_SESSION_REQUEST  'S'   // twave_sessionRequest
#define TWAVE_MSG_SESSION_KEY      'K'   // twave_payload_K
#define TWAVE_MSG_COMMAND          'C'   // twave_payload_C
#define TWAVE_MSG_MAINTENANCE      'M'   // twave_payload_M  (Gateway -> Node)
#define TWAVE_MSG_MAINT_REPLY      'N'   // twave_payload_N  (Node -> Gateway)
#define TWAVE_MSG_MAINT_EXTENDED   'X'   // twave_payload_X

/* Aktor-Status — alle mit twave_payload_actuator, HA-Domain haengt am Type-Byte */
#define TWAVE_MSG_STATUS_SHUTTER   'R'   // cover / shutter  — Rolllade mit Position
#define TWAVE_MSG_STATUS_GARAGE    'r'   // cover / garage   — Rolllade ohne Position (auf/zu)
#define TWAVE_MSG_STATUS_SWITCH    's'   // switch           — Schalter / Steckdose
#define TWAVE_MSG_STATUS_TRIGGER   't'   // button           — an fuer motorDelay, dann aus
#define TWAVE_MSG_STATUS_LOCK      'l'   // lock
#define TWAVE_MSG_STATUS_VALVE     'v'   // valve
#define TWAVE_MSG_STATUS_PLAYER    'p'   // number + button  — MP3-Player (Titel/Volume)

/* Sensor-Status */
#define TWAVE_MSG_ENVIRONMENT      'E'   // twave_payload_E  — Temp/Hum/Druck/Batterie
#define TWAVE_MSG_BOOLEAN          'B'   // binary_sensor    — Zustand 0/1
#define TWAVE_MSG_MOTION           'm'   // binary_sensor    — wie 'B', aber off_delay 5 s
#define TWAVE_MSG_GASMETER         'G'   // sensor           — Gaszaehler (32 Bit)
#define TWAVE_MSG_JOYSTICK         'J'   // (noch keine HA-Discovery im Gateway)
#define TWAVE_MSG_ACCELERATION     'A'   // (noch keine HA-Discovery im Gateway)

/* Gateway selbst */
#define TWAVE_MSG_GATEWAY_STATUS   'g'   // sensor — status/rssi/ram/temp/hum/baro/iaq

/* --- Maintenance-Kommando-Nummern (cmd-Feld von twave_payload_M) --- */

/* Antwort-Code ohne zugehöriges Kommando: die Startmeldung, die ein Node
 * unaufgefordert nach dem Booten schickt (param1=NodeID, param2=PowerLevel). */
#define TWAVE_REPORT_STARTUP             1

#define TWAVE_MAINT_RESET              100
#define TWAVE_MAINT_NODEID_SET         101   // NodeID ändern, kein Neustart
#define TWAVE_MAINT_NODEID_SET_RESTART 102   // NodeID ändern, mit Neustart
#define TWAVE_MAINT_NETWORKID_SET      103   // NetworkID ändern, kein Neustart
#define TWAVE_MAINT_NETWORKID_RESTART  104   // NetworkID ändern, mit Neustart
#define TWAVE_MAINT_POWERLEVEL_SET     105   // TxPower ändern
#define TWAVE_MAINT_FORCE_PAIRING      128   // Pairing-Modus erzwingen
#define TWAVE_MAINT_ERASE_CONFIG       155   // Konfiguration löschen (EEPROM)
#define TWAVE_MAINT_READ_CONFIG        160   // Aktuelle Konfiguration auslesen

/* --- Event-Konstanten (newState-Feld von twave_payload_C) --- */

#define TWAVE_EVENT_HALT    0
#define TWAVE_EVENT_UP      1
#define TWAVE_EVENT_DOWN    2
#define TWAVE_EVENT_TARGET  3

/* --- Session --- */

struct twave_sessionRequest {
    uint8_t  type        = TWAVE_MSG_SESSION_REQUEST;
    uint8_t  reserved;
    uint16_t magic;
};

struct twave_payload_K {
    uint8_t  type        = 'K';
    uint8_t  RECEIVERID;
    uint16_t magic       = 19126;
    uint32_t SESSIONKEY;
};

/* --- Command --- */

struct twave_payload_C {
    uint8_t  type        = TWAVE_MSG_COMMAND;
    uint8_t  cmdType;       // 1=Verfahren, 2=Anlernen, 3=Pairen, 4=Status, 5=Ablernen
    uint8_t  channel;
    uint8_t  newState;      // 1=AN/HOCH, 2=AUS/RUNTER, 3=Ziel anfahren
    uint8_t  target;        // Zielposition in %
    uint8_t  statusRequired;
    uint16_t serial      = 555;
    uint32_t SessionKey;
};

/* --- Status --- */

struct twave_payload_actuator {    // Alle Aktoren (Rolllade, Schalter/Steckdose, Ventil)
    // ABSICHTLICH ohne Default: dieses Struct bedient sieben verschiedene
    // Type-Bytes ('R','r','s','t','l','v','p'). Der Node MUSS type selbst setzen,
    // sonst legt Home Assistant das falsche oder gar kein Geraet an.
    uint8_t  type;
    uint8_t  ReceiverAddress;
    uint8_t  channel;
    uint8_t  isMoving;
    uint8_t  success;
    uint8_t  toggleMode;
    uint16_t reserved2;
    uint32_t actPosition;
    uint32_t maxPosition;
    uint32_t SessionKey;
};

/* --- Environment / Sensor --- */

struct twave_payload_E {
    uint8_t  type     = 'E';
    uint8_t  battery;
    uint16_t voltage;
    float    temperature;
    float    humidity;
    float    pressure;
};

/* --- Maintenance --- */

struct twave_payload_M {
    uint8_t type      = TWAVE_MSG_MAINTENANCE;
    uint8_t cmd;
    uint8_t param1;
    uint8_t param2;
};

struct twave_payload_N {
    uint8_t type      = 'N';
    uint8_t cmd;
    uint8_t success;
    uint8_t param1;
    uint8_t param2;
    uint8_t reserved1;
    uint8_t reserved2;
    uint8_t reserved3;
};

struct twave_payload_X {
    uint8_t  type         = 'X';
    uint8_t  address;
    uint8_t  thisNodeID;
    uint8_t  networkID;
    uint8_t  powerLevel;
    uint8_t  noOfRetries;
    uint8_t  modemConfig;
    uint8_t  sendStatusTo;
    uint16_t sessionTimeout;
    uint16_t motorDelay;
};

/* --- Channel State (intern, nicht über Funk) --- */

struct twave_channelState {
    uint8_t  errorState;
    uint8_t  channel;
    uint8_t  actMode;
    uint8_t  actState;
    uint32_t actPosition;
    uint32_t maxPosition;
};

#pragma pack(pop)

/* --- Größen-Verifikation (schlägt beim Kompilieren an, wenn Padding falsch) --- */
static_assert(sizeof(twave_sessionRequest) == 4,  "sessionRequest size mismatch");
static_assert(sizeof(twave_payload_K)      == 8,  "payload_K size mismatch");
static_assert(sizeof(twave_payload_C)      == 12, "payload_C size mismatch");
static_assert(sizeof(twave_payload_actuator) == 20, "payload_actuator size mismatch");
static_assert(sizeof(twave_payload_E)      == 16, "payload_E size mismatch");
static_assert(sizeof(twave_payload_M)      == 4,  "payload_M size mismatch");
static_assert(sizeof(twave_payload_N)      == 8,  "payload_N size mismatch");
static_assert(sizeof(twave_payload_X)      == 12, "payload_X size mismatch");

/* --- Rückwärtskompatible Aliasnamen (deprecated, werden in v2.0 entfernt) --- */
typedef twave_sessionRequest sessionRequest;
typedef twave_payload_K      payload_K;
typedef twave_payload_C      payload_C;
typedef twave_payload_actuator payload_R;
typedef twave_payload_E      payload_E;
typedef twave_payload_M      payload_M;
typedef twave_payload_N      payload_N;
typedef twave_payload_X      payload_X;
typedef twave_channelState   channelState;
