# esp8266_awning_control
## ESP8266 (secure) MQTT to ISM Radio Band Awning Remote Control

(using Arduino IDE)

### System Setup
- The awning is normally controlled via a handheld 433 MHz (or other [ISM radio band](https://en.wikipedia.org/wiki/ISM_radio_band) depending on where you live) remote control
- The ESP8266 NodeMCU with [**awning_control**](src/awning_control.ino) sketch provides secure MQTT access via WiFi and converts MQTT messages into radio control sequences sent by the FS100A RF Transmitter
- An MQTT client, such as a smart phone or tablet with [IoT MQTT Panel](https://snrlab.in/iot/iot-mqtt-panel-user-guide) app, can communicate with awning_control from a remote location
- An MQTT broker (such as a [Raspberry Pi](https://www.raspberrypi.org/) with [Mosquitto](https://mosquitto.org/)) passes control and status messages between the two clients
- **Your awning specific radio remote control sequences must initially be received and recorded as described in [thexperiments /
esp8266_RFControl](https://github.com/thexperiments/esp8266_RFControl)!!!**

see figure below

![Awning Control Setup](awning_control_setup-en.png)

(https://www.123rf.com/photo_52895758_stock-vector-sign-yellow-awning.html - 
Copyright: [maudis60](https://www.123rf.com/profile_maudis60) / all other images: creative commons/public domain)


### Hardware

**Schematic**

![Schematic](hw/awning_control_schematic_v1.0.png)

**ESP8266 NodeMCU, Breadboard, RF Transmitter Module and Antenna**

![Perfboard](hw/awning_control_perfboard_v1.0.png)

The circuit is powered from the ESP8266 DevKit Micro-USB socket (~70mA @5V).

*__Note:__* Transmitter FS1000a: 3...12V / Receiver YK-MK-5V:   5V


**Bill of Materials**

    | Pos. | Part / Source                                                                            | Description                          |
    |------|------------------------------------------------------------------------------------------|--------------------------------------|
    |    1 | DEBO JT ESP8266 - NodeMCU ESP8266 WiFi-Modul                                             | ESP8266 module / NodeMCU             |
    |      | https://www.reichelt.de/nodemcu-esp8266-wifi-modul-debo-jt-esp8266-p219900.html          |                                      |
    | ---- | ---------------------------------------------------------------------------------------- | -------------------------------------|
    |    2 | DEBO 433 RX/TX - Entwicklerboards - 433 MHz RX/TX Modul                                  | 433 MHz Receiver / Transmitter       |
    |      | https://www.reichelt.de/entwicklerboards-433-mhz-rx-tx-modul-debo-433-rx-tx-p224219.html | Modules                              |
    |      | - or -                                                                                   | Receiver:    YK-MK-5V                |
    |      | kwmobile 3X 433 MHz Sender Empfänger Funk Modul für Arduino und Raspberry Pi -           | Transmitter: FS1000A                 |
    |      | Wireless Transmitter Module                                                              |                                      |
    |      | https://smile.amazon.de/gp/product/B01H2D2RH6 (3 pcs per unit)                           |                                      |
    | ---- | ---------------------------------------------------------------------------------------- | ------------------------------------ |
    |    3 | 433MHz Antenne Helical-Antenne Fernbedienung für Arduino Raspberry Pi                    | Antenna                              |
    |      | https://smile.amazon.de/gp/product/B00SO651VU (10 pcs per unit)                          | (a piece of wire might work as well) |
    | ---- | ---------------------------------------------------------------------------------------- | ------------------------------------ |
    |    4 | Piece of Breadboard                                                                      |                                      |
    | ---- | ---------------------------------------------------------------------------------------- | ------------------------------------ |
    |    5 | 3x1 Socket Header (2 pcs)                                                                |                                      |
    | ---- | ---------------------------------------------------------------------------------------- | ------------------------------------ |
    |    6 | Micro-USB to USB-A Cable                                                                 |                                      |
    | ---- | ---------------------------------------------------------------------------------------- | ------------------------------------ |
    |    7 | Case                                                                                     |                                      |
    | ---- | ---------------------------------------------------------------------------------------- | ------------------------------------ |
    |    8 | USB Power Supply                                                                         |                                      |
    | ---- | ---------------------------------------------------------------------------------------- | ------------------------------------ |


### Dashboard with [IoT MQTT Panel](https://snrlab.in/iot/iot-mqtt-panel-user-guide)
![awning_control_panel-de+en](https://user-images.githubusercontent.com/83612361/124654778-4395f700-de9f-11eb-89f1-63ba9eb8cf68.png)

**MQTT Interface**
```
MQTT subscriptions:
     <base_topic>/in           (-)
     <base_topic>/out          ([seconds])
     <base_topic>/stop         (-)

MQTT publications:
     <base_topic>/status       ("online"|"dead"$)
     <base_topic>/last_cmd     ('>' | ']' | '}' | ')' | '<' | '[')*

$ via LWT

*) Last Command Tokens
    > - moving out
    ] - moved  out
    } - moving partially out
    ) - moved  partially out
    < - moving in
    [ - moved  out
```
