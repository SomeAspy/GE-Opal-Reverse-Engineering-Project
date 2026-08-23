// Copyright (C) 2026 SomeAspy (Aiden B. amb@aspy.dev)
// SPDX-License-Identifier: GPL-3.0-only
// https://github.com/someaspy/GE-Opal-2-fixes

// Eventually I want this code to use the front panel that came with the
// machine, but the LED driver on the daughterboard died so I have to wait for a
// new one to come in
// This is also the case with the bin LED, which I fried by putting 12v through
// it

#include "HardwareSerial.h"
#include <Arduino.h>
#include <EmonLib.h>

// Pins
const int PowerSwitch = 2;
const int BinSwitch = 3;
const int TankFull = 5;
const int TankEmpty = 6;
const int IrBlaster = 7;
const int Compressor = 8;
const int Auger = 9;
const int Fan = 10;
const int UvLed = 11;
const int Pump = 12;
const int MotorAmmeter = A0;
// Using analog because the voltage isn't enough to trigger digital high (~1.6v)
const int IrReceiver = A1;

EnergyMonitor augerMeter;

void setup() {
  Serial.begin(115200);

  // Grounded inputs need to be pulled up
  pinMode(PowerSwitch, INPUT_PULLUP);
  pinMode(BinSwitch, INPUT_PULLUP);
  pinMode(TankFull, INPUT_PULLUP);
  pinMode(TankEmpty, INPUT_PULLUP);

  // Standard inputs
  pinMode(IrReceiver, INPUT);
  pinMode(MotorAmmeter, INPUT);

  // Outputs
  pinMode(IrBlaster, OUTPUT);
  pinMode(Compressor, OUTPUT);
  pinMode(Auger, OUTPUT);
  pinMode(Fan, OUTPUT);
  pinMode(UvLed, OUTPUT);
  pinMode(Pump, OUTPUT);

  // Calibration factor by gemini, because I hate complex math
  augerMeter.current(MotorAmmeter, 5.76);
};

// Water management
bool pumping = false;
unsigned long pumpStartedTime = 0;
const unsigned long pumpTimeout = 120000; // 2m - Filter can drop flow a lot

// Compressor management
// If the machine is off, assume its been off for a safe amount of time
unsigned long compressorStopTime = 300001;
unsigned long compressorStartTime = 0;
const unsigned long compressorTimeout = 300000; // 5m
bool isCompressorRunning = false;

// Auger & defrost cycle management
// From testing the machine usually settles around 0.45A to 0.48A.
// 0.05 means the compressor is off.
// Note currentDrawLimit is ignored when the compressor first starts to
// accomodate inrush current. Tweak as needed.
const float currentDrawLimit = 0.50;
const unsigned long augerMotorGracePeriod = 15000; // 15s for inrush to settle
const unsigned long defrostCycleLength = 600000;   // 10m
bool defrostCycle = false;
unsigned long defrostCycleStartTime = 0;

// Bin management
const unsigned long binFullDelay = 10000;
const unsigned long binFullCooldown = 3600000;
unsigned long binCheck = 0;
unsigned long binLastFullAt = 0;
bool binFull = false;

// General state initialization
bool waitingForReset = false;

void loop() {
  // DIGITAL READS ARE INVERTED BECAUSE WE PULL UP!!!
  // Read values for the current cycle
  const bool isPowered = digitalRead(PowerSwitch) == LOW;
  const bool isTankFull = digitalRead(TankFull) == LOW;
  const bool isTankEmpty = digitalRead(TankEmpty) == LOW;
  const bool isBinInserted = digitalRead(BinSwitch) == LOW;
  // IRM sample count by Gemini, because I can't be bothered.
  const double currentDraw = augerMeter.calcIrms(1480);
  bool IrReceiving = false;

  // IR Receiver needs a moment to register it's state
  digitalWrite(IrBlaster, HIGH);
  delay(4);
  const int IrVoltage = analogRead(IrReceiver);
  if (IrVoltage >= 200) { // Around 1v
    IrReceiving = true;
  }
  digitalWrite(IrBlaster, LOW);

  Serial.println(currentDraw);
  if (IrReceiving == true) {
    Serial.println(millis());
    Serial.println("can see");
  }

  // When the machine should be stopped
  if (!isPowered || waitingForReset || defrostCycle || binFull) {
    Serial.println("Halted");
    digitalWrite(Pump, LOW);
    digitalWrite(Compressor, LOW);
    digitalWrite(Fan, LOW);
    digitalWrite(UvLed, LOW);
    digitalWrite(Auger, LOW);
    pumping = false;

    // Start compressor cooldown
    if (isCompressorRunning) {
      compressorStopTime = millis();
      isCompressorRunning = false;
    }

    // If the bin is open we can assume ice was removed
    if (!isBinInserted) {
      waitingForReset = false;
      binFull = false;
      binCheck = 0;
    }
    // If the defrost cycle triggered the halt and is done, start the machine
    if (millis() - defrostCycleStartTime > defrostCycleLength &&
        defrostCycle == true) {
      defrostCycle = false;
    }

    // If the bin being filled triggered the halt, clear once ice making
    // cooldown finishes
    if (millis() - binLastFullAt > binFullCooldown && binFull == true) {
      binFull = false;
    }
    return;
  }

  // If the pump doesn't move enough water into the tank, something is wrong
  if (pumping && (millis() - pumpStartedTime > pumpTimeout)) {
    // We are probably out of water
    Serial.println("no water?");
    waitingForReset = true;
    return;
  }

  // Don't stop pumping once the lower float rises, so we can fill it up to the
  // upper float to save pump spam
  if (isTankEmpty && !pumping) {
    digitalWrite(UvLed, HIGH);
    digitalWrite(Pump, HIGH);

    pumping = true;
    pumpStartedTime = millis();
  }

  // Stop the pump when the upper float triggers
  if (isTankFull && pumping) {
    digitalWrite(UvLed, LOW);
    digitalWrite(Pump, LOW);
    pumping = false;
  }

  // If IR LOS is broken, there is either ice actively falling or the bin is
  // full to rule out the first case, wait for the IR sensor to lose LOS for the
  // delay specified in binFullDelay
  if (IrReceiving == HIGH) {
    binCheck = 0;
  }

  if (IrReceiving == LOW) {
    if (binCheck == 0) {
      binCheck = millis();
    } else if (millis() - binCheck > binFullDelay) {
      binFull = true;
      binLastFullAt = millis();
      return;
    }
  }

  if (!isCompressorRunning) {
    // Don't abuse the compressor
    if (millis() - compressorStopTime < compressorTimeout) {
      return;
    }
    // Wait if we are in the middle of checking the bin
    if (binCheck != 0) {
      return;
    }
    digitalWrite(Compressor, HIGH);
    digitalWrite(Fan, HIGH);
    digitalWrite(Auger, HIGH);
    compressorStartTime = millis();
    isCompressorRunning = true;
  }

  // Stop the auger from blindly pushing harder when its jammed
  // Notably, the original machine has a light for defrost cycle, but I've never
  // seen it trigger, nor do I see a method of monitoring the motor on the
  // original motherboard
  if (currentDraw >= currentDrawLimit &&
      millis() - compressorStartTime > augerMotorGracePeriod) {
    defrostCycle = true;
    defrostCycleStartTime = millis();
    return;
  }
}
