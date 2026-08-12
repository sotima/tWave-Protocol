#pragma once

#include <stdint.h>
#include "tWavePayloads.h"
#include "tWaveConfig.h"
#include "tWaveDebug.h"

/**
 * tWave Protocol — Maintenance-Antworten
 *
 * Wartungskommandos kommen als 'M' (twave_payload_M) herein, die Antwort geht
 * als 'N' (twave_payload_N) zurück. Die Kommando-Nummern selbst (TWAVE_MAINT_*)
 * stehen in tWaveConfig.h.
 *
 * Konvention: das cmd-Feld der Antwort wiederholt die Kommando-Nummer, damit
 * das Gateway die Antwort zuordnen kann. Ausnahme ist die Startmeldung, die
 * ein Node unaufgefordert nach dem Booten schickt.
 */

/* TWAVE_REPORT_STARTUP steht in tWavePayloads.h bei den übrigen
 * cmd-Feldwerten und kommt über den Include oben mit. */

/**
 * Baut eine Maintenance-Antwort.
 *
 * @param cmd     Kommando-Nummer, auf die geantwortet wird (TWAVE_MAINT_*)
 * @param success 1 = ausgeführt, 0 = abgelehnt/fehlgeschlagen
 * @param param1  Kontextabhängig (meist die betroffene ID oder der alte Wert)
 * @param param2  Kontextabhängig (meist der neue Wert)
 */
inline twave_payload_N twave_buildMaintenanceReply(uint8_t cmd,
                                                   uint8_t success,
                                                   uint8_t param1,
                                                   uint8_t param2) {
    twave_payload_N reply;
    reply.type    = TWAVE_MSG_MAINT_REPLY;
    reply.cmd     = cmd;
    reply.success = success;
    reply.param1  = param1;
    reply.param2  = param2;
    reply.reserved1 = 0;
    reply.reserved2 = 0;
    reply.reserved3 = 0;
    return reply;
}

/**
 * Prüft, ob ein Maintenance-Kommando überhaupt für diesen Node gilt.
 *
 * Die Aktor-Nodes akzeptieren Wartungskommandos nur von einem Gateway
 * (Node-ID als Vielfaches von 10) und nur als gezielte Nachricht, nie als
 * Broadcast — sonst würde ein "NodeID ändern" alle Nodes gleichzeitig treffen.
 *
 * @param senderNodeID Absender der Nachricht
 * @param targetID     Empfängerfeld aus dem RadioHead-Header
 * @param thisNodeID   Eigene Node-ID
 */
inline bool twave_maintenanceAllowed(uint8_t senderNodeID,
                                     uint8_t targetID,
                                     uint8_t thisNodeID) {
    return twave_isGateway(senderNodeID) && targetID == thisNodeID;
}

/* --- Optionaler Sende-Helfer (nur wenn RadioHead eingebunden ist) --- */
#ifdef RHReliableDatagram_h

/**
 * Baut die Antwort und verschickt sie.
 *
 * An Adresse 255 (Broadcast) geht die Nachricht ohne ACK raus — darauf kann
 * per Definition niemand quittieren. Alle anderen Adressen werden bestätigt.
 *
 * @return true wenn versendet (Broadcast immer, sonst nur bei ACK)
 */
inline bool twave_sendMaintenanceReply(RHReliableDatagram& manager,
                                       uint8_t receiver,
                                       uint8_t cmd,
                                       uint8_t success,
                                       uint8_t param1,
                                       uint8_t param2) {
    DEBUGPRINT0(F("Send maintenance message to: "));
    DEBUGPRINTLN0(receiver);

    twave_payload_N reply = twave_buildMaintenanceReply(cmd, success, param1, param2);

    if (receiver == 255) {
        manager.sendto((uint8_t*)&reply, sizeof(reply), receiver);
        return true;
    }

    if (!manager.sendtoWait((uint8_t*)&reply, sizeof(reply), receiver)) {
        DEBUGPRINTLN0(F("Send maintenance report failed"));
        return false;
    }
    return true;
}

#endif  // RHReliableDatagram_h
