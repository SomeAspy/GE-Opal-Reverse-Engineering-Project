// Copyright (C) 2026 SomeAspy (Aiden B. amb@aspy.dev)
// SPDX-License-Identifier: GPL-3.0-only
// https://github.com/someaspy/GE-Opal-2-fixes

// Pins
#include <Arduino.h>

namespace pin {
constexpr uint8_t bin_switch = 3;
constexpr uint8_t bin_led = 4;
constexpr uint8_t tank_full = 5;
constexpr uint8_t tank_empty = 6;
constexpr uint8_t ir_blaster = 7;
constexpr uint8_t compressor = 8;
constexpr uint8_t auger = 9;
constexpr uint8_t fan = 10;
constexpr uint8_t uv_led = 11;
constexpr uint8_t pump = 12;
constexpr uint8_t auger_ammeter = A0;
// Using analog because the voltage isn't enough to trigger digital high (~1.6v)
constexpr uint8_t ir_receiver = A1;
} // namespace pin

// Front Panel
constexpr uint8_t front_panel_i2c_address = 0x60;

// Bitmasks sent to Front panel

namespace led {
constexpr uint8_t clean_button = 0x01;
constexpr uint8_t light_button = 0x02;
constexpr uint8_t power_button = 0x04;
constexpr uint8_t add_water = 0x08;
constexpr uint8_t making_ice = 0x10;
constexpr uint8_t cleaning = 0x20;
constexpr uint8_t defrosting = 0x40;
constexpr uint8_t wifi = 0x80;
} // namespace led

// Bitmasks from the front panl
// button_idle_bitmask is also fired when a button is released
namespace button {
constexpr uint8_t idle = 0x00;
constexpr uint8_t clean = 0x01;
constexpr uint8_t light = 0x02;
constexpr uint8_t power = 0x03;
constexpr uint8_t power_held = 0x04;
constexpr uint8_t light_held = 0x05;
constexpr uint8_t clean_held = 0x06;
} // namespace button

// Timing & variables

// Calibration factor by gemini, because I hate complex math
constexpr float ammeter_calibration_factor = 5.76;
// IRM sample count by Gemini, because I can't be bothered.
constexpr int irm_sample_count = 1480;

// 2m - Filter can drop flow a lot
constexpr unsigned long pump_timeout = 120000;
constexpr unsigned long compressor_cooldown = 300000; // 5m

// From testing the machine usually settles around 0.45A to 0.48A.
// 0.05 means the compressor is off. This probably means we have a calibration
// error.
// Note currentDrawLimit is ignored when the compressor first starts to
// accomodate inrush current. Tweak as needed.
constexpr float auger_current_draw_limit = 0.50;
constexpr unsigned long auger_inrush_grace = 15000; // 15s for inrush to settle
constexpr unsigned long defrost_cycle_length = 600000; // 10m

constexpr unsigned long bin_full_delay = 10000;
constexpr unsigned long bin_full_pause = 3600000;
// Don't DDoS the front panel microprocessor
constexpr unsigned long i2c_communication_delay = 50;

// 200 ~= 1v. Needed because digital high requires near 5v
constexpr int ir_receiver_voltage_threshold = 200;
