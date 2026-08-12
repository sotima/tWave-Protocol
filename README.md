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
│   ├── tWaveSession.h            ← Header-only: Session-Key-Logik
│   └── tWaveMaintenance.h        ← Header-only: Maintenance-Antworten
│
├── test/
│   ├── run_tests.sh              ← Wire-Format-Test (AVR + ESP32)
│   ├── test_wire_layout.cpp      ← static_asserts auf Groessen und Offsets
│   └── gateway_reference.h       ← eingefrorene Gateway-Structs (Gegenprobe)
│
└── examples/
    ├── ReceiverNode/ReceiverNode.ino
    └── SenderNode/SenderNode.ino
```

### Tests

```bash
./test/run_tests.sh
```

Prüft, dass sich das Byte-Layout der Structs nicht unbemerkt ändert — auf beiden
Zielplattformen und gegen das noch nicht migrierte Gateway. Reiner
Compile-Zeit-Test, keine Hardware nötig. Details in [`test/README.md`](test/README.md).

**Vor jeder Änderung an `tWavePayloads.h` oder `tWaveConfig.h` ausführen.** Die
Structs gehen ohne Längen- oder Versionskennung über die Funkstrecke; ein
verschobenes Feld bricht die Verständigung mit allen nicht neu geflashten Nodes,
ohne dass es einen Fehler gibt.

Die gesamte Bibliothek ist header-only (`inline`). Ursprünglich waren
`tWaveSession` und `tWaveMaintenance` als `.h/.cpp` geplant; da alle Funktionen
sehr klein sind, spart das Inlining auf dem ATmega328P Flash und Aufrufoverhead —
und ungenutzte Funktionen kosten gar nichts.

---

## Protokoll-Übersicht

### Payload-Typen

| Typ | Konstante                  | Struct                   | HA-Domain       | Beschreibung                        |
|-----|----------------------------|--------------------------|-----------------|-------------------------------------|
| `S` | `TWAVE_MSG_SESSION_REQUEST`| `twave_sessionRequest`   | –               | Session-Anfrage (Sender → Empfänger)|
| `K` | `TWAVE_MSG_SESSION_KEY`    | `twave_payload_K`        | –               | Session-Key-Antwort                 |
| `C` | `TWAVE_MSG_COMMAND`        | `twave_payload_C`        | –               | Kommando (mit SessionKey)           |
| `M` | `TWAVE_MSG_MAINTENANCE`    | `twave_payload_M`        | –               | Maintenance-Befehl (Gateway → Node) |
| `N` | `TWAVE_MSG_MAINT_REPLY`    | `twave_payload_N`        | –               | Maintenance-Antwort (Node → Gateway)|
| `X` | `TWAVE_MSG_MAINT_EXTENDED` | `twave_payload_X`        | –               | Extended Maintenance / Konfiguration|
| `R` | `TWAVE_MSG_STATUS_SHUTTER` | `twave_payload_actuator` | `cover`/shutter | Rolllade mit Position               |
| `r` | `TWAVE_MSG_STATUS_GARAGE`  | `twave_payload_actuator` | `cover`/garage  | Rolllade ohne Position (auf/zu)     |
| `s` | `TWAVE_MSG_STATUS_SWITCH`  | `twave_payload_actuator` | `switch`        | Schalter / Steckdose                |
| `t` | `TWAVE_MSG_STATUS_TRIGGER` | `twave_payload_actuator` | `button`        | An für `motorDelay`, dann aus       |
| `l` | `TWAVE_MSG_STATUS_LOCK`    | `twave_payload_actuator` | `lock`          | Schloss                             |
| `v` | `TWAVE_MSG_STATUS_VALVE`   | `twave_payload_actuator` | `valve`         | Ventil                              |
| `p` | `TWAVE_MSG_STATUS_PLAYER`  | `twave_payload_actuator` | `number`+`button`| MP3-Player (Titel/Volume)          |
| `E` | `TWAVE_MSG_ENVIRONMENT`    | `twave_payload_E`        | `sensor`        | Temp/Hum/Druck/Batterie             |
| `B` | `TWAVE_MSG_BOOLEAN`        | –                        | `binary_sensor` | Zustand 0/1                         |
| `m` | `TWAVE_MSG_MOTION`         | –                        | `binary_sensor` | wie `B`, aber `off_delay` 5 s       |
| `G` | `TWAVE_MSG_GASMETER`       | –                        | `sensor`        | Gaszähler (32 Bit)                  |
| `J` | `TWAVE_MSG_JOYSTICK`       | –                        | –               | Handler im Gateway, keine Discovery |
| `A` | `TWAVE_MSG_ACCELERATION`   | –                        | –               | Handler im Gateway, keine Discovery |
| `g` | `TWAVE_MSG_GATEWAY_STATUS` | –                        | `sensor`        | Gateway selbst (rssi/ram/temp/…)    |

Die HA-Domain wird im Gateway allein aus dem Type-Byte abgeleitet
(`sendDiscoveryMessage()`), das der Node sendet — ein falscher Buchstabe legt
also das falsche Home-Assistant-Gerät an.

> **Groß-/Kleinschreibung beachten:** `S`↔`s`, `M`↔`m`, `G`↔`g` und `R`↔`r` sind
> jeweils zwei völlig verschiedene Nachrichtentypen. Immer die Konstanten
> verwenden, nie die Zeichenliterale.

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

### Phase 1 — MVP ✅ abgeschlossen (v0.1.0)

Eliminiert ~150–200 Zeilen Duplikation pro Node ohne Verhaltensänderung.
Alle 5 Nodes migriert und auf Hardware getestet.

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

### Phase 2 — Session & Maintenance ✅ Bibliothek fertig (v0.2.0), Nodes noch nicht migriert

5. **`tWaveMaintenance.h`** — `twave_buildMaintenanceReply()`, `twave_maintenanceAllowed()`,
   optional `twave_sendMaintenanceReply()`
6. **`tWaveSession.h`** — `twave_createSessionKey()`, `twave_verifyCommandKey()`,
   `twave_session_state`, optional `twave_sendSessionKey()`

Die `twave_send*()`-Helfer sind mit `#ifdef RHReliableDatagram_h` geklammert und
existieren nur, wenn RadioHead vor `tWaveProtocol.h` eingebunden wurde. Damit
bleibt die Bibliothek auch ohne RadioHead übersetzbar (z. B. für Host-Tests).

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

