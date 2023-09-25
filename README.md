# lelit-bianca-protocol
Project notes for trying to sniff and reverse engineer the protocols used for LCC information on the Lelit Bianca V2

## Project status (per 2021-06-30)
The protocol has now been fully reverse engineered (documented below). There is still a matter of calculating more exact ADC-value-to-temperature equations remaining, but I'm now focused more on creating a replacement LCC (see [magnusnordlander/smart-lcc](https://github.com/magnusnordlander/smart-lcc)).

It doesn't cover the presumably updated protocol for the Bianca V3, and since I don't have access to a Bianca V3 (for reverse engineering or otherwise) there's currently no plan for when that could be reverse engineered.

## Architecture
The Lelit Bianca has a split controller architecture. One part, the control board, does power management, controls relays, and ADC, and the other part, the LCC, drives the display and buttons, and acts as a PID controller. These two parts communicate via a bus, which is the main focus of this project.

The bus is 6 wires with the following purpose:

* Pinout (red wire is pin 6):
  * Pin 1: 12V for OLED (Measured 12.6V)
  * Pin 2: TX (to Control board, 3V3)
  * Pin 3: RX (from Control board, 5V)
  * Pin 4: GND
  * Pin 5: 3V3 for OLED (Measured 3.1V)
  * Pin 6: 3V3 for MCU(Measured 3.1V)

The TX and RX pins is a UART running at 9600 bps 8N1 with inverted signalling. The LCC acts as the bus master. It sends a message to the control board, and the control board replies.

## LCC to Control Board messages

```
80:11:17:00:28
80 ii jj bb zz

ii = shift register 2
jj = shift register 1
bb = front buttons, 0x08 if the minus button is pressed, 0x04 if the plus button is pressed, and 0x0B if both.
zz = checksum, CheckSum8 Modulo 128
```

### The shift registers

In the control board there are two shift registers, which are unlabled on the PCB, but which I call Shift Register 1 and Shift Register 2. They are both STPIC6C595's, and SER_OUT of SR1 is connected to SER_IN of SR2. SR1 gets its data from the control board MCU (a STM8S003F3P6).

Here's bitmask values for what the different bits in the shift registers do:

```
typedef enum : uint8_t {
    LCC_SHIFT_REGISTER_1_CN6_1 = 1 << 0,
    LCC_SHIFT_REGISTER_1_CN6_3 = 1 << 1,
    LCC_SHIFT_REGISTER_1_CN6_5 = 1 << 2,
    LCC_SHIFT_REGISTER_1_CM9_BREW_BOILER_RELAY = 1 << 3,
    LCC_SHIFT_REGISTER_1_FA7_SOLENOID = 1 << 4,
    LCC_SHIFT_REGISTER_1_FA8_SOLENOID_V3 = 1 << 5, // FA8 is unconnected on Bianca V2
    LCC_SHIFT_REGISTER_1_CN10_DISABLE_OLED_12V = 1 << 6,
    LCC_SHIFT_REGISTER_1_CN10_DISABLE_OLED_3V3 = 1 << 7,
} LccShiftRegister1Flags;

typedef enum : uint8_t {
    LCC_SHIFT_REGISTER_2_CN5_SERVICE_BOILER_RELAY = 1 << 0,
    LCC_SHIFT_REGISTER_2_FA10_PUMP_RELAY = 1 << 4, // Also connected to the larger solenoid on V2
    LCC_SHIFT_REGISTER_2_FA9 = 1 << 5,
} LccShiftRegister2Flags;
```

From what I can tell, drains 1-3 and drains 6-7 on SR2 are unconnected.

## Control board to LCC messages

```
81:00:00:5D:7F:00:79:7F:02:5D:7F:03:2B:00:02:05:7F:67
81 lu cc cc cc ss ss ss CC CC CC SS SS SS tt tt tt zz

c = coffee boiler temperature, low gain
C = coffee boiler temperature, high gain
s = service boiler temperature, low gain
S = service boiler temperature, high gain
t = service boiler water level, normal value is around 128, once the level drops it goes to around 600 pretty quick.
z = checksum, CheckSum8 Modulo 128
l = water tank level, 0 = not empty, 4 = empty
u = brew microswitch, 0 = open, 2 = closed
```

### Weird number conversion

