///////////////////////////////////////////////////////////////////////////////
// awning_control_wm.ino
//
// Awning Control with 433 MHz RF Transmitter via secure MQTT
// for ESP8266 - variant using WiFiManager
//
// - Secure Connection to MQTT Broker via WiFi
// - RF Control commands In/Out/Stop
// - Partial extension of awning by sending stop after defined time
//
// MQTT subscriptions:
//     <base_topic>/in                (-)
//     <base_topic>/out               ([seconds])
//     <base_topic>/stop              (-)
//
// MQTT publications:
//     <base_topic>/status            ("online"|"dead"$)
//     <base_topic>/last_cmd          ('>' | ']' | '}' | ')' | '<' | '[')*
//
// $ via LWT
//
// *)
// > - moving out
// ] - moved  out
// } - moving partially out
// ) - moved  partially out
// < - moving in
// [ - moved  out
//
// created: 06/2021 updated: 06/2021
//
// This program is Copyright (C) 06/2021 Matthias Prinke
// <m.prinke@arcor.de> and covered by GNU's GPL.
// In particular, this program is free software and comes WITHOUT
// ANY WARRANTY.
//
// History:
//
// 20210628 Created
// 20210629 Fixed handling of <stop> command while previous command is still
//          executed
//          Fixed RF Control sequence for <stop> command 
// 20230129 Added WiFiManager, improved robustness
//
// ToDo:
// 
//
// Notes:
//
// -
//
///////////////////////////////////////////////////////////////////////////////

#include <ESP8266WiFi.h>      // .arduino15/packages/esp8266/hardware/esp8266/3.1.x/libraries/ESP8266WiFi
#include <WiFiClientSecure.h> // .arduino15/packages/esp8266/hardware/esp8266/3.1.x/libraries/ESP8266WiFi
#include <WiFiManager.h>      // https://github.com/tzapu/WiFiManager
#include <MQTT.h>             // https://github.com/256dpi/arduino-mqtt
#include <RFControl.h>        // https://github.com/pimatic/RFControl
#include <time.h>

const char sketch_id[] = "awning_control_ 20230129";

//enable only one of these below, disabling both is fine too.
// #define CHECK_CA_ROOT
// #define CHECK_PUB_KEY
// #define CHECK_FINGERPRINT
#define CERT cert_ISRG_Root_X1

////--------------------------////

#include "secrets.h"
#ifndef SECRETS
    #define HOSTNAME "awning_control"
    const char *AP_PASS  = "your-ap-passwd";

    const char MQTT_HOST[] = "xxx.yyy.zzz.com";
    const int MQTT_PORT = 8883;
    const char MQTT_USER[] = ""; // leave blank if no credentials used
    const char MQTT_PASS[] = ""; // leave blank if no credentials used

    #ifdef CHECK_CA_ROOT
    static const char digicert[] PROGMEM = R"EOF(
    -----BEGIN CERTIFICATE-----
    xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    -----END CERTIFICATE-----
    )EOF";
    #endif

    #ifdef CHECK_PUB_KEY
    // Extracted by: openssl x509 -pubkey -noout -in ca.crt
    static const char pubkey[] PROGMEM = R"KEY(
    -----BEGIN PUBLIC KEY-----
    xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    xxxxxxxx
    -----END PUBLIC KEY-----
    )KEY";
    #endif

    #ifdef CHECK_FINGERPRINT
	// Extracted by: openssl x509 -fingerprint -in fullchain.pem
    static const char fp[] PROGMEM = "AA:BB:CC:DD:EE:FF:00:11:22:33:44:55:66:77:88:99:AA:BB:CC:DD";
    #endif
#endif
    
const int SEND_PIN        = 16; // GPIO16 - D0 on nodeMCU
const int REPEATS         = 3;  // no. of RF remote control command repetitions
const int CONN_RETRIES    = 10; // no. of retries for failing WiFi/MQTT connection 
const unsigned int t_busy = 40000; // time required to move awning completely in/out

