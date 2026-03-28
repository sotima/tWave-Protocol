#pragma once

#include <stdint.h>

/**
 * tWave Protocol — Netzwerkkonfiguration und Node-Config-Struct
 */

/* --- Netzwerk-Konstanten (müssen auf allen Nodes gleich sein) --- */

#define TWAVE_NETWORK_ID   212
#define TWAVE_FREQUENCY    868.0

// TWAVE_MODEM_CONFIG erfordert RadioHead — nur definieren wenn verfügbar
#ifdef RH_RF69_h
  #define TWAVE_MODEM_CONFIG  RH_RF69::GFSK_Rb250Fd250
#endif

// Sendeleistung: im Debug-Modus reduziert, sonst voll
#ifndef TWAVE_POWER_LEVEL
  #if DEBUG > -1
    #define TWAVE_POWER_LEVEL 10
  #else
    #define TWAVE_POWER_LEVEL 13
  #endif
#endif

/* --- Maintenance-Kommando-Nummern --- */

#define TWAVE_MAINT_RESET              100
#define TWAVE_MAINT_NODEID_SET         101   // NodeID ändern, kein Neustart
#define TWAVE_MAINT_NODEID_SET_RESTART 102   // NodeID ändern, mit Neustart
#define TWAVE_MAINT_NETWORKID_SET      103   // NetworkID ändern, kein Neustart
#define TWAVE_MAINT_NETWORKID_RESTART  104   // NetworkID ändern, mit Neustart
#define TWAVE_MAINT_POWERLEVEL_SET     105   // TxPower ändern
#define TWAVE_MAINT_FORCE_PAIRING      128   // Pairing-Modus erzwingen
#define TWAVE_MAINT_ERASE_CONFIG       155   // Konfiguration löschen (EEPROM)
#define TWAVE_MAINT_READ_CONFIG        160   // Aktuelle Konfiguration auslesen

/* --- Event-Konstanten (Fahrbefehle) --- */

#define TWAVE_EVENT_HALT    0
#define TWAVE_EVENT_UP      1
#define TWAVE_EVENT_DOWN    2
#define TWAVE_EVENT_TARGET  3

/* --- NODE_ID Fallback --- */

#ifndef NODE_ID
  #define NODE_ID 1
#endif

/* --- Node-Konfiguration (wird im EEPROM gespeichert) --- */

#pragma pack(push, 1)

struct twave_config {
    uint8_t  thisNodeID     = NODE_ID;
    uint8_t  networkID      = TWAVE_NETWORK_ID;
    uint8_t  powerLevel     = 13;
    uint8_t  noOfRetries    = 5;
    uint8_t  modemConfig    = 0;   // 0=GFSK_Rb250Fd250, 1=GFSK_Rb125Fd125, 2=GFSK_Rb55555Fd50
    uint8_t  sendStatusTo   = 0;
    uint16_t sessionTimeout = 2500;
    uint16_t motorDelay     = 0;
};

#pragma pack(pop)

static_assert(sizeof(twave_config) == 10, "twave_config size mismatch");

/* --- Hilfsfunktionen --- */

// Liefert true wenn nodeId eine Gateway-Adresse ist (Vielfaches von 10)
inline bool twave_isGateway(uint8_t nodeId) {
    return (nodeId % 10) == 0;
}

/**
 * Validiert und repariert eine aus dem EEPROM gelesene Konfiguration.
 * Alle Felder die außerhalb gültiger Grenzen liegen werden auf Defaults zurückgesetzt.
 *
 * @param cfg           Die zu prüfende Konfiguration (wird ggf. verändert)
 * @param defaultNodeID Fallback-NodeID wenn EEPROM nicht initialisiert (aus NODE_ID)
 */
inline void twave_validateConfig(twave_config& cfg, uint8_t defaultNodeID) {
    // Wenn EEPROM frisch (0xFF), sendStatusTo auf Gateway (0) setzen
    if (cfg.thisNodeID >= 255) {
        cfg.sendStatusTo = 0;
    }
    if (cfg.thisNodeID >= 255) {
        cfg.thisNodeID = defaultNodeID;
    }
    if (cfg.networkID >= 255) {
        cfg.networkID = TWAVE_NETWORK_ID;
    }
    if (cfg.powerLevel > 20) {
        cfg.powerLevel = 13;
    }
    if (cfg.noOfRetries > 10 || cfg.noOfRetries < 1) {
        cfg.noOfRetries = 5;
    }
    if (cfg.modemConfig >= 3) {
        cfg.modemConfig = 0;
    }
    if (cfg.sessionTimeout > 10000) {
        cfg.sessionTimeout = 2500;
    }
    if (cfg.motorDelay > 60000) {
        cfg.motorDelay = 0;
    }
}

/* --- Rückwärtskompatible Aliasnamen (deprecated) --- */
typedef twave_config _config;