The temperatures and the service boiler water levels are analog signals, with a weird encoding. The high/low gain thing are a speculation on my part, but it seems likely. The code for converting a 3 byte number to an uint16 is as follows:

```c
if (weirdNum[2] == 0x7F) {
    return (weirdNum[1] | 0x80) + (weirdNum[0] << 8);
} else if (weirdNum[2] == 0x00) {
    return weirdNum[1] + (weirdNum[0] << 8);
}
```

The water level is presumably not *really* meaningful as an analog value. It just requires an ADC to read. It's really just two states.

### ADC value to temperatures

ADC values can be somewhat reliably converted to the same temperature as the LCC shows (and back) by third degree polynomials. More data is needed for more exact conversions.

Current best-guess constants are given below for the following form of polynomial:

`y = ax^3 + bx^2 + cx +d`

#### Low gain ADC value to temperatures in degrees Celsius

```
a = -1.99514E-07
b = 7.66659E-05
c = 0.546325171
d = -17.22637553
```

#### High gain ADC value to temperatures in degrees Celsius

```
a = 2.80075E-07
b = -0.000371374
c = 0.272450858
d = -4.737333399
```

#### Degrees Celsius to low gain ADC value

```
a = 1.94759E-06
b = -0.000294428
c = 1.812604664
d = 31.49048711
```

#### Degrees Celsius to high gain ADC value

```
a = -0.000468472
b = 0.097074921
c = 1.6935213
d = 27.8765092
```

# Old information

## Important project note 2021-06-04
So, I just discovered that the Bianca uses inverted UART signalling. Most protocol weirdness seems to be related to that. Quite a bit of information created before (including protocol dumps) 2021-06-04 is somewhat wrong because of this. I will be purging incorrect information at some point, but at the moment, YMMV

## Goals
* Stage 1: Expose interesting information as an IoT appliance
* ~~Stage 2: Allow for IoT control of set points, sleep mode etc~~ (Not possible without replacing LCC)

## Answered questions

* Is the Red port on the LCC a debug port?
  * Not exactly. It's an on-board programming port.
* Is the LCC communicating with the Control board using a serial protocol?
  * Yes, and it contains interesting information.
* What's with the weird encodings?
  * I don't know why they do it this way, but that's fine.
* What's in the other bits on the CILO protocol?
  * Button presses


## Random thoughts on the protocol

Dumping the data I've identified 6 analog signals that makes sense to find out what they do. Byte references are

* COLI
  * Each packet ends with 0x00 (byte 17). Including the terminating NUL, each packet is 18 bytes long.
  * Most values are some weird 3 byte thing (which I call a triplet), with a weird encoding. See transformHextriplet in sniffer.ino. The maximum length in the current implementation is 12 bits, so that could well be the actual max length.
  * Microswitch: Bitmask 0x40 of byte 1. On if 0.
  * Brew boiler temp: Triplet of bytes 7-9. Lower value means higher temperature. See temp-calibration.txt for some calibration values (in celsius).
  * Service boiler temp: Triplet of bytes 10-12. Lower value means higher temperature. See temp-calibration.txt for some calibration values (in celsius).
  * Water level in the service boiler (not 100% sure, but somewhat): Triplet of bytes 13-15
    * This would heavily imply that the LCC is responsible for runnning the pump to fill the service boiler.
  * There are two other possible triplets, on 1-3 and 4-6. You *do* get values on those, but they're eerily similar to brew/service boiler temp.
  * Considering the manual (pg. 29, Alarms showed on display), the LCC should also have an opinion on water tank level (My machine is plumbed, but at some point I'm gonna plug in a tank and see what's what)
* CILO
  * Each packet ends with a terminating NUL (byte 4), and is 5 bytes long, NUL included.
  * Brew boiler heating element: Bitmask 0x0E of byte 0. On if value equals 0x0A.
  * Service boiler heating element: Bitmask 0xF0 of byte 1. On if value equals 0xE0.
  * Pump: Bitmask 0x0E of byte 1. On if value equals 0x0E.
  * There are additional flags here that vary, that will need to be figured out for stage 2.

## Background (Written at the start of the project)

