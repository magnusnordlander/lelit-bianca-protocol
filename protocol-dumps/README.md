# Protocol dumps

## Log.txt

A full heat-up and pull captured without much metadata.

## Log2.txt

A full heat-up and two extractions. 

Approximate timestamps for some interesting marks (+- a couple of real-time seconds):

* 217692300: Brew boiler at 73°C
* 229726632: Steam boiler at 57°C
* 412477312: Brew boiler at 121°C, steam boiler at 74°C
* 611134060: Brew boiler at 90°C, steam boiler at 103°C
* 733189296: Brew boiler at 96°C, steam boiler at 119°C
* 907539328: Brew boiler at 121°C, steam boiler at 125°C
* 1247735056: Brew boiler stable at \~93°C
* 1356966800: Brew boiler stable at \~93°C
* 1380976536: Just after extraction
* 1621250516: Just before extraction
* 1663424704: Just after extraction

## Log3.txt

The machine is at heat and does nothing

## Log4.txt

One extraction

## LOG14.TXT

Slightly different format (SD card dump). This is just part of a cold start up.

## LOG15.TXT

A warm start, running the pump a little, and then ending with turning on eco mode, and later sleep mode. This caused the logger's parsing to go bonkers, so I reset it, creating LOG16.TXT

## LOG16.TXT

See LOG15.TXT