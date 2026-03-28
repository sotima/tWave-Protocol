# tWave-Protocol

Gemeinsame Arduino/PlatformIO-Bibliothek für das tWave Smart-Home-System.
Enthält alle Payload-Structs, das Session-Key-Protokoll und das Maintenance-Protokoll,
die bisher in jedem Node und im Gateway dupliziert waren.

**Zielplattformen:** ATmega328P (8 MHz, 2 KB RAM) und ESP32

---

## Implementierungsplan

### Bibliotheksstruktur

```
tWave-Protocol/
├── library.json
├── library.properties
├── README.md
│
├── src/
│   ├── tWaveProtocol.h           ← Haupt-Include (zieht alles rein)
│   ├── tWavePayloads.h           ← Header-only: alle Payload-Structs
│   ├── tWaveConfig.h             ← Header-only: _config Struct + Validierung
│   ├── tWaveDebug.h              ← Header-only: ersetzt debugUtils.h
│   ├── tWaveSession.h / .cpp     ← Session-Key-Logik (Sender- und Empfänger-Seite)
│   └── tWaveMaintenance.h / .cpp ← Maintenance-Protokoll-Handler
│
└── examples/
    ├── ReceiverNode/ReceiverNode.ino
    └── SenderNode/SenderNode.ino
```

---

## Protokoll-Übersicht

### Payload-Typen

| Typ  | Struct              | Beschreibung                                      |
|------|---------------------|---------------------------------------------------|
| `S`  | `twave_sessionRequest` | Session-Anfrage (Sender → Empfänger)           |
| `K`  | `twave_payload_K`   | Session-Key-Antwort (Empfänger → Sender)          |
| `C`  | `twave_payload_C`   | Kommando (Sender → Empfänger, mit SessionKey)     |
| `R`  | `twave_payload_R`   | Rollladenstatus (Empfänger → Sender/Gateway)      |
| `v`  | `twave_payload_L`   | Ventilstatus (Empfänger → Sender/Gateway)         |
| `E`  | `twave_payload_E`   | Umgebungsdaten / Sensor (Node → Gateway)          |
| `M`  | `twave_payload_M`   | Maintenance-Befehl (Gateway → Node)               |
| `N`  | `twave_payload_N`   | Maintenance-Antwort (Node → Gateway)              |
| `X`  | `twave_payload_X`   | Extended Maintenance / Konfiguration              |

### Session-Key-Protokoll (S → K → C)

```
Sender                          Empfänger
  |                                 |
  |──── sessionRequest (S) ────────>|   magic = random()
  |                                 |
  |<─── payload_K (K) ─────────────|   SESSIONKEY = millis(), magic gespiegelt
  |                                 |
  |──── payload_C (C) ─────────────>|   enthält SESSIONKEY zur Verifikation
  |                                 |
  |<─── payload_R/L/N (Status) ─────|   enthält SESSIONKEY zur Zuordnung
```

Der SESSIONKEY läuft nach `config.sessionTimeout` (Default: 2500 ms) ab.

### Maintenance-Kommandos

| CMD | Beschreibung                       |
|-----|------------------------------------|
| 100 | Reset (ruft `setup()` neu auf)     |
| 101 | NodeID ändern (ohne Neustart)      |
| 102 | NodeID ändern (mit Neustart)       |
| 103 | NetworkID ändern (ohne Neustart)   |
| 104 | NetworkID ändern (mit Neustart)    |
| 105 | TxPower ändern                     |
| 128 | Pairing-Modus erzwingen            |
| 155 | Konfiguration löschen (EEPROM)     |
| 160 | Aktuelle Konfiguration auslesen    |

---

## Design-Prinzipien

### "Bring your own manager"

Die Library kapselt **keine** RadioHead-Objekte. `RH_RF69` und `RHReliableDatagram`
werden weiterhin im Node-Code erstellt. Die Library liefert Protokoll-Logik als
Hilfsfunktionen — der Node ruft `sendtoWait` selbst auf.

Vorteile:
- Keine RadioHead-Abhängigkeit in der Library selbst
- Kein Stack-Druck durch Library-interne Puffer
- Schrittweise Migration möglich (ein `case`-Block nach dem anderen)

