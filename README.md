# lelit-bianca-protocol
Project notes for trying to sniff and reverse engineer the protocols used for LCC information on the Lelit Bianca

## Goals
* Stage 1: Expose interesting information as an IoT appliance
* Stage 2: Allow for IoT control of set points, sleep mode etc

## Open questions

* What's with the weird encodings?
* What's in the other bits on the CILO protocol?

## Answered questions

* Is the Red port on the LCC a debug port?
  * Not exactly. It's an on-board programming port.
* Is the LCC communicating with the Control board using a serial protocol?
  * Yes, and it contains interesting information.

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