- **Default-Initialisierer für `type` sind Teil des Protokolls, kein Komfort.**
  Die sendenden Stellen (Gateway, Handsender) legen ihre Payload-Structs als
  Globals an und setzen nur die Nutzfelder — das Type-Byte kommt allein aus dem
  Default. Fehlt er, landet das Struct in `.bss`, das Type-Byte ist 0 und kein
  Empfänger erkennt die Nachricht. Genau das ist in v0.1.0 passiert:
  `twave_sessionRequest` und `twave_payload_C` hatten den Default verloren, der
  Handsender war dadurch von v0.1.0 bis v0.2.1 funktionsunfähig (behoben in
  v0.2.2). Einzige bewusste Ausnahme ist `twave_payload_actuator`, weil es sieben
  verschiedene Type-Bytes bedient — dort setzt der Node ihn selbst.

- **`payload_R` vs. `payload_L`:** ✅ erledigt in v0.1.0 — zu `twave_payload_actuator`
  zusammengeführt, `payload_R` bleibt als Alias.
- **Stack-Puffer in `createSessionKey`:** ✅ erledigt in v0.2.0 — die alte
  Node-Implementierung legt `uint8_t data[RH_RF69_MAX_MESSAGE_LEN]` (60 Byte) auf
  dem Stack an und sendet den kompletten Puffer. `twave_createSessionKey()` füllt
  nur noch das 8-Byte-Struct, das der Aufrufer direkt verschickt.
- **Verkürzte `K`-Nachricht:** Damit gehen statt 60 nur noch 8 Byte über die Luft.
  Beide bekannten Empfänger (ESP32-Gateway `main.cpp`, Handsender) lesen die
  Antwort per `memcpy` fester Länge und prüfen die Payload-Länge nicht — die
  Änderung ist abwärtskompatibel. Bei neuen Empfängern darauf achten.
- **`NODE_ID`-Fallback:** `#ifndef NODE_ID #define NODE_ID 1 #endif` verhindert
  Compiler-Fehler wenn kein Build-Flag gesetzt ist.
- **ESP32-Padding:** Nach erster ESP32-Integration `sizeof()`-Checks mit
  `static_assert` verifizieren.
