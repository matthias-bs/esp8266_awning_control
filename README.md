# esp8266_awning_control

[![CI](https://github.com/matthias-bs/esp8266_awning_control/actions/workflows/CI.yml/badge.svg)](https://github.com/matthias-bs/esp8266_awning_control/actions/workflows/CI.yml)
[![GitHub release](https://img.shields.io/github/release/matthias-bs/esp8266_awning_control?maxAge=3600)](https://github.com/matthias-bs/esp8266_awning_control/releases)
[![License: MIT](https://img.shields.io/badge/license-GPLv3-green)](https://github.com/matthias-bs/esp8266_awning_control/blob/main/LICENSE)

## ESP8266 (secure) MQTT to ISM Radio Band Awning Remote Control

(using Arduino IDE)

### System Setup
- The awning is normally controlled via a handheld 433 MHz (or other [ISM radio band](https://en.wikipedia.org/wiki/ISM_radio_band) depending on where you live) remote control
- The ESP8266 NodeMCU with [**awning_control**](src/awning_control.ino) sketch provides secure MQTT access via WiFi and converts MQTT messages into radio control sequences sent by the FS1000A RF Transmitter
- An MQTT client, such as a smart phone or tablet with [IoT MQTT Panel](https://snrlab.in/iot/iot-mqtt-panel-user-guide) app, can communicate with awning_control from a remote location
- An MQTT broker (such as a [Raspberry Pi](https://www.raspberrypi.org/) with [Mosquitto](https://mosquitto.org/)) passes control and status messages between the two clients
- **Your awning specific radio remote control sequences must initially be received and recorded as described in [thexperiments /
esp8266_RFControl](https://github.com/thexperiments/esp8266_RFControl)!!!**
- **The transmit algorithm suports only fixed (i.e. non-rolling) codes! This means the remote control must always send the same sequence per command.**
- **Please note: There is no feedback from the awning to _awning_control_!**

see figure below

![Awning Control Setup](awning_control_setup-en.png)

Copyright Notice:

* https://www.123rf.com/photo_52895758_stock-vector-sign-yellow-awning.html 

   Copyright: [maudis60](https://www.123rf.com/profile_maudis60)

* https://commons.wikimedia.org/wiki/File:Nodemcu_amica_bot_02.png

   Copyright: [Make Magazin DE](https://commons.wikimedia.org/wiki/User:MakeMagazinDE) 

* all other images: 

   creative commons/public domain


### Hardware

**Schematic**

![Schematic](hw/awning_control_schematic_v1.0.png)


**ESP8266 NodeMCU, Breadboard, RF Transmitter Module and Antenna**

![Perfboard](hw/awning_control_perfboard_v1.0.png)


**Fritzing Parts**

<table>
<tr><td>NodeMCU-v1.0              <td>https://github.com/squix78/esp8266-fritzing-parts/blob/master/nodemcu-v1.0/NodeMCUV1.0.fzpz
<tr><td>FS1000A 433MHz Transmitter<td>https://github.com/AchimPieters/Fritzing-Custom-Parts/releases/tag/0.0.1
<tr><td>Antenna                   <td>https://forum.fritzing.org/t/hc-12-module-433mhz-long-range-1-8km/1976
</table>

    
**Power Supply**

The circuit is powered from the ESP8266 DevKit Micro-USB socket (~70mA @5V).

*__Note:__* Transmitter FS1000a: 3...12V / Receiver YK-MK-5V:   5V


**Bill of Materials**

<table>
<tr>
    <th> Pos.
    <th> Part / Source
    <th> Description
</tr>
<tr>
    <td> 1
    <td> DEBO JT ESP8266 - NodeMCU ESP8266 WiFi-Modul<br>
    https://www.reichelt.de/nodemcu-esp8266-wifi-modul-debo-jt-esp8266-p219900.html
    <td> ESP8266 module / NodeMCU
</tr>
<tr>
    <td> 2
    <td> DEBO 433 RX/TX - Entwicklerboards - 433 MHz RX/TX Modul<br>
         https://www.reichelt.de/entwicklerboards-433-mhz-rx-tx-modul-debo-433-rx-tx-p224219.html<br>
         - or -<br>
         kwmobile 3X 433 MHz Sender Empfänger Funk Modul für Arduino und Raspberry Pi -<br>
         Wireless Transmitter Module<br>
         https://smile.amazon.de/gp/product/B01H2D2RH6<br>
         (3 pcs per unit)
    <td> 433 MHz Receiver / Transmitter Modules<br>
         Receiver: YK-MK-5V<br>
         Transmitter: FS1000A
</tr>
<tr>
    <td> 3
    <td> 433MHz Antenne Helical-Antenne Fernbedienung für Arduino Raspberry Pi<br>
        https://smile.amazon.de/gp/product/B00SO651VU<br>
        (10 pcs per unit)
    <td> Antenna<br>
        (a piece of wire might work as well)
</tr>
<tr>
    <td> 4
    <td> Piece of Breadboard
</tr>
<tr>
    <td> 5
    <td> 3x1 Socket Header (2 pcs)
    <td>
</tr>
<tr>
    <td> 6
    <td> Micro-USB to USB-A Cable
    <td>
</tr>
<tr>
    <td> 7
    <td> Case
    <td>
</tr>
<tr>
    <td> 8
    <td> USB Power Supply
    <td>
</tr>
</table>
   
   
### Software Configuration
   
**RF Control Sequence**

1. Connect the RF Receiver (YK-MK-5V) to the ESP8266 NodeMCU.

    *__Note 1:__* YK-MK-5V needs a 5V power supply!

    *__Note 2:__* The ESP8266 GPIO pins are **not** 5V-compatible! **Make sure the input voltage from the receiver is limited to 3.3V!** 
   
2. Record your remote control sequences as described in [thexperiments / esp8266_RFControl](https://github.com/thexperiments/esp8266_RFControl).

3. Modify the variables `buckets`, `timings_up`, `timings_stop` and `timings_down` in [awning_control.ino](src/awning_control.ino) accordingly.
   
   `buckets[8]` is an array of pulse and pause lengths in microseconds occurring in a sequence. A pulse is simply the transmission on the ISM radio band carrier frequency (e.g. 433 MHz).
   
   `timings_<up|stop|down>[]` contains indices into `buckets[]`, i.e. it points to the pulse/pause lengths in an alternating fashion.
   
   Example:
   ```
   unsigned long buckets[8] = {344,720,1536,4748,7734,0,0,0};
   char timings_up[] = "3210...";
   ```
   This means:
   1. send a pulse of length #3 (4748µs)
   2. make a pause of length #2 (1536µs)
   3. send a pulse of length #1 (720µs)
   4. make a pause of length #0 (344µs)
   
   etc.
   
4. If one command is working and others are not...
   
   Try to compare the sequences. The `buckets` values should be more or less identical. Are the `timings` different in length? Do you see some striking similarities in the sequences? In my case, the receive algorithm missed the first pulse of the `STOP` sequence. Shifting the array by one position revealed the mainly identical sequences; after adding the first pulse length from the `up` or `down` sequences, the `STOP` command worked fine. 
   
**WiFi, MQTT and Security**

Set up your configuration in a file [secrets.h](https://github.com/matthias-bs/esp8266_awning_control/blob/main/secrets.h) which will be included in [esp8266_awning_control.ino](https://github.com/matthias-bs/esp8266_awning_control/blob/main/esp8266_awning_control.ino) (preferred) or edit the file `awning_control.ino` directly (not recommended). Please refer to [BearSSL_Validation](https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266WiFi/examples/BearSSL_Validation) and [ESP_MQTT_Secure](https://github.com/debsahu/ESP_MQTT_Secure) for details.
   
**Arduino IDE Config**

![Arduino IDE Config](https://user-images.githubusercontent.com/83612361/214785475-e2e2516b-c845-4e01-8300-1e8c806bd3a3.png)

   
### Dashboard with [IoT MQTT Panel](https://snrlab.in/iot/iot-mqtt-panel-user-guide)
![awning_control_panel-de+en](https://user-images.githubusercontent.com/83612361/124654778-4395f700-de9f-11eb-89f1-63ba9eb8cf68.png)




**Install _IoT MQTT Panel_ on your Android device**
  
  see [IoT MQTT Panel](https://snrlab.in/iot/iot-mqtt-panel-user-guide)


**Set up *IoT MQTT Panel* from configuration file**

You can either edit the provided [JSON configuration file](IoTMQTTPanel_Awning_Control.json) before importing it or import it as-is and make the required changes in *IoT MQTT Panel*. Don't forget to add the broker's certificate if using Secure MQTT! (in the App: *Connections -> Edit Connections: Certificate path*.)
   
   
**Editing [IoTMQTTPanel_Awning_Control.json](IoTMQTTPanel_Awning_Control.json)**

At the beginning, replace the dummy IP address *123.345.678* and port *8883* by your MQTT broker's IP address/hostname and port, change *Your_Client_ID* and *Your_MQTT_Connection* as needed:
```
[...]
"connections":[{"autoConnect":true,"host":"123.345.678","port":8883,"clientId":"Your_Client_ID","connectionName":"Your_MQTT_Connection"
[...]
```

At the end, change *Your_Username* and *Your_Password* as needed:
```
[...]
"username":"Your_Username","password":"Your_Password"
[...]
```

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
                                                            
### References and Acknowledgements

1. ESP_MQTT_Secure

    https://github.com/debsahu/ESP_MQTT_Secure

2. esp8266_RFControl
         
    https://github.com/thexperiments/esp8266_RFControl
         
3. RFControl
         
   https://github.com/pimatic/RFControl
        
4. "Raspberry Pi LAN" by Dirk Koudstaal 
   
   https://commons.wikimedia.org/wiki/File:Raspberry_Pi_LAN.svg
         
## Disclaimer and Legal

> This project is a community project not for commercial use.
> The authors will not be held responsible in the event of device failure or damages/injuries due to moving objects without any visual feedback.
> Please think twice about any possible harmful consequences!
