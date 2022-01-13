# ZigUP
CC2530 based multi-purpose ZigBee Relais, Switch, Sensor and Router

![Image of ZigUP board](https://github.com/formtapez/ZigUP/raw/master/Pictures/top2.jpg)
[Bottom side](https://raw.githubusercontent.com/formtapez/ZigUP/master/Pictures/bottom.jpg)

# Features

* [Small enough to fit under a normal lightswitch](https://raw.githubusercontent.com/formtapez/ZigUP/master/Pictures/size.jpg) in an european flush-mounted box ("Unterputzdose" - That´s the UP in ZigUP)
* integrated optional ZigBee Router functionality (extends the range of all your other devices)
* Powerful bistable relais for up to 10 amps load
* 2 Inputs for switches/buttons:
	* Input "KEY" directly toggles the relais and outputs a ZigBee message
	* Input "DIG" only outputs a ZigBee message - So your coordinator can decide if the relais has to be toggled or not.
* Input for 16 digital temperature (DS18B20) and 1 humidity sensors (DHT22/AM2302) (Measurements will be reported via ZigBee)
* Input for S0-Bus impulses from power-, water- or gas-meters. Count-Value will be reported via ZigBee)
* Output for one normal LED or up to 10 WS2812B/Neopixel RGB-LEDs (controllable via ZigBee)
* Analog input to measure voltages of up to 32 Volt. (Voltage will be reported via ZigBee)
* Fully equipped debug-port to allow CC Debugger flashing and packet sniffing

# Connection diagrams

![Power and Light connections](https://github.com/formtapez/ZigUP/raw/master/Pictures/connection_Light.png)
![Switch connections](https://github.com/formtapez/ZigUP/raw/master/Pictures/connection_Switch.png)
![S0 connections](https://github.com/formtapez/ZigUP/raw/master/Pictures/connection_S0.png)
![DHT22 connections](https://github.com/formtapez/ZigUP/raw/master/Pictures/connection_DHT22.png)
![DS18B20 connections](https://github.com/formtapez/ZigUP/raw/master/Pictures/connection_DS18B20.png)
![ADC connections](https://github.com/formtapez/ZigUP/raw/master/Pictures/connection_ADC.png)
![LED connections](https://github.com/formtapez/ZigUP/raw/master/Pictures/connection_LED.png)
![WS2812B connections](https://github.com/formtapez/ZigUP/raw/master/Pictures/connection_WS2812B.png)

# Compilation

1. Get **Z-Stack Home 1.2.2a.44539** from http://www.ti.com/tool/Z-STACK-ARCHIVE
2. Get **IAR Embedded Workbench for 8051** from https://www.iar.com/iar-embedded-workbench (you can use the free trial version for one month)
3. Clone ZigUP source to **\Projects\zstack\HomeAutomation\ZigUP\\**
4. Start **\ZigUP\CC2530DB\ZigUP.eww** to load project in IAR
5. Choose between **Router** and **EndDevice** configuration
6. Edit **\Projects\zstack\Tools\CC2530DB\f8wConfig.cfg** to select the ZigBee channel your coordinator is using.
7. Compile

# Flashing with CC Debugger
1. Get **SmartRF Flash Programmer v1.12.8 (not v2.x!)** from https://www.ti.com/tool/flash-programmer
2. Connect CC Debugger to the Debug-Port of ZigUP with an 1:1 cable.
3. Select "Program CCxxxx..." and "System-on-Chip" tab
4. Load HEX-File and perform "Erase, program and verify" action

# Packet Sniffing using CC Debugger
As an alternative usage of this board, it can be used as a ZigBee packet-sniffer in combination with the CC Debugger.
1. Get **PACKET-SNIFFER v2.18.1 (not SNIFFER-2 v1.x!)** from https://www.ti.com/tool/PACKET-SNIFFER
2. Connect CC Debugger to the Debug-Port of ZigUP with an 1:1 cable.
3. Select protocol "IEEE 802.15.4/ZigBee" and click "Start"
4. Change radio channel and click "Start"
5. Re-flash ZigUP firmware when you are done, because it was replaced by a sniffer-firmware.


# Bill of Materials (BOM)

Qty | Value | Package | Parts | URL (exemplary supplier)
:--:|:----- |:------- |:------|:------------------------
5 | 100 | R0805 | R1, R3, R5, R9, R10 | https://uk.farnell.com/multicomp/mcwr08x1000ftl/res-100r-1-0-125w-0805-thick-film/dp/2447552
1 | 1k | R0805 | R8 | https://uk.farnell.com/multicomp/mcsr08x102-jtl/res-1k-5-0-125w-0805-ceramic/dp/2074338
3 | 10k | R0805 | R2, R4, R6 | https://uk.farnell.com/multicomp/mcwr08x1002ftl/res-10k-1-0-125w-0805-thick-film/dp/2447553
1 | 27k | R0805 | R7 | https://uk.farnell.com/multicomp/mcwr08x2702ftl/res-27k-1-0-125w-thick-film/dp/2447620
2 | 100n | C0805 | C4, C5 | https://uk.farnell.com/multicomp/mcu0805r104kct/cap-0-1-f-50v-10-x7r-0805/dp/9406387
3 | 10µ | C0805 | C1, C3, C6 | https://uk.farnell.com/murata/grm21br61e106ma73l/cap-10-f-25v-20-x5r-0805/dp/2611941
1 | 220µ/6V3/EEEFK0J221P | PANASONIC_D | C2 | https://uk.farnell.com/panasonic/eeefk0j221p/cap-220-f-6-3v-radial-smd/dp/1850086
8 | 4148 | 1206 | D1 - D8 | https://uk.farnell.com/taiwan-semiconductor/ts4148-rxg/diode-small-signal-75v-0-15a-1206/dp/2708388
2 | IRFML8244 | SOT23 | T1, T2 | https://uk.farnell.com/infineon/irfml8244trpbf/mosfet-n-ch-25v-5-7a-sot23/dp/1857298
1 | IRM-02-3.3 | IRM-02 | IC1 | https://uk.farnell.com/mean-well/irm-02-3-3/power-supply-ac-dc-3-3v-0-6a/dp/2815480
1 | CC2530 | E18-MS1-PCB | IC2 | https://www.aliexpress.com/item/-/32803052003.html
1 | DSP2A-L2-DC3V | DSP2A-L2 | K1 | https://uk.farnell.com/panasonic-electric-works/dsp2a-l2-dc3v/relay-dpst-no-250vac-30vdc-5a/dp/2095635
1 | MST 2.5A 250V | MST | F1 | https://uk.farnell.com/multicomp/mst-2-5a-250v/fuse-radial-slow-blow-2-5a/dp/1566104
1 | Phoenix 1792876 | PTS-3 | X1 | https://uk.farnell.com/phoenix-contact/1792876/terminal-block-wire-to-brd-3pos/dp/2072378
2 | TE 1-2834021-4 | MSC4 | X2, X3 | https://uk.farnell.com/te-connectivity/1-2834021-4/tb-wire-to-board-4pos-26-20awg/dp/2610379
1 | DEBUG | MA05-2 | X4 | https://uk.farnell.com/amphenol-icc-fci/67997-210hlf/connector-header-10pos-2row-2/dp/2886080
1 | PCB | FR4 | all | Use [Gerber files](https://github.com/formtapez/ZigUP/tree/master/Layout/Gerber) or contact me if you need a bare PCB

