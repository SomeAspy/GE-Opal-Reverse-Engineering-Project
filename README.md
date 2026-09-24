# GE-Opal-Reverse-Engineering-Project

This repository documents, all my attempts to make the GE Opal 2 not kill itself
(More specifically, the *GE Profile:tm: Opal:tm: 2.0 Ultra Nugget Ice Maker )

Important context to a lot of frustration here: This is a $500 dollar ice machine. It has a lifespan of 6 months to 1 year

## Consider sponsoring

I have quite literally dumped hundreds of dollars into this project.
Some of the costs are prototyping modules, connectors, electronics components, and OEM parts for the machine.

**Money spent on this project so far: $755.98**
_this does not factor in my time spent on the project_

## Directory

- [`reverse-engineering-efforts/`](./reverse-engineering-efforts) - Generally contains specifics about the hardware of the Opal 2
  - [`i2c.md`](./reverse-engineering-efforts/i2c.md) - Details of the I2C implementation in the original machine
  - [`pipes.d2`](./reverse-engineering-efforts/i2c.md) - D2 file for generating `pipes.svg`
  - [`pipes.svg`](./reverse-engineering-efforts/pipes.svg) - How the Opal 2 is plumbed
- [`physical-repairs/`](./physical-repairs) - Details of specific repair efforts
  - [`gearboxRepair.md`](physical-repairs/gearboxRepair.md) - Details about the gearbox and the procedures and parts to fix it
  - [`squeakFromShaft.md`](physical-repairs/squeakFromShaft.md) - Details about a repair attempting to stop the squeaking shaft
- [`hardware/`](./hardware) - PCB related stuff
  - [`RoughSketch-ArduinoNanoESP32`](./hardware/RoughSketch-ArduinoNanoESP32) - Details the rough prototype build with modular components before the PCB fabrication
    - [`prototype-parts.md`](./hardware/RoughSketch-ArduinoNanoESP32/prototype-parts.md) - A list of the exact modules I used for prototyping
    - [`Schematic.pdf`](./hardware/RoughSketch-ArduinoNanoESP32/Schematic.pdf) - A human readable version of the schematic, made by printing to PDF from KiCAD
- [`attribution.md`](./attribution.md) - Links and references used for files
- [`todo.md`](./todo.md) - List of further reseach needed

## To Do

(in no specific order)

- [ ] Finish connectors.md with colors and diagrams
- [ ] Design PCB
- [ ] Reverse engineer I2C bus of other front panel [(Need parts)](./todo.md)
- [ ] Design full independent schematic
- [ ] Reverse engineer GE implementation of the RGB light [(Need parts)](./todo.md)

## Default button mappings

A few new button actions are defined, and a few changed from the original machine behavior

- **Power**: Toggles Power
  - **When held**: Triggers a hard reset of the Arduino. _I hope you know what you're doing_
- **Light**: Toggles Light
- **Clean**: Toggles cleaning mode. In my implentation, there is no timer or anything. It simply forces the pump to run until it is pressed again.

## The Prototype

![Mess of wires and stuff coming from the ice machine](./images/ThePrototype.jpg)

> [!WARNING]  
> Schematics and code are not final!

### LLM Policy and usage in this project:

I hate LLMs, and "AI" crap. Therefore, I decided to very clearly define how they were used in this project.