The Lelit Bianca displays interesting information on the display of the LCC. I want to be able to integrate my Lelit Bianca into my Smart Home. Considering that the Lelit Bianca is able to add functionality by upgrading just the LCC module (which carries a versioned firmware), it stands to reason that it is more than just a mere display and buttons for the Control board. The two are connected via a 6 wire connection. The LCC is able to display the following definitely external information:

* Brew boiler temperature
* Service boiler temperature
* Pump status

It's also either able to set or itself controls the following:

* Brew boiler set point
* Service boiler set point
* Pre-infusion parameters

Carring this amount of information in a parallel bus together with a bus to drive an OLED display would require far more wires than 6. It therefore stands to reason that the LCC must be communicating with the Control Board via a serial bus. That serial bus must at the very least *contain* interesting status information (temperatures e.g.).

Furthermore, there is an unpopulated port on the LCC. This could possibly be a debug port. Depending on what gets stored where, a debug port might be necessary for stage 2.

## Project log

### 2021-06-26
I finally figured out what the last byte in the LCC packet was. It's the front buttons.

This is the full description of an LCC packet:

```
80:11:17:00:28
80 ps ec bb zz

p = pump relay, bitmask 0x1
s = service boiler ssr, bitmask 0x1
e = electrovalve relay, bitmask 0x1
c = coffee boiler ssr, bitmask 0x8
bb = front buttons, 0x08 if the minus button is pressed, 0x04 if the plus button is pressed, and 0x0B if both.
zz = checksum, CheckSum8 Modulo 128
```

