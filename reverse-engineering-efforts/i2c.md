# The front panel

The front panel appears to be communicating with the motherboard over I2C

## Device Addresses

| Address |   Device    |
| :-----: | :---------: |
| `0x60`  | Front Panel |

## Button Codes (Received from `0x60`)

|  Hex   |        Action         |
| :----: | :-------------------: |
| `0x00` |    Button Released    |
| `0x01` |     Clean Pressed     |
| `0x02` |     Light Pressed     |
| `0x03` |     Power Pressed     |
| `0x04` |      Power Held       |
| `0x05` |      Light Held       |
| `0x06` |      Clean Held       |
| `0x07` | Power + Clean Pressed |
| `0x08` | Clean + Light Pressed |
| `0x09` | Light + Power Pressed |

Note that holding buttons fires both the initial button pressed code and then the Held code after 3 seconds of holding

For example, pressing and holding power:

1. Power button detects interaction
2. `0x03` fired
3. Power button is held for 3 seconds
4. `0x04` fired
5. Power button released
6. `0x00` fired

I popped the microcontroller on my original front panel that also had `Descale` and `Sanitize` LEDs. The replacement board I got does not have these LEDs, so I am unable to reverse engineer the codes as of now.

## Light Codes (Sent to `0x60`)

|  Hex   |   Action    |
| :----: | :---------: |
| `0x00` |   All Off   |
| `0x01` |    Clean    |
| `0x02` |    Light    |
| `0x04` |    Power    |
| `0x08` |  Add Water  |
| `0x10` | Making Ice  |
| `0x20` |  Cleaning   |
| `0x40` | Defrosting  |
| `0x80` | WiFi Symbol |

Light codes are sent as bitmask. If you want to turn on multiple LEDs then you add the bits.
For example:

`0x01 | 0x80` = Clean + WiFi Symbol

`0x02 | 0x04 | 0x20` = Light + Power + Cleaning
