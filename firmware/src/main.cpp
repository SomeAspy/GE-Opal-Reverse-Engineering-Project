// Copyright (C) 2026 SomeAspy (Aiden B. amb@aspy.dev)
// SPDX-License-Identifier: GPL-3.0-only
// https://github.com/someaspy/GE-Opal-2-fixes

#include "HardwareSerial.h"
#include <Arduino.h>
#include <EmonLib.h>
#include <Wire.h>

// Pins

const int BinSwitchPin = 3;
const int TankFullPin = 5;
const int TankEmptyPin = 6;
const int IrBlasterPin = 7;
const int CompressorPin = 8;
const int AugerPin = 9;
const int FanPin = 10;
const int UvLedPin = 11;
const int PumpPin = 12;
const int MotorAmmeterPin = A0;
// Using analog because the voltage isn't enough to trigger digital high (~1.6v)
const int IrReceiverPin = A1;

// Front Panel
const byte FrontPanel = 0x60;

// Bitmasks sent to Front panel
const byte CleanLed = 0x01;
const byte LightLed = 0x02;
const byte PowerLed = 0x04;
const byte AddWaterLed = 0x08;
const byte MakingIceLed = 0x10;
const byte CleaningLed = 0x20;
const byte DefrostingLed = 0x40;
const byte WifiLed = 0x80;

// Bitmasks from the front panl
const byte ButtonReleased = 0x00;
const byte CleanButton = 0x01;
const byte LightButton = 0x02;
const byte PowerButton = 0x03;
const byte PowerButtonHold = 0x04;
const byte LightButtonHold = 0x05;
const byte CleanButtonHold = 0x06;

// Timing & variables

// Calibration factor by gemini, because I hate complex math
const double AugerAmmeterCalibrationFactor = 5.76;
// IRM sample count by Gemini, because I can't be bothered.
const int AugerAmmeterSampleCount = 1480;

const unsigned long pumpTimeout = 120000; // 2m - Filter can drop flow a lot
const unsigned long compressorTimeout = 300000; // 5m

// From testing the machine usually settles around 0.45A to 0.48A.
// 0.05 means the compressor is off. This probably means we have a calibration
// error.
// Note currentDrawLimit is ignored when the compressor first starts to
// accomodate inrush current. Tweak as needed.
const float currentDrawLimit = 0.50;
const unsigned long augerMotorGracePeriod = 15000; // 15s for inrush to settle
const unsigned long defrostCycleLength = 600000;   // 10m

const unsigned long binFullDelay = 10000;
const unsigned long binFullCooldown = 3600000;
// Don't DDoS the front panel microprocessor
const unsigned long panelCommunicationDelay = 50;

EnergyMonitor augerMeter;

void setup() {
  Serial.begin(115200);
  Wire.begin();

  // Grounded inputs need to be pulled up
  pinMode(BinSwitchPin, INPUT_PULLUP);
  pinMode(TankFullPin, INPUT_PULLUP);
  pinMode(TankEmptyPin, INPUT_PULLUP);

  // Standard inputs
  pinMode(IrReceiverPin, INPUT);
  pinMode(MotorAmmeterPin, INPUT);

  // Outputs
  pinMode(IrBlasterPin, OUTPUT);
  pinMode(CompressorPin, OUTPUT);
  pinMode(AugerPin, OUTPUT);
  pinMode(FanPin, OUTPUT);
  pinMode(UvLedPin, OUTPUT);
  pinMode(PumpPin, OUTPUT);

  augerMeter.current(MotorAmmeterPin, AugerAmmeterCalibrationFactor);
};

// Front panel communication
unsigned long lastPanelCommunication = 0;
byte lastButtonPress = 0x00;
unsigned long errorBlink = 0;

// Water management
bool pumping = false;
unsigned long pumpStartedTime = 0;

