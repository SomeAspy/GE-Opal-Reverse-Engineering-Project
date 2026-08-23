# The front panel

The front panel appears to be communicating with the motherboard over I2C

Front panel address:`0x60`

Messages from Front panel:
Button Released: `0x00`
Clean pressed: `0x01`
Light pressed: `0x02`
Power pressed: `0x03`
Power held: `0x04`
Light held: `0x05`
Clean held: `0x06`

I popped the microcontroller on my original front panel that also had `Descale` and `Sanitize` LEDs. The replacement board I got does not have these LEDs, so I am unable to reverse engineer the codes as of now.
Commands to Front panel:
`0x00`: All Off
`0x01`: Clean
`0x02`: Light
`0x04`: Power
`0x08`: Add Water
`0x10`: Making Ice
`0x20`: Cleaning
`0x40`: Defrosting
`0x80`: WiFi Symbol