### ATmega328P-Speicherbeschränkungen (2 KB RAM)

- Kein OOP / keine virtuellen Methoden
- `F()` Makro für alle String-Literals im Flash
- Keine `String`-Klasse, kein `new`, kein `malloc`
- Alle Puffer werden vom Aufrufer bereitgestellt
- `#pragma pack(1)` für plattformübergreifend identisches Struct-Layout (AVR ↔ ESP32)

### Naming Convention

- Structs: `twave_`-Präfix (z.B. `twave_payload_C`)
- Konstanten: `TWAVE_`-Präfix (z.B. `TWAVE_MSG_COMMAND`)
- Während Migration: Typedef-Aliasnamen für Rückwärtskompatibilität

```cpp
typedef twave_payload_C payload_C;  // Deprecated, wird in v2.0 entfernt
```

---

## Implementierungsreihenfolge

### Phase 1 — MVP

Eliminiert ~150–200 Zeilen Duplikation pro Node ohne Verhaltensänderung.

1. **`tWavePayloads.h`** — alle Structs mit `#pragma pack(1)`
2. **`tWaveConfig.h`** — `twave_config` Struct mit `NODE_ID`-Default + Validierungsfunktion
3. **`tWaveDebug.h`** — ersetzt alle lokalen `debugUtils.h`-Kopien
4. **`library.json`** + lokale Einbindung in `platformio.ini`

Lokale Einbindung (solange kein eigenes Repo):
```ini
[env]
lib_deps =
  mikem/RadioHead@1.120
  ${PROJECT_DIR}/../tWave-Protocol
```

### Phase 2 — Session & Maintenance

5. **`tWaveMaintenance.h/.cpp`** — `twave_buildMaintenanceReply()` + Kommando-Konstanten
6. **`tWaveSession.h/.cpp`** — `twave_createSessionKey()`, `twave_verifyCommandKey()`, `twave_session_state`

Beispiel nach Migration:

```cpp
// Vorher:
case 'S':
    if (payloadLen == sizeof(sessionRequest)) {
        if (isKnown || pairingMode) {
            sessionRequest req; memcpy(&req, buf, sizeof(req));
            createSessionKey(req.magic);  // greift intern auf manager zu
        }
    }
    break;

// Nachher:
case TWAVE_MSG_SESSION_REQUEST:
    if (payloadLen == sizeof(twave_sessionRequest)) {
        if (isKnown || pairingMode) {
            twave_sessionRequest req; memcpy(&req, buf, sizeof(req));
            twave_payload_K reply;
            twave_createSessionKey(req.magic, config.thisNodeID, sessionState, reply);
            memcpy(tx_buf, &reply, sizeof(reply));
            manager.sendtoWait(tx_buf, sizeof(reply), other_node);
        }
    }
    break;
```

### Phase 3 — Später

7. **Queue-Management** — nur ESP32, `#ifdef ESP32`-Guard, konfigurierbarer Ring-Buffer
8. **Sender-Session-Helper** — `twave_sendWithSession()` für Handsender-Pattern
9. **`payload_R` und `payload_L` konsolidieren** — beide Structs sind bitweise identisch

---

## Einbindung als GitHub-Repo (Phase 2+)

```ini
[env]
lib_deps =
  mikem/RadioHead@1.120
  https://github.com/sotima/tWave-Protocol.git#v1.0.0
```

---

## Bekannte Besonderheiten

- **`payload_R` vs. `payload_L`:** Bitweise identische Structs, unterscheiden sich nur im
  Typ-Buchstaben (`R` für Rollläden, `v` für Ventile). Konsolidierung in Phase 3.
- **Stack-Bug in `createSessionKey`:** Bisheriger Code allokiert lokal
  `uint8_t data[66]` auf dem Stack. Die Library-Version behebt das, indem der
  Aufrufer `tx_buf` (global) übergibt.
- **`NODE_ID`-Fallback:** `#ifndef NODE_ID #define NODE_ID 1 #endif` verhindert
  Compiler-Fehler wenn kein Build-Flag gesetzt ist.
- **ESP32-Padding:** Nach erster ESP32-Integration `sizeof()`-Checks mit
  `static_assert` verifizieren.
