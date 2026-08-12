#pragma once
#include <stdint.h>

/**
 * Referenz-Kopie der Struct-Definitionen aus dem ESP32-Gateway,
 * Stand ESP32-tWave-Gateway Commit e96b83d (v9.18) — also der Fassung, die
 * heute produktiv im Feld läuft und mit allen bestehenden Nodes spricht.
 *
 * ABSICHTLICH OHNE #pragma pack: genau so legt der ESP32-Compiler sie an.
 * Der Test in test_wire_layout.cpp stellt sie den gepackten Library-Structs
 * gegenüber. Stimmen alle Offsets überein, kann das Gateway auf die Library
 * umgestellt werden, ohne dass sich ein einziges Byte auf der Funkstrecke
 * ändert.
 *
 * Diese Datei ist eine eingefrorene Momentaufnahme und wird NICHT
 * nachgeführt. Sie dokumentiert, wogegen die Kompatibilität einmal
 * nachgewiesen wurde. Ist das Gateway erst auf die Library migriert, hat
 * sie ihren Zweck erfüllt und kann samt dem zugehörigen Testabschnitt raus.
 */

namespace gw_ref {

struct payload_C {
	uint8_t type = 'C';
	uint8_t cmdType;
	uint8_t channel;
	uint8_t newState;
	uint8_t target;
	uint8_t statusRequired;
	uint16_t serial = 555;
	uint32_t SessionKey;
};

struct payload_R {
	uint8_t type;
	uint8_t ReceiverAddress;
	uint8_t channel;
	uint8_t isMoving;
	uint8_t success;
	uint8_t toggleMode;
	uint16_t reserved2;
	uint32_t actPosition;
	uint32_t maxPosition;
	uint32_t SessionKey;
};

struct payload_K {
	uint8_t type;
	uint8_t RECEIVERID;
	uint16_t magic;
	uint32_t SESSIONKEY;
};

struct sessionRequest {
	uint8_t type = 'S';
	uint8_t reserved;
	uint16_t magic;
};

struct payload_E {
	uint8_t type;
	uint8_t battery;
	uint16_t voltage;
	float temperature;
	float humidity;
	float pressure;
};

struct payload_M {
	uint8_t type='M';
	uint8_t cmd;
	uint8_t param1;
	uint8_t param2;
};

struct payload_X {
	uint8_t type='X';
	uint8_t address;
	uint8_t thisNodeID;
	uint8_t networkID;
	uint8_t powerLevel;
	uint8_t noOfRetries;
	uint8_t modemConfig;
	uint8_t sendStatusTo;
	uint16_t sessionTimeout;
	uint16_t motorDelay;
};

struct payload_N {
	uint8_t type;
	uint8_t cmd;
	uint8_t success;
	uint8_t param1;
	uint8_t param2;
	uint8_t reserved1;
	uint8_t reserved2;
	uint8_t reserved3;
};

/* Diese vier hat das Gateway zusätzlich; in der Library gibt es sie (noch)
 * nicht, daher werden sie nur der Vollständigkeit halber mitgeführt und
 * unten gegen ihre erwartete Grösse geprüft. */

struct payload_A {
  uint8_t type = 'A';
  uint8_t state;
  uint8_t rotation;
  uint8_t reserved;
  uint16_t battery;
};

struct payload_B {
	uint8_t type = 'B';
	uint8_t state;
	uint8_t battery;
	uint8_t reserved;
	uint16_t voltage;
};

struct payload_G {
	uint8_t type = 'G';
	uint8_t battery;
	uint16_t voltage;
	uint32_t counts;
};

struct payload_J {
	uint8_t type = 'J';
	uint8_t state;
	int8_t joy_x;
	int8_t joy_y;
	uint8_t battery;
	uint8_t reserved;
	uint16_t voltage;
};

} // namespace gw_ref
