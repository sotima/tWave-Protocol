#pragma once

#include <Arduino.h>
#include "tWavePayloads.h"
#include "tWaveDebug.h"

/**
 * tWave Protocol — Session-Key-Handling
 *
 * Ablauf einer Transaktion:
 *   1. Sender schickt 'S' (twave_sessionRequest) mit einer Zufalls-Magic
 *   2. Empfänger erzeugt einen Session-Key und antwortet mit 'K' (twave_payload_K)
 *   3. Sender schickt 'C' (twave_payload_C) mit genau diesem Key zurück
 *   4. Empfänger prüft Key + Timeout und führt das Kommando aus
 *
 * Der Session-Key ist immer nur für EIN Kommando gültig und wird bei der
 * Prüfung verbraucht — egal ob die Prüfung erfolgreich war oder nicht.
 *
 * Die Funktionen hier fassen das Funkmodul NICHT an: twave_createSessionKey()
 * füllt nur das Antwort-Struct, das Senden macht der Aufrufer. Damit entfällt
 * der 60-Byte-Stack-Puffer, den die alte Node-Implementierung angelegt hat
 * (auf dem ATmega328P mit 2 KB RAM durchaus relevant).
 */

/* --- Session-Zustand eines Nodes --- */

struct twave_session_state {
    uint32_t sessionKey       = 0;  // Offener Key, 0 = keine Session aktiv
    uint32_t sentAt           = 0;  // millis() zum Zeitpunkt des Sendens
    uint32_t activeSessionKey = 0;  // Key der laufenden Transaktion (für die Statusantwort)
};

/**
 * Erzeugt einen neuen Session-Key und füllt die 'K'-Antwort.
 * Ein evtl. noch offener Key wird dabei überschrieben.
 *
 * @param magic       Magic aus dem eingegangenen twave_sessionRequest
 * @param thisNodeID  Eigene Node-ID (landet als RECEIVERID in der Antwort)
 * @param state       Session-Zustand (wird aktualisiert)
 * @param reply       Antwort-Struct, das der Aufrufer anschließend versendet
 */
inline void twave_createSessionKey(uint16_t magic,
                                   uint8_t thisNodeID,
                                   twave_session_state& state,
                                   twave_payload_K& reply) {
    // millis() als Key-Quelle — 0 ist reserviert für "keine Session"
    state.sessionKey = millis();
    if (state.sessionKey == 0) {
        state.sessionKey = 1;
    }
    state.sentAt = millis();

    reply.type       = TWAVE_MSG_SESSION_KEY;
    reply.RECEIVERID = thisNodeID;
    reply.magic      = magic;
    reply.SESSIONKEY = state.sessionKey;
}

/**
 * Prüft den Session-Key eines eingegangenen Kommandos.
 *
 * Der offene Key wird in JEDEM Fall verbraucht (state.sessionKey = 0), damit ein
 * abgelehntes Kommando keinen zweiten Versuch mit demselben Key erlaubt.
 * Bei Erfolg wird der Key nach state.activeSessionKey übernommen — von dort
 * holt ihn die Statusantwort.
 *
 * @param receivedKey    SessionKey-Feld aus dem empfangenen twave_payload_C
 * @param state          Session-Zustand (wird aktualisiert)
 * @param sessionTimeout Gültigkeitsdauer in ms (aus twave_config.sessionTimeout)
 * @return               true wenn der Key gültig und nicht abgelaufen war
 */
inline bool twave_verifyCommandKey(uint32_t receivedKey,
                                   twave_session_state& state,
                                   uint16_t sessionTimeout) {
    bool valid = false;

    if (state.sessionKey > 0 && (millis() - state.sentAt) < sessionTimeout) {
        if (receivedKey == state.sessionKey) {
            state.activeSessionKey = state.sessionKey;
            valid = true;
        } else {
            DEBUGPRINTLN0(F("WRONG Sessionkey!"));
        }
    } else {
        DEBUGPRINTLN0(F("Session Timeout"));
    }

    state.sessionKey = 0;
    return valid;
}

/**
 * Bequemlichkeits-Variante: prüft zusätzlich die Payload-Länge.
 * Nur verfügbar, wenn das Kommando-Struct komplett vorliegt.
 */
inline bool twave_verifyCommandKey(const twave_payload_C& cmd,
                                   twave_session_state& state,
                                   uint16_t sessionTimeout) {
    return twave_verifyCommandKey(cmd.SessionKey, state, sessionTimeout);
}

/* --- Optionaler Sende-Helfer (nur wenn RadioHead eingebunden ist) --- */
#ifdef RHReliableDatagram_h

/**
 * Erzeugt den Session-Key und verschickt die 'K'-Antwort in einem Rutsch.
 *
 * Es werden nur sizeof(twave_payload_K) == 8 Bytes gesendet — die alte
 * Node-Implementierung hat stets den vollen 60-Byte-Puffer rausgeblasen.
 * Beide bekannten Empfänger (ESP32-Gateway und Handsender) lesen die Antwort
 * per memcpy fester Länge ein und prüfen die Payload-Länge nicht, die
 * Verkürzung ist also abwärtskompatibel und spart Airtime.
 *
 * @return true wenn das ACK des Empfängers kam
 */
inline bool twave_sendSessionKey(RHReliableDatagram& manager,
                                 uint16_t magic,
                                 uint8_t thisNodeID,
                                 uint8_t receiver,
                                 twave_session_state& state) {
    twave_payload_K reply;
    twave_createSessionKey(magic, thisNodeID, state, reply);

    if (!manager.sendtoWait((uint8_t*)&reply, sizeof(reply), receiver)) {
        DEBUGPRINTLN0(F("sendtoWait SessData failed"));
        return false;
    }
    DEBUGPRINT0(F("Sessionkey sent to: "));
    DEBUGPRINTLN0(receiver);
    return true;
}

#endif  // RHReliableDatagram_h