Also, in reading [documentation for an OLED display module](https://www.displaymodule.com/products/1-54-128x64-monochrome-graphic-oled-display-module-spi?variant=31919976153186) I'm considering for the replacement unit, I think I've figured out the purpose of the two separate 3V3 lines on the bus. OLED displays prefer to be powered with specific timings. One of the 3V3 lines power the MCU (pin 6), the other the display (pin 5). I'm guessing they are appropriately timed.

### 2021-06-13
```
81:00:00:5D:7F:00:79:7F:02:5D:7F:03:2B:00:02:05:7F:67
81 lu cc cc cc ss ss ss CC CC CC SS SS SS tt tt tt zz

c = coffee boiler temperature, low gain
C = coffee boiler temperature, high gain
s = service boiler temperature, low gain
S = service boiler temperature, high gain
t = service boiler water level, normal value is around 128, once the level drops it goes to around 600 pretty quick.
z = checksum, CheckSum8 Modulo 128
l = water tank level, 0 = not empty, 4 = empty
u = brew microswitch, 0 = open, 2 = closed
```

The temperatures and the service boiler water levels are analog signals, with a weird encoding. The high/low gain thing are a speculation on my part, but it seems likely. The code for converting a 3 byte number to an uint16 is as follows:

```c
if (weirdNum[2] == 0x7F) {
    return (weirdNum[1] | 0x80) + (weirdNum[0] << 8);
} else if (weirdNum[2] == 0x00) {
    return weirdNum[1] + (weirdNum[0] << 8);
}
```

The water level is presumably not *really* meaningful as an analog value. It just requires an ADC to read. It's really just two states.

For the LCC:

```
80:11:17:00:28
80 ps ec xx zz

p = pump relay, bitmask 0x1
s = service boiler ssr, bitmask 0x1
e = electrovalve relay, bitmask 0x1
c = coffee boiler ssr, bitmask 0x8
zz = checksum, CheckSum8 Modulo 128
```

It is a bit unclear what bytes 3 bitmask 0x07 and byte 4 actually do. I have used a logic analyzer to sniff shift registers on the LCC, and it seems to me that this might be vestigial in the Bianca. Needs further research though.

### 2021-06-07
The control board checksum seems to be CheckSum8 Modulo 128, on everything except the first nibble. I'm not *entirely* sure, but the same could be the case for the LCC's last byte.

### 2021-06-06
I've been able to make quite a bit of new progress after the setback in finding out that all my captured data was wrong. Some facts:

* The LCC acts as the bus master. It sends out packets, and the control board responds.
* LCC packets are 5 bytes long and (probably) always begins with 0x80.
* Control board packets are 18 bytes long, and (probably) always begins with 0x81.
* A safe default packet (which the LCC uses when in advanced settings mode) is `80 00 00 00 00`.
* When the LCC has encountered an error, it sends `80 00 07 00 07`.

I've determined that the temperature probes of the control board measure resistance. 47kOhm is about 27 degrees centigrade, and using a 4.7kOhm rheostat I'm ably to vary the temperature around 80-150 degrees centigrade. I've been able to analyze what varies in the control board data when mucking around with the inputs to the control board and have drawn the following conclusions:

```
81:00:00:5D:7F:00:79:7F:02:5D:7F:03:2B:00:02:05:7F:67:
xx lu ?c cc cc ?s ss ss ?c cc cc ?s ss ss xx ss xx zz?

c = coffee boiler temperature
s = service boiler temperature
x = unknown
z = checksum?
l = livello serb, 0 = open, 4 = closed
u = microswitch erogazione, 0 = open, 2 = closed
```

Next step is to make a data logger and log lots of real data.

### 2021-06-04
Some significant discoveries.

Having a bench setup I hooked up my logic analyzer to the buses, and it turns out the reason the signals were so weird is because the UARTs on both the LCC and Control Board uses inverted signalling. Interpreting the signals as inverted preliminarily makes the protocol seem less weird (though it's definitely inverted regardless of it's weirdness). Sadly this invalidates some of my previous work.

I was also able to build a bus proxy, so that's step one towards being able to build a replacement LCC. Currently it used a Raspberry Pi Pico, since that was the only board I had at home that supports inverted UARTs. I'm not sure if I'm gonna end up using a Pi Pico (or a RP2040) in the end, but it's not an unreasonable choice. It's cheap and fast. The idea would be to use a Pico as a headless PID, and expose an SPI bus for displaying information in the PID and controlling PID settings. That SPI bus would be accessed by something like an ESP32, which could then display the information on an OLED display, and change settings over the network etc. The only downside I see right now is that the RP2040 has only 2 UARTs, which makes debugging marginally more difficult.

Also, looking at the board for the control board, it's weird that both 3V3 pins are the same. I'm gonna have to look in to that.

Some outstanding tasks:

* Finish up the proxy program so that it exposes data through SPI and logs it to an SD card.
* Look at the protocol again. Some of my previous work will surely be useful, but not everything.
* Look in to the possibility of replacing actual thermocouples with potentiometers for debugging/reverse engineering purposes.
* Experiment with the inputs on the control board to be sure that the protocol is correct.
* Gather more data from my actual machine
* Start controlling the control board without the LCC and see that it does what I want.

### 2021-06-03
I've purchased a spare control board. The line-voltage and logic-level parts are mostly separate boards, and I've separated them to be able to rig up a bench setup (tested and works). The MCU is a STM8S003F3. Datasheet at https://www.st.com/resource/en/datasheet/stm8s003f3.pdf. It also uses two STPIC6C595 to control logic-level outputs, and it seems the thermocouples (I bought two of those too) are resistance based.

Another controller board data sheet: https://www.st.com/resource/en/reference_manual/cd00190271-stm8s-series-and-stm8af-series-8bit-microcontrollers-stmicroelectronics.pdf


### 2021-03-12

I discovered that the signalling level on the CILO bus is actually 3V3. This explains the transistor setup on the LCC! I'm not sure if the MCU on the LCC is 5V tolerant, but I would guess it isn't and that they're using the transistor to make sure that the COLI bus is shifted to 3V3. I'd further venture that they considered the maximum output (amperage-wise) of the digital pins on the MCU insufficient for the bus, so they are using that as a gate to one of the 3V3 pins on the bus. Weird but kind of clever.

I also tried porting the standalone logger to an Arduino Nano 33 IoT, but I'm having some trouble with the UARTs. I will be getting to the bottom of that though, since it's A) interesting, and B) could shed more light on the protocol.


### 2021-03-11

Some progress. I wrote a standalone sniffer that reads data off the bus, dumps it to an SD card, and shows the processed data on a display. Since the bus provides 12V, I'm able to drive the whole shebang off of it. 

I also discovered that my assumption about COLI packets always starting with 3F was wrong. They do not.

Some longer-term game plan elements:

* Try to use an Arduino Nano 33 IoT instead of a Arduino Mega 2560 (needs level shifting, but has integrated WiFi). This requires using SoftSerial instead of hardware serial.
* Using wifi, integrate the data into my smart home via MQTT.
* Try using the Arduino as a bus transceiver instead of a sniffer (this will require better cabling).
* Make a nicer enclosure.