// Compressor management
// If the machine is off, assume its been off for a safe amount of time
unsigned long compressorStopTime = 300001;
unsigned long compressorStartTime = 0;
bool isCompressorRunning = false;

// Auger & defrost cycle management
bool defrostCycle = false;
unsigned long defrostCycleStartTime = 0;

// Bin management
unsigned long binCheck = 0;
unsigned long binLastFullAt = 0;
bool binFull = false;

// General state initialization
bool tankEmptyHalt = false;
bool isPowered = false;
bool isLightOn = false;

void loop() {
  // DIGITAL READS ARE INVERTED BECAUSE WE PULL UP!!!
  // Read values for the current cycle
  const bool isTankFull = digitalRead(TankFullPin) == LOW;
  const bool isTankEmpty = digitalRead(TankEmptyPin) == LOW;
  const bool isBinInserted = digitalRead(BinSwitchPin) == LOW;

  const double currentDraw = augerMeter.calcIrms(AugerAmmeterSampleCount);

  if (millis() - lastPanelCommunication > panelCommunicationDelay) {
    lastPanelCommunication = millis();
    byte currentLights = 0x00;
    if (isPowered) {
      currentLights |= PowerLed;
    }
    if (defrostCycle) {
      currentLights |= DefrostingLed;
    }
    if (tankEmptyHalt) {
      currentLights |= AddWaterLed;
    }
    if (isLightOn) {
      currentLights |= LightLed;
    }
    if (isCompressorRunning) {
      currentLights |= MakingIceLed;
    }

    Wire.requestFrom(FrontPanel, (byte)1);
    if (Wire.available()) {
      byte buttonCode = Wire.read();
      if (buttonCode != lastButtonPress) {
        if (buttonCode != ButtonReleased) {
          switch (buttonCode) {
          case PowerButton:
            isPowered = !isPowered;
            break;
          case LightButton:
            isLightOn = !isLightOn;
            break;
          }
        }
        lastButtonPress = buttonCode;
      }
      Wire.beginTransmission(FrontPanel);
      Wire.write(currentLights);
      Wire.endTransmission();
    }
  }

  bool IrReceiving = false;

  // IR Receiver needs a moment to register it's state
  digitalWrite(IrBlasterPin, HIGH);
  delay(4);
  const int IrVoltage = analogRead(IrReceiverPin);
  if (IrVoltage >= 200) { // Around 1v
    IrReceiving = true;
  }
  digitalWrite(IrBlasterPin, LOW);

  Serial.println(currentDraw);
  if (IrReceiving == true) {
    Serial.println(millis());
    Serial.println("can see");
  }

  // When the machine should be stopped
  if (!isPowered || tankEmptyHalt || defrostCycle || binFull) {
    Serial.println("Halted");
    digitalWrite(PumpPin, LOW);
    digitalWrite(CompressorPin, LOW);
    digitalWrite(FanPin, LOW);
    digitalWrite(UvLedPin, LOW);
    digitalWrite(AugerPin, LOW);
    pumping = false;

    // Start compressor cooldown
    if (isCompressorRunning) {
      compressorStopTime = millis();
      isCompressorRunning = false;
    }

    // If the bin is open we can assume ice was removed
    if (!isBinInserted) {
      tankEmptyHalt = false;
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
    tankEmptyHalt = true;
    return;
  }

  // Don't stop pumping once the lower float rises, so we can fill it up to the
  // upper float to save pump spam
  if (isTankEmpty && !pumping) {
    digitalWrite(UvLedPin, HIGH);
    digitalWrite(PumpPin, HIGH);

    pumping = true;
    pumpStartedTime = millis();
  }

  // Stop the pump when the upper float triggers
  if (isTankFull && pumping) {
    digitalWrite(UvLedPin, LOW);
    digitalWrite(PumpPin, LOW);
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
    digitalWrite(CompressorPin, HIGH);
    digitalWrite(FanPin, HIGH);
    digitalWrite(AugerPin, HIGH);
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