- I did **NOT** copy-paste code or otherwise use LLM generated material in this project's code.
- An LLM was used to help me understand how to use KiCAD because I have never used it before.
- An LLM was used to sanity check ([rubber ducky](https://en.wikipedia.org/wiki/Rubber_duck_debugging)) to avoid killing the machine.
- An LLM was used to help me understand the basics of I2C and reverse engineer the communication the Opal uses.
  - **I** implemented the I2C communications in the firmware.

Do **NOT** submit LLM generated issues, PRs, or use this repo in training data.

You **MAY** submit an **issue** that you made with the assistance of an LLM, but you must **paraphrase and understand** the content in your own words.

### My blog post on this

[A Scathing Review of the GE Opal 2 Nugget Ice Maker](https://blog.aspy.dev/a-scathing-review-of-the-ge-opal-2-nugget-ice-maker/)

# Background

I've tried putting lubrication on the auger ([`squeakFromShaft.md`](./physical-repairs/squeakFromShaft.md)) but it did not resolve the squeaking issue
I've decided I am going to replace the PCB with an arduino to implement a proper defrost cycle.

## Notice

As I was writing this, Reddit decided to delete the 2 year old post I was using as a reference.
Some of the more important images are available in [`images`](./images).

<https://web.archive.org/web/20250804095737/https://www.reddit.com/r/IceChewersAnonymous/comments/1hxlbkg/fixing_the_opal_20/>

## The Problem

The GE Opal ice maker has well-known issues where it will start to fail after about a year of use, and in some cases, prior.
The machine is incredibly high maintenance, [with GE recommending running bleach through the machine](https://products.geappliances.com/appliance/gea-support-search-content?contentId=000060634) ([archive.org capture](https://web.archive.org/web/20260716162758/https://products.geappliances.com/appliance/gea-support-search-content?contentId=000060634))

There is even (as of writing) [an ongoing class action](https://classlawdc.com/2026/01/21/ge-profile-opalnugget-ice-maker-series-1-0-and-2-0-defective-product-investigation/)

These machines do not fail gracefully, often making screeching and whining noises due to the internal parts getting frozen and locked up. This commonly involves the destruction of the gearbox, auger, and/or bearings.

Browsing Reddit reveals many owners have constant issues with these machines, with one redditor even claiming to go through 4 machines in 2 years. Despite its flaws, this machine is well liked by r/IceChewersAnonymous, though they also agree that it has significant longevity issues. [A quick search shows the love/hate the community has for it.](https://www.reddit.com/r/IceChewersAnonymous/search/?q=opal+2)

More importantly, several members of the community have taken attempts to repair the machine pretty far.

- https://www.reddit.com/r/IceChewersAnonymous/comments/1ophadu/opal_20_repair_update/
- https://www.reddit.com/r/IceChewersAnonymous/comments/1hxlbkg/fixing_the_opal_20/
- https://www.reddit.com/r/IceChewersAnonymous/comments/159ts6e/ge_opal_20_add_water_fix/

# Piping

![Piping](./reverse-engineering-efforts/pipes.svg)

# Nominal voltages recorded from the opal 2 ice maker during operation

_I am aware that JST XHB is not a real standard. However, this is the bastardized cloned modification China has made. it is a JST XH connector with a locking tab. You will not find them on Amazon, or Digikey. I found mine [on Aliexpress](https://www.aliexpress.us/item/3256812673164043.html?spm=a2g0o.order_detail.order_detail_item.3.7c9d126ahviqlg&gatewayAdapt=glo2usa)._

_Same applies to JST HY. With some help from the Arduino Community and JLCPCB, I was able to find what appears to be a matching connector.
[LCSC #C42451574](https://www.lcsc.com/product-detail/C42451574.html)_

| PCB Label     | Part                    | Connector Type & Color           | Voltage      |
| ------------- | ----------------------- | -------------------------------- | ------------ |
| UV            | UV Light                | JST XHB 2-pin connector (Blue)   | 12V DC       |
| compressor    | Compressor              | JST VHR 3-pin connector (Red)    | 120V AC      |
| motor         | Auger Motor             | JST VHR 3-pin connector (White)  | 120V AC      |
| WP            | Pump                    | JST XHB 2-pin connector (Purple) | 12V DC       |
| FAN1          | Fan                     | JST XHB 2-pin connector (Gray)   | 12V DC       |
| WIFI          | WiFi Board              | JST HY 5-pin connector (White)   | 3.3V + 5V DC |
| IM1101        | Front Panel             | JST XHB 4-pin connector (Black)  | 5V + I2C     |
| LED11 & LED12 | Ice Box LED(s?)         | JST XHB 2-pin connector (White)  | 12V DC       |
| SW            | Ice box presence switch | JST XHB 2-pin connector (Red)    | 5V DC        |
| CON5          | Internal Tank Floats    | JST HY 4-pin connector (Red)     | 5V DC        |
| TX            | IR LED For Capacity     | JST XHB 2-pin connector (Yellow) | 5V DC        |
| RX            | IR Receiver             | JST XHB 2-pin connector (Green)  | 5V DC        |
| AC            | AC Input                | JST VHR 3-pin connector (Black)  | 120V AC      |
| IMN1001       | ???                     | JST HY 7-pin connector (White)   | ???          |
| CON6          | ???                     | JST HY 6-pin connector (White)   | 5V + ???     |
| Clean         | ???                     | JST XHB 2-pin connector (Cyan)   | ???          |
| RGB           | ???                     | JST XHB 3-pin connector (White)  | 5V + ???     |
| 1033          | ???                     | JST XHB 3-pin connector (Black)  | 5V + ???     |
| CON3          | ???                     | JST XHB 3-pin connector (Red)    | ???          |
| ???           | ???                     | JST HY 5-pin connector (Black)   | 5V + ???     |

- I2C protocols documented in [`i2c.md`](./reverse-engineering-efforts/i2c.md)

- Internal Tank Floats (4 wires, 2 floats)
  - Black & Red: Lower float (float low = closed circuit)
  - Yellow & White: Upper float (float high = closed circuit)

## FAQ

### Why an ESP32 based board instead of a standard 8-bit Atmel board?

The idea was to keep it simple, but I started running into issues once I got the HLW8032.
Unlike the ACS712 which uses analog to communicate with the Arduino, the HLW8032 uses serial.
The original board I was planning to use and prototyping with, the Arduino Nano, only has 1 serial bus. That being the main USB port. This would mean you cannot program the controller while the HLW8032 is connected. I did not like that.
Furthermore, all the Arduino libraries for reading from the HLW8032 assume 32-bit math, which breaks down on the Arduino Nano.

### What about WiFi? The app?

I decided to completely ignore this, as a WiFi connected ice machine is just stupid.
Maybe I will add it some day, but there are 2 ways to go about it. The OEM WiFi daughterboard communicates to the motherboard via serial. You can either use that and just pretend to be the OEM motherboard, or you could completely rebuild/reverse-engineer the connection to the SmartHQ app, which... yikes...

### Can I buy a board?

Not yet. Though random Chinese shell corporations keep reaching out to me offering assistance, which I very politely decline.
