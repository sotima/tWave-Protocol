#pragma once

#include <stdint.h>

/**
 * tWave Protocol — Payload Structs
 *
 * Alle Structs sind mit #pragma pack(1) definiert, damit das
 * Speicherlayout auf AVR (ATmega328P) und ESP32 identisch ist.
 */

#pragma pack(push, 1)

/* --- Message Type Bytes --- */
#define TWAVE_MSG_SESSION_REQUEST  'S'   // sessionRequest
#define TWAVE_MSG_SESSION_KEY      'K'   // payload_K
#define TWAVE_MSG_COMMAND          'C'   // payload_C
#define TWAVE_MSG_STATUS_SHUTTER   'R'   // payload_R  (Rolllade)
#define TWAVE_MSG_STATUS_SWITCH    's'   // payload_R  (Schalter/Steckdose)
#define TWAVE_MSG_STATUS_VALVE     'v'   // payload_L  (Ventil)
#define TWAVE_MSG_ENVIRONMENT      'E'   // payload_E  (Sensor)
#define TWAVE_MSG_MAINTENANCE      'M'   // payload_M
#define TWAVE_MSG_MAINT_REPLY      'N'   // payload_N
#define TWAVE_MSG_MAINT_EXTENDED   'X'   // payload_X

/* --- Session --- */

struct twave_sessionRequest {
    uint8_t  type;       // 'S'
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
    uint8_t  type;
    uint8_t  cmdType;       // 1=Verfahren, 2=Anlernen, 3=Pairen, 4=Status, 5=Ablernen
    uint8_t  channel;
    uint8_t  newState;      // 1=AN/HOCH, 2=AUS/RUNTER, 3=Ziel anfahren
    uint8_t  target;        // Zielposition in %
    uint8_t  statusRequired;
    uint16_t serial      = 555;
    uint32_t SessionKey;
};

/* --- Status --- */

struct twave_payload_R {    // Rolllade und Schalter/Steckdose
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

struct twave_payload_L {    // Ventil (bitweise identisch mit payload_R)
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
    uint8_t type;
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
static_assert(sizeof(twave_payload_R)      == 20, "payload_R size mismatch");
static_assert(sizeof(twave_payload_L)      == 20, "payload_L size mismatch");
static_assert(sizeof(twave_payload_E)      == 16, "payload_E size mismatch");
static_assert(sizeof(twave_payload_M)      == 4,  "payload_M size mismatch");
static_assert(sizeof(twave_payload_N)      == 8,  "payload_N size mismatch");
static_assert(sizeof(twave_payload_X)      == 12, "payload_X size mismatch");

/* --- Rückwärtskompatible Aliasnamen (deprecated, werden in v2.0 entfernt) --- */
typedef twave_sessionRequest sessionRequest;
typedef twave_payload_K      payload_K;
typedef twave_payload_C      payload_C;
typedef twave_payload_R      payload_R;
typedef twave_payload_L      payload_L;
typedef twave_payload_E      payload_E;
typedef twave_payload_M      payload_M;
typedef twave_payload_N      payload_N;
typedef twave_payload_X      payload_X;
typedef twave_channelState   channelState;
