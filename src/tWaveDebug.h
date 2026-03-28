#pragma once

/**
 * tWave Protocol — Debug-Utilities
 *
 * Ersetzt die lokalen debugUtils.h-Dateien in jedem Node-Projekt.
 *
 * DEBUG-Level per Build-Flag setzen: -DDEBUG=0 bis -DDEBUG=3
 * DEBUG=-1 schaltet alle Ausgaben ab (Produktion).
 *
 * DEBUGPRINT0 / DEBUGPRINTLN0  : Sehr wichtige Meldungen (immer bei DEBUG>=0)
 * DEBUGPRINT3 / DEBUGPRINTLN3  : Unwichtige Meldungen (nur bei DEBUG>=3)
 */

#ifndef DEBUG
  #define DEBUG -1
#endif

#if DEBUG >= 0
  #define DEBUGPRINT0(x)   Serial.print(x)
  #define DEBUGPRINTLN0(x) Serial.println(x)
#else
  #define DEBUGPRINT0(x)
  #define DEBUGPRINTLN0(x)
#endif

#if DEBUG >= 1
  #define DEBUGPRINT1(x)   Serial.print(x)
  #define DEBUGPRINTLN1(x) Serial.println(x)
#else
  #define DEBUGPRINT1(x)
  #define DEBUGPRINTLN1(x)
#endif

#if DEBUG >= 2
  #define DEBUGPRINT2(x)   Serial.print(x)
  #define DEBUGPRINTLN2(x) Serial.println(x)
#else
  #define DEBUGPRINT2(x)
  #define DEBUGPRINTLN2(x)
#endif

#if DEBUG >= 3
  #define DEBUGPRINT3(x)   Serial.print(x)
  #define DEBUGPRINTLN3(x) Serial.println(x)
#else
  #define DEBUGPRINT3(x)
  #define DEBUGPRINTLN3(x)
#endif

/* --- Port-Manipulation Hilfsmakros --- */
#define CLR(x, y) (x &= (~(1 << y)))
#define SET(x, y) (x |= (1 << y))

#define testPinLow(pin)  { digitalWrite(pin, LOW);  digitalWrite(pin, HIGH); }
#define testPinHigh(pin) { digitalWrite(pin, HIGH); digitalWrite(pin, LOW);  }