The ultimate goal is to understand fully the protocol, and then create a replacement LCC board.

### 2021-03-10

Major progress today. I logged more raw sessions, did a lot of head scratching, and I now have an Arduino program able to read all the information available and interesting. There are still some open questions, like "What's with the weird encoding". Next step is to build a simple datalogger so I don't have to have the computer running to capture logs. I'm also gonna be attaching a display, for funsies.

### 2021-03-09

I got plenty of work done on this project today. I got my oscilloscope, and measured the RX/TX lines. They are *not* RS232, but rather a 5V UART running at 9600 baud. I hooked up an Arduino to it to grab protocol data.

I had to guess at stop bits and parity, and the current capture is with 8N1, though either that's completely wrong, or the protocol pads with a lot of unnecessary data. I am able to get *some* data that makes *some* sense though. The data is committed to this repository. The raw capture is `dump/log.txt`. Most of the work at making sense of it is being done in the Excel file.

Thus far I've mostly looked at the communications from the control board to the LCC (labeled RX in the dump, though I'll probaby change that to COLI, for Control board Out, LCC in, and CILO for Control board In, LCC Out).

Some conclusions I'm already prepared to draw about the architecture of the Bianca:

* The control board is basically just a bridge to the machines sensors and actuators. All the logic is in the LCC.
* There appears to be at least 4 distinct analog sensors (think thermocouples etc.) interfaced to the LCC.
* I've identified a binary flag that appears to be whether the handle has been pulled.
* The LCC appears to have control over the two heating elements (on/off).
  * The LCC thus is the PID controller.
  * The stock LCC doesn't run the two elements separately, though it seems possible from a protocol standpoint. I'm not gonna try it though.
* I would assume it also has control over the pump, though I need some more experimentation to determine how.
* It should definitely be possible to sniff data from the protocol.
* It's very likely possible to develop a smarter LCC replacement using something like an Arduino.


### 2021-03-08

I added a vampire board to the connection betweeh the LCC and the Control Board. I was able to measure the voltages.

* Black pinout (red wire is pin 6):
  * Pin 1: 12V (Measured 12.6V)
  * Pin 2: TX (to Control board, 3V3)
  * Pin 3: RX (from Control board, 5V)
  * Pin 4: GND
  * Pin 5: 3V3 (Measured 3.1V)
  * Pin 6: 3V3 (Measured 3.1V)

I'm kind of surprised that pin 6 is also 3V3. Tomorrow I'm getting an oscilloscope, but it does seem very likely that the RX/TX pins are actual RS232. Will confirm using the scope. If so, I do have a couple of MAX232s, so I'll be able to build a simple logger from an Arduino and an SD card.

### 2021-01-13

Display seems to be a MI12864GO 128x64 OLED with an SSD1305 driver run over either SPI or I2C. Pin 5 is Vdd. IC logic levels for SSD1305 is 3V3. Panel driving is 12V. Vcc is Pin 23, which is connected to Black pin 1.

I've updated the pinouts in yesterday's entry, which is not ideal, but it'll be fine.

Some conclusions then: I'm gonna assume that the RS232 markings are correct, and that signalling is done using RS232 TX/RX pins. It's as of yet unclear what levels the signaling is at. If my guesses as to voltages are correct, I'm guessing it's using 12V signalling, otherwise the transistors would be unnecessary. I guess it's time to disassemble the Bianca and measure the RX/TX voltages on a live system? `¯\_(ツ)_/¯`

Actually, before I do that, I'm also gonna build a small board with just some pins to go inbetween the control box and the LCC, just so that I can assemble the Bianca again and still have access to the signalling pins.

As for the OBP port, that probably uses standard Atmel ISP. I'm gonna get a programming harness and see if I can read the firmware from it, though it's not unlikely that reading it has been fused off.

Addendum: The SSD1305 logic level is 3V3. The microcontroller is directly connected to signalling pins on the SSD1305. Clearly data going out from the microcontroller does so at 3V3 levels. That should mean the microcontroller is 3V3 powered... But that begs the question of what's up with Black 6.

### 2021-01-13

I remembered my Bus Pirate can program AVRs, and decided to hook it up to the OBP port. Sadly (but very much expectedly), lock bits are set such that the flash cannot be easily read.

