# Wire-Format-Test

Sichert ab, dass sich das Byte-Layout der Payload-Structs nicht unbemerkt ändert.

```bash
./test/run_tests.sh
```

Keine Hardware nötig, nichts wird geflasht. Der Test besteht ausschliesslich aus
`static_assert`s — das Skript übersetzt `test_wire_layout.cpp` einmal mit dem
AVR- und einmal mit dem ESP32-Compiler aus den PlatformIO-Paketen. Kompiliert es
durch, stimmt das Layout; sonst nennt die Fehlermeldung Struct und Feld.

## Worum es geht

Die Structs gehen roh per `memcpy` über die Funkstrecke. Sender und Empfänger
haben keinerlei Möglichkeit, ein abweichendes Layout zu bemerken — es gibt keine
Längen- oder Versionskennung im Protokoll. Wer ein Feld einfügt, umsortiert oder
seinen Typ wechselt, bricht die Verständigung mit **jedem nicht neu geflashten
Node im Feld**, und zwar lautlos: die Nachricht kommt an, wird angenommen und
falsch interpretiert.

Erschwerend kommen zwei Architekturen zusammen. Der ATmega328P legt Felder
byteweise ab, der ESP32 richtet sie natürlich aus. Ohne Gegenmassnahme wäre
`twave_payload_actuator` auf dem einen 18 und auf dem anderen 20 Bytes gross.
Dagegen wirken zwei Dinge: `#pragma pack(1)` und die `reserved`-Felder, die
Mehrbyte-Werte von Hand auf passende Grenzen schieben. Beides ist leicht
versehentlich zu zerstören, und genau davor schützt dieser Test.

## Was geprüft wird

1. **Plattform-Annahmen** — little-endian, `float` als 4-Byte-IEEE-754-single.
2. **Wire-Format** — Grösse, Alignment und jedes einzelne Feld-Offset aller
   Structs, die über Funk gehen, gegen fest hinterlegte Sollwerte.
3. **EEPROM-Layout** — `twave_config`. Geht nicht über Funk, wird aber per
   `EEPROM.put()` gespeichert, wobei die Nodes die Adresse aus
   `EEPROM.length() - sizeof(twave_config)` berechnen. Eine geänderte Grösse
   verschiebt den ganzen Block, und jeder Node im Feld liest nach dem Update
   Müll.
4. **Gegenprobe gegen das Gateway** — siehe unten.

Die Sollwerte stehen absichtlich als feste Zahlen im Test und werden nicht
gegeneinander abgeleitet: diese Zahlen *sind* das Protokoll. Ändert es sich
bewusst, gehören die neuen Werte in denselben Commit — dann steht die Änderung
im Diff und ist eine Entscheidung statt eines Versehens.

Nachweislich erkannt werden: eingefügte und entfernte Felder, Typwechsel,
Umsortierungen (auch von Feldern gleichen Typs, da namentlich geprüft wird),
ein verlorengegangenes `#pragma pack(1)` sowie Änderungen an `twave_config`.

Nicht erkennbar ist eine reine Bedeutungsänderung bei unverändertem Layout —
wenn etwa `success` künftig invertiert gemeint ist. Dagegen hilft kein
Layout-Test.

## Die Gateway-Gegenprobe

`gateway_reference.h` enthält eine eingefrorene Kopie der Structs aus
`ESP32-tWave-Gateway/src/main.cpp`, Stand Commit `e96b83d` (v9.18) — der
Fassung, die produktiv läuft. Das Gateway definiert seine Structs bislang
selbst und **ohne** `#pragma pack`.

Der Test stellt beide gegenüber und belegt damit: die gepackten Library-Structs
ergeben auf dem ESP32 exakt dasselbe Layout wie die heutigen Gateway-Structs.
Das Gateway kann also auf die Library umgestellt werden, ohne dass sich ein
Byte auf der Funkstrecke ändert und ohne einen einzigen Node im Feld anzufassen.

Aussagekräftig ist dieser Abschnitt nur im ESP32-Build — im AVR-Build ist er
trivial erfüllt, weil AVR ohnehin byteweise packt.

Die Datei ist eine Momentaufnahme und wird **nicht** nachgeführt. Ist das
Gateway migriert, hat sie ihren Zweck erfüllt und kann samt Abschnitt 4 des
Tests entfallen.

Für `'A'`, `'B'`, `'G'` und `'J'` gibt es in der Library noch keine Structs;
das Gateway definiert sie lokal. Bis sie nachgezogen sind, hält der Test nur
ihre Grösse fest.

## Wenn der Test fehlschlägt

Zuerst klären, ob die Änderung gewollt war.

*Unbeabsichtigt* — Änderung zurücknehmen. Der Test hat getan, wofür er da ist.

*Beabsichtigt* — die Sollwerte im Test anpassen und einplanen, dass **alle**
Nodes im Feld neu geflasht werden müssen, das Gateway eingeschlossen. Ein
gemischter Betrieb aus altem und neuem Layout funktioniert nicht und fällt
nicht sofort auf. Bei Aktoren mit Sessionschutz äussert sich das als sporadisch
abgelehnte Kommandos, nicht als klarer Fehler.
