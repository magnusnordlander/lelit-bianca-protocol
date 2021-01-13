# lelit-bianca-protocol
Project notes for trying to sniff and reverse engineer the protocols used for LCC information on the Lelit Bianca

## Goals
* Stage 1: Expose interesting information as an IoT appliance
* Stage 2: Allow for IoT control of set points, sleep mode etc

## Background

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

## Open questions

* Is the LCC communicating with the Control board using a serial protocol? If so, what is it?
* Is the Red port on the LCC a debug port?

## Project log

### 2021-01-13

Display seems to be a MI12864GO 128x64 OLED with an SSD1305 driver run over either SPI or I2C. Pin 5 is Vdd. IC logic levels for SSD1305 is 3V3. Panel driving is 12V. Vcc is Pin 23, which is connected to Black pin 1.

I've updated the pinouts in yesterday's entry, which is not ideal, but it'll be fine.

Some conclusions then: I'm gonna assume that the RS232 markings are correct, and that signalling is done using RS232 TX/RX pins. It's as of yet unclear what levels the signaling is at. If my guesses as to voltages are correct, I'm guessing it's using 12V signalling, otherwise the transistors would be unnecessary. I guess it's time to disassemble the Bianca and measure the RX/TX voltages on a live system? `¯\_(ツ)_/¯`

Actually, before I do that, I'm also gonna build a small board with just some pins to go inbetween the control box and the LCC, just so that I can assemble the Bianca again and still have access to the signalling pins.

As for the OBP port, that probably uses standard Atmel ISP. I'm gonna get a programming harness and see if I can read the firmware from it, though it's not unlikely that reading it has been fused off.

Addendum: The SSD1305 logic level is 3V3. The microcontroller is directly connected to signalling pins on the SSD1305. Clearly data going out from the microcontroller does so at 3V3 levels. That should mean the microcontroller is 3V3 powered... But that begs the question of what's up with Black 6.

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