// RF remote control pulse sequences
unsigned long buckets[8] = {344,720,1536,4748,7734,0,0,0};

// buckets=340,724,1536,4756,7732,0,0,0&timings=32100101101010010101010110011001010101011001100110011001010101011001010110010101103210010110101001010101011001100101010101100110011001100101010101100101011001010114&repeats=3
char timings_in[] = "3210010110101001010101011001100101010101100110011001100101010101100101011001010114";

char timings_stop[] = "3210010110101001010101011001100101010101100110011001100101010101100110011001100114";

// buckets=340,724,1556,4740,7736,0,0,0&timings=32100101101010010101010110011001010101011001100110011001010101011001011010010110103210010110101001010101011001100101010101100110011001100101010101100101101001011014&repeats=3
char timings_out[] = "3210010110101001010101011001100101010101100110011001100101010101100101101001011014";

// MQTT topics
const char MQTT_SUB_IN[]     = HOSTNAME "/in";
const char MQTT_SUB_OUT[]    = HOSTNAME "/out";
const char MQTT_SUB_STOP[]   = HOSTNAME "/stop";
const char MQTT_PUB_STATUS[] = HOSTNAME "/status";
const char MQTT_PUB_CMD[]    = HOSTNAME "/last_cmd";


//////////////////////////////////////////////////////

#if (defined(CHECK_PUB_KEY) and defined(CHECK_CA_ROOT)) or (defined(CHECK_PUB_KEY) and defined(CHECK_FINGERPRINT)) or (defined(CHECK_FINGERPRINT) and defined(CHECK_CA_ROOT)) or (defined(CHECK_PUB_KEY) and defined(CHECK_CA_ROOT) and defined(CHECK_FINGERPRINT))
#error "cant have both CHECK_CA_ROOT and CHECK_PUB_KEY enabled"
#endif

BearSSL::WiFiClientSecure net;
MQTTClient client;

unsigned long lastMillis = 0;
time_t now;
char last_cmd[2] = "-";
unsigned long ts_stop = 0;
bool new_cmd = false;

void mqtt_connect()
{
    Serial.print("Checking WiFi...");
    int count = 0;
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(1000);
        if (++count == CONN_RETRIES) {
            Serial.println("wifi connection failed.");
            ESP.restart();
        }
    }

    Serial.print("\nMQTT connecting ");
    count = 0;
    while (!client.connect(HOSTNAME, MQTT_USER, MQTT_PASS))
    {
        Serial.print(".");
        delay(1000);
        if (++count == CONN_RETRIES) {
            Serial.println("MQTT connection failed.");
            ESP.restart();
        }
        
    }

    Serial.println("connected!");
    client.subscribe(MQTT_SUB_IN);
    client.subscribe(MQTT_SUB_OUT);
    client.subscribe(MQTT_SUB_STOP);

}

void messageReceived(String &topic, String &payload)
{
    Serial.println("Received [" + topic + "]: " + payload);

    if (last_cmd[0] == '>' || last_cmd[0] == '<' || last_cmd[0] == '}') {
        if (topic != MQTT_SUB_STOP) {
            Serial.println("Busy - command ignored!");
            return;
        }
    }
    new_cmd = true;
    if (topic == MQTT_SUB_IN) {
        Serial.println("CMD: in");
        RFControl::sendByCompressedTimings(SEND_PIN, buckets, timings_in, REPEATS);
        last_cmd[0] = '<';
        ts_stop = millis() + t_busy;
    } else if (topic == MQTT_SUB_OUT) {
        int t = 0;
        if (payload != "") {
            t = payload.toInt();
        }
        Serial.println("CMD: out");
        RFControl::sendByCompressedTimings(SEND_PIN, buckets, timings_out, REPEATS);
        
        if (t > 0) {
            Serial.println(String(t));
            last_cmd[0] = '}';
            ts_stop = millis() + t * 1000;
        } else {
            last_cmd[0] = '>';
            ts_stop = millis() + t_busy;
        }
    } else if (topic == MQTT_SUB_STOP) {
        Serial.println("CMD: stop");
        RFControl::sendByCompressedTimings(SEND_PIN, buckets, timings_stop, REPEATS);
        last_cmd[0] = '#';
        ts_stop = millis();
    }
}