### 2021-01-12

I got the LCC, and took high-res photos of it. 

The outside is labeled as being 12VDC, and the circuit board labels the black port as RS232, which is consistent with 12VDC. The red port is labled OBP, which could be short for on-board programmer. The microcontroller is labeled as an Atmel ATMega644-20TU-TW, which is very peculiar since a Google search doesn't seem to turn up anything for that part number.

* There is no MAX232 chip on the board. Those are *super* common in RS232 designs.
* The pinout of the microcontroller seems consistent with other ATMega644s (pin numbers in standard TQFP44 counter clock-wise numbering from the notch; for the ports on the board, Red is numbered with 1 being close to silk screen 1, Black is numbered in the same manner)
  * Pin 1 (PCINT13/ICP3/MOSI PB5) is connected to Red 3
  * Pin 2 (PCINT14/OC3A/MISO PB6) is connected to Red 4
  * Pin 3 (PCINT15/SCK PB7) is connected to Red 5
  * Pin 4 (Reset) is connected to Red 2
  * Looking at the trace width 5 and 6 should be VCC or GND (consistent)
  * 7 and 8 is connected to an oscillator, which is consistent with them being XTAL2/XTAL1.
  * Pin 9 (PCINT24/RXD0/T3* PD0) is connected to the collector of a A8A transistor, the base of which is connected to Black 3, and the emitter to ground
  * Pin 10 (PCINT25/TXD0 PD1) is connected to a via routed to the base of an A8K transistor, the collector of which is Black 2, and the emitter to ground
* Red pinout:
  * Pin 1: Pin 6 on Black, probably VCC
  * Red 2: Reset (pulled high)
  * Red 3: PB5 (MOSI?)
  * Red 4: PB6 (MISO?)
  * Red 5: PB7 (SCK?)
  * Pin 6: GND
* Black pinout:
  * Pin 1: 12V presumably
  * Pin 2: TX?
  * Pin 3: RX?
  * Pin 4: GND
  * Pin 5: 3V3 very likely
  * Pin 6: 5V presumably

### 2021-01-09

Ordered a spare LCC (Lelit P/N 9600045). Did some research on the internet, and started this log. I'm planning to open up the LCC, take high-res photos and try to determine the following:

* Are there any COTS chips inside the LCC from which we might glean insights? 
* What is the pinout of the Black connector?
* What is the purpose of the Red connector? Could that be a debug port?

## Reference materials

### Links
* [Parts diagram](https://www.1st-line.com/wp-content/uploads/2020/02/Lelit-PL162-parts-diagram-REV04.pdf)
* [Control box diagram front](https://www.espressoxxl.de/media/image/product/61750/lg/9600046~5.jpg)
* [Control box diagram back](https://www.espressoxxl.de/media/image/product/61750/lg/9600046~7.jpg)

### Parts
* LCC
  * Lelit P/N 9600045
  * Has two 6-pin sockets, red and black. Black is connected to Gicar box.
  * Presumably Black is a 4 wire serial bus plus VCC and GND.
* Control board ("Gicar box")
  * Lelit P/N 9600046
  * Has 5 line voltage pins (FA1, FA4, FA7, FA8 and FA10)
    * FA1: Neutral
    * FA4: Line
    * FA7: "EV CAR." (Carichi?)
    * FA8: Not labeled
    * FA10: Pump
  * Has 5 low voltage connectors on the top
    * CN2: 2 pins, Water tank level probe
    * CN4: 2 pins, "Stemp.cald.caffé", probably the temperature probe for the brew boiler
    * CN6: 8 pins, Not labeled, not populated
    * CN9: 2 pins, "SSR Riscaldamento Caldaia Caffe", Controls the SSR for the brew boiler
    * CN10: 6 pins, "Scheda Display Termopid OLED", Connects to the LCC. Presumably a 4 wire serial port plus VCC and GND?
  * Has 4 low voltage connectors on the bottom
    * CN7: 2 pins, Lever actuated microswitch on the front to start the pump
    * CN5: 2 pins, "SSR Riscaldamento Caldaia Vapore", Controls the SSR for the service boiler
    * CN3: 2 pins, "Stemp.cald.vapore", probably the temperature probe for the service boiler
    * CN1: 2 pins, "Livello caldaia vapore", service boiler water level probe
