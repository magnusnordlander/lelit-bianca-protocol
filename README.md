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
    * CN7: 2 pins, "[U]int.Erogazione", maybe the lever actuated switch on the front
    * CN5: 2 pins, "SSR Riscaldamento Caldaia Vapore", Controls the SSR for the service boiler
    * CN3: 2 pins, "Stemp.cald.vapore", probably the temperature probe for the service boiler
    * CN1: 2 pins, "Livello caldaia vapore", service boiler water level probe
