// Copyright (C) 2026 SomeAspy (Aiden B. amb@aspy.dev)
// SPDX-License-Identifier: GPL-3.0-only
// https://github.com/someaspy/GE-Opal-2-fixes

#include <Arduino.h>
// Pins

namespace pin {
const byte bin_switch = 3;
const byte tank_full = 5;
const byte tank_empty = 6;
const byte ir_blaster = 7;
const byte compressor = 8;
const byte auger = 9;
const byte fan = 10;
const byte uv_led = 11;
const byte pump = 12;
const byte auger_ammeter = A0;
// Using analog because the voltage isn't enough to trigger digital high (~1.6v)
const byte ir_receiver = A1;
} // namespace pin

// Front Panel
const byte front_panel_i2c_address = 0x60;

// Bitmasks sent to Front panel

namespace led {
const byte clean_button = 0x01;
const byte light_button = 0x02;
const byte power_button = 0x04;
const byte add_water = 0x08;
const byte making_ice = 0x10;
const byte cleaning = 0x20;
const byte defrosting = 0x40;
const byte wifi = 0x80;
} // namespace led

// Bitmasks from the front panl
// button_idle_bitmask is also fired when a button is released
namespace button {
const byte idle = 0x00;
const byte clean = 0x01;
const byte light = 0x02;
const byte power = 0x03;
const byte power_held = 0x04;
const byte light_held = 0x05;
const byte clean_held = 0x06;
} // namespace button

// Timing & variables

// Calibration factor by gemini, because I hate complex math
const double ammeter_calibration_factor = 5.76;
// IRM sample count by Gemini, because I can't be bothered.
const int irm_sample_count = 1480;

const unsigned long pump_timeout = 120000; // 2m - Filter can drop flow a lot
const unsigned long compressor_cooldown = 300000; // 5m

// From testing the machine usually settles around 0.45A to 0.48A.
// 0.05 means the compressor is off. This probably means we have a calibration
// error.
// Note currentDrawLimit is ignored when the compressor first starts to
// accomodate inrush current. Tweak as needed.
const float auger_current_draw_limit = 0.50;
const unsigned long auger_inrush_grace = 15000;    // 15s for inrush to settle
const unsigned long defrost_cycle_length = 600000; // 10m

const unsigned long bin_full_delay = 10000;
const unsigned long bin_full_pause = 3600000;
// Don't DDoS the front panel microprocessor
const unsigned long i2c_communication_delay = 50;

// 200 ~= 1v. Needed because digital high requires near 5v
const int ir_receiver_voltage_threshold = 200;
