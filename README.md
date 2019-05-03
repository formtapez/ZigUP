# ZigUP
CC2530 based multi-purpose ZigBee Relais, Switch, Sensor and Router

![Image of ZigUP board](Pictures/top2.jpg)

# Features

* Small enough to fit under a normal lightswitch in an european flush-mounted box ("Unterputzdose" - That´s the UP in ZigUP)
* integrated ZigBee Router (extends the range of all your other devices)
* Powerful bistable relais for up to 10 amps load
* 2 Inputs for switches/buttons:
	* Input "KEY" directly toggles the relais and outputs a ZigBee message
	* Input "DIG" only outputs a ZigBee message - So your coordinator can decide if the relais has to be toggled or not.
* Input for digital temperature and humidity sensors (DHT22/AM2302/DS18B20) (Measurements will be reported via ZigBee)
* Input for S0-Bus impulses from power-, water- or gas-meters. Count-Value will be reported via ZigBee)
* Output for one normal LED or up to 10 WS2812B/Neopixel RGB-LEDs (controllable via ZigBee)
* Analog input to measure voltages of up to 32 Volt. (Voltage will be reported via ZigBee)

Hi! I'm your first Markdown file in **StackEdit**. If you want to learn about StackEdit, you can read me. If you want to play with Markdown, you can edit me. Once you have finished with me, you can create new files by opening the **file explorer** on the left corner of the navigation bar.


# Connection diagrams

![Power and Light connections](Pictures/connection_Light.png)
![Switch connections](Pictures/connection_Switch.png)
![S0 connections](Pictures/connection_S0.png)
![DHT22 connections](Pictures/connection_DHT22.png)
![DS18B20 connections](Pictures/connection_DS18B20.png)
![ADC connections](Pictures/connection_ADC.png)
![LED connections](Pictures/connection_LED.png)
![WS2812B connections](Pictures/connection_WS2812B.png)

# Compilation

1. Get **Z-Stack Home 1.2.2a.44539** from http://www.ti.com/tool/Z-STACK-ARCHIVE
2. Get **IAR Embedded Workbench for 8051** from https://www.iar.com/iar-embedded-workbench (you can use the free trial version for one month)
3. Clone ZigUP source to **\Z-Stack Home 1.2.2a.44539\Projects\zstack\HomeAutomation\ZigUP\\**
4. Start **\ZigUP\CC2530DB\ZigUP.ewp** to load project in IAR
5. Set project configuration to "Router".
6. Compile

# Flashing with CC Debugger
1. Get **SmartRF Flash Programmer v1.12.8 (not v2.x!)** from https://www.ti.com/tool/flash-programmer
2. Load HEX-File and click "flash"