void setup()
{
    Serial.begin(115200);
    //Serial.setDebugOutput(true);
    Serial.println();
    Serial.println();
    Serial.println(sketch_id);
    Serial.println();
    Serial.print("Connect a web client to ESP Access Point: ");
    Serial.println(HOSTNAME);
    Serial.print("Password: ");
    Serial.println(AP_PASS);
    Serial.println("Open http://192.168.4.1 in your web browser and enter your WiFi Access Point credentials.");
    
    WiFiManager wm;

    // After a timeout, the autoConnect function will return regardless of the result
    wm.setConfigPortalTimeout(180);
    
    bool res;
    // res = wm.autoConnect(); // auto generated AP name from chipid
    // res = wm.autoConnect("AutoConnectAP"); // anonymous ap
    res = wm.autoConnect(HOSTNAME, AP_PASS); // password protected ap
    
    if (!res) {
        Serial.println("Failed to connect");
        ESP.restart();
    } 
    
    Serial.println("connected!");

    Serial.print("Setting time using SNTP ");
    configTime(-5 * 3600, 0, "pool.ntp.org", "time.nist.gov");
    now = time(nullptr);
    int count = 0;
    while (now < 1510592825)
    {
        delay(500);
        Serial.print(".");
        now = time(nullptr);
        if (count == CONN_RETRIES) {
            Serial.println("Failed to set time.");
            ESP.restart();
        }
    }
    Serial.println("done!");
    struct tm timeinfo;
    gmtime_r(&now, &timeinfo);
    Serial.print("Current time: ");
    Serial.print(asctime(&timeinfo));
  
    #ifdef CHECK_CA_ROOT
        //BearSSL::X509List cert(digicert);
        BearSSL::X509List cert(CERT);
        net.setTrustAnchors(&cert);
    #endif
    #ifdef CHECK_PUB_KEY
        BearSSL::PublicKey key(pubkey);
        net.setKnownKey(&key);
    #endif
    #ifdef CHECK_FINGERPRINT
        net.setFingerprint(fp);
    #endif
    #if (!defined(CHECK_PUB_KEY) and !defined(CHECK_CA_ROOT) and !defined(CHECK_FINGERPRINT))
        net.setInsecure();
    #endif

    client.begin(MQTT_HOST, MQTT_PORT, net);
    client.onMessage(messageReceived);
    client.setWill(MQTT_PUB_STATUS, "dead", true /* retained*/, true /* qos */);
    mqtt_connect();
}

void loop()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.print("Checking wifi" );
        while (WiFi.waitForConnectResult() != WL_CONNECTED)
        {
            //WiFi.begin(ssid, pass);
            Serial.print(".");
            delay(10);
        }
        Serial.println("connected");
    }
    else
    {
        if (!client.connected())
        {
            mqtt_connect();
        }
        else
        {
            client.loop();
        }
    }

    if ((last_cmd[0] == '}') && (millis() > ts_stop)) {  
        Serial.println("stop");
        RFControl::sendByCompressedTimings(SEND_PIN, buckets, timings_stop, REPEATS);
        last_cmd[0] = ')';
    }
    if ((last_cmd[0] == '>') && (millis() > ts_stop)) {  
        Serial.println("out");
        last_cmd[0] = ']';
    }
    if ((last_cmd[0] == '<') && (millis() > ts_stop)) {  
        Serial.println("in");
        last_cmd[0] = '[';
    }    

    // publish a message roughly every second.
    if ((millis() - lastMillis > 5000) || new_cmd)
    {
        new_cmd = false;
        lastMillis = millis();
        client.publish(MQTT_PUB_CMD, last_cmd, false, 0);
        client.publish(MQTT_PUB_STATUS, "online", false, 0);
    }
}
