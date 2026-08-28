// Copyright (C) 2026 SomeAspy (Aiden B. amb@aspy.dev)
// SPDX-License-Identifier: GPL-3.0-only
// https://github.com/someaspy/GE-Opal-2-fixes

#include "constants.h"
#include <EmonLib.h>
#include <Wire.h>
#include <avr/wdt.h>

namespace {
EnergyMonitor augerMeter;

// Front panel communication
unsigned long lastPanelCommunication = 0;
uint8_t lastButtonPress = button::idle;

// Water management
bool pumping = false;
unsigned long pumpStartedTime = 0;

// Compressor management
// If the machine is off, assume its been off for a safe amount of time
unsigned long compressorStopTime = -compressor_cooldown;
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
} // namespace

void setup() {
  wdt_enable(WDTO_2S);
  Serial.begin(115200);
  Wire.begin();

  // Grounded inputs need to be pulled up
  pinMode(pin::bin_switch, INPUT_PULLUP);
  pinMode(pin::tank_full, INPUT_PULLUP);
  pinMode(pin::tank_empty, INPUT_PULLUP);

  // Standard inputs
  pinMode(pin::ir_receiver, INPUT);
  pinMode(pin::auger_ammeter, INPUT);

  // Outputs
  pinMode(pin::ir_blaster, OUTPUT);
  pinMode(pin::compressor, OUTPUT);
  pinMode(pin::auger, OUTPUT);
  pinMode(pin::fan, OUTPUT);
  pinMode(pin::uv_led, OUTPUT);
  pinMode(pin::pump, OUTPUT);
  pinMode(pin::bin_led, OUTPUT);

  augerMeter.current(pin::auger_ammeter, ammeter_calibration_factor);
}

void loop() {
  wdt_reset();
  // DIGITAL READS ARE INVERTED BECAUSE WE PULL UP!!!
  // Read values for the current cycle
  const bool isTankFull = !digitalRead(pin::tank_full);
  const bool isTankEmpty = !digitalRead(pin::tank_empty);
  const bool isBinInserted = !digitalRead(pin::bin_switch);

  const double currentDraw = augerMeter.calcIrms(irm_sample_count);

  if (millis() - lastPanelCommunication > i2c_communication_delay) {
    lastPanelCommunication = millis();
    uint8_t currentLights = 0x0;
    if (isPowered) {
      currentLights |= led::power_button;
    }
    if (defrostCycle) {
      currentLights |= led::defrosting;
    }
    if (tankEmptyHalt) {
      currentLights |= led::add_water;
    }
    if (isLightOn) {
      currentLights |= led::light_button;
    }
    if (isCompressorRunning) {
      currentLights |= led::making_ice;
    }

    Wire.requestFrom(front_panel_i2c_address, static_cast<uint8_t>(1));
    if (Wire.available()) {
      auto buttonCode = static_cast<uint8_t>(Wire.read());
      if (buttonCode != lastButtonPress) {
        if (buttonCode != button::idle) {
          switch (buttonCode) {
          case button::power:
            isPowered = !isPowered;
            tankEmptyHalt = false;
            break;
          case button::light:
            isLightOn = !isLightOn;
            digitalWrite(pin::bin_led, isLightOn);
            break;
          default:
            Serial.print("missing case: ");
            Serial.println(buttonCode);
          }
        }
        lastButtonPress = buttonCode;
      }
      Wire.beginTransmission(front_panel_i2c_address);
      Wire.write(currentLights);
      Wire.endTransmission();
    }
  }

  bool irReceiving = false;

  // IR Receiver needs a moment to register it's state
  digitalWrite(pin::ir_blaster, true);
  delay(4);
  const int irVoltage = analogRead(pin::ir_receiver);
  if (irVoltage >= ir_receiver_voltage_threshold) {
    irReceiving = true;
  }
  digitalWrite(pin::ir_blaster, false);

  Serial.println(currentDraw);
  if (irReceiving) {
    Serial.println(millis());
    Serial.println("can see");
  }

  // When the machine should be stopped
  if (!isPowered || tankEmptyHalt || defrostCycle || binFull) {
    Serial.println("Halted");
    digitalWrite(pin::pump, false);
    digitalWrite(pin::compressor, false);
    digitalWrite(pin::fan, false);
    digitalWrite(pin::uv_led, false);
    digitalWrite(pin::auger, false);
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
    if (millis() - defrostCycleStartTime > defrost_cycle_length &&
        defrostCycle) {
      defrostCycle = false;
    }

    // If the bin being filled triggered the halt, clear once ice making
    // cooldown finishes
    if (millis() - binLastFullAt > bin_full_pause && binFull) {
      binFull = false;
    }
    return;
  }

  // If the pump doesn't move enough water into the tank, something is wrong
  if (pumping && (millis() - pumpStartedTime > pump_timeout)) {
    // We are probably out of water
    Serial.println("no water?");
    tankEmptyHalt = true;
    return;
  }

  // Don't stop pumping once the lower float rises, so we can fill it up to the
  // upper float to save pump spam
  if (isTankEmpty && !pumping) {
    digitalWrite(pin::uv_led, true);
    digitalWrite(pin::pump, true);

    pumping = true;
    pumpStartedTime = millis();
  } else if (isTankFull && pumping) {
    // Stop the pump when the upper float triggers
    digitalWrite(pin::uv_led, false);
    digitalWrite(pin::pump, false);
    pumping = false;
  }

  // If IR LOS is broken, there is either ice actively falling or the bin is
  // full to rule out the first case, wait for the IR sensor to lose LOS for the
  // delay specified in bin_full_delay
  if (irReceiving) {
    binCheck = 0;
  }

  if (!irReceiving) {
    if (binCheck == 0) {
      binCheck = millis();
    } else if (millis() - binCheck > bin_full_delay) {
      binFull = true;
      binLastFullAt = millis();
      return;
    }
  }

  if (!isCompressorRunning) {
    // Don't abuse the compressor
    if (millis() - compressorStopTime < compressor_cooldown) {
      return;
    }
    // Wait if we are in the middle of checking the bin
    if (binCheck != 0) {
      return;
    }
    digitalWrite(pin::compressor, true);
    digitalWrite(pin::fan, true);
    digitalWrite(pin::auger, true);
    compressorStartTime = millis();
    isCompressorRunning = true;
  }

  // Stop the auger from blindly pushing harder when its jammed
  // Notably, the original machine has a light for defrost cycle, but I've never
  // seen it trigger, nor do I see a method of monitoring the motor on the
  // original motherboard
  if (currentDraw >= auger_current_draw_limit &&
      millis() - compressorStartTime > auger_inrush_grace) {
    defrostCycle = true;
    defrostCycleStartTime = millis();
    return;
  }
}
