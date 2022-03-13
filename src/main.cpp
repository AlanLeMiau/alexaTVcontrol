/**************************************************
*	\    ^ ^ 	Project:		alexaTVcontrol
*	 )  (="=)	Program name:	main.cpp
*	(  /   ) 	Author:			Alan Fuentes
*	 \(__)|| 	https://github.com/AlanLeMiau
* This program emulates the remote control of a Phillips TV
* sending commands with an InfraRed LED
* and receiving voice commands through an Echo Dot 3 device.
**************************************************/

#include <ESP8266WiFi.h>
#include <fauxmoESP.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <IRutils.h>
#include "data.h"

#define ID_TV       "Tele"
#define ID_SOURCE   "Fuente"
#define ID_SOUND    "Sonido"

/* ESP01 pinout
 *  
 * |1|(2)(3)|4|
 * (5)(6)(7)(8)
 * 1 -> GND
 * 2 -> GPIO2
 * 3 -> GPIO0
 * 4 -> GPIO3 (RXD)
 * 5 -> GPIO1 (TXD)
 * 6 -> CH_PD
 * 7 -> RESET
 * 8 -> VCC
 */
typedef enum {
    turnON,
    turnOFF,
    soundUp,
    soundDown,
    mute,
    sourceUp,
    sourceDown,
    nocmd,
} cmdTypes;

const uint64_t POWER  = 0x080C;
const uint64_t UP     = 0x1810;//0x1010; 
const uint64_t DOWN   = 0x1811;//0x1011;
const uint64_t ENTER  = 0x1817;//0x1017;
const uint64_t VOLp   = 0x0810;
const uint64_t VOLm   = 0x0811;
const uint64_t MUTE   = 0x080D;
const uint64_t SOURCE = 0x0838;

const int ledIR = 2;
IRsend irsend(ledIR, true);
decode_results results;
fauxmoESP fauxmo;
cmdTypes command = nocmd;
uint8_t miau = LOW;

void wifiSetup() {
    // Set WIFI module to STA mode
    WiFi.mode(WIFI_STA);
    // Connect
    Serial.printf("\n [WIFI] Connecting to %s \n", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    // Wait
    while (WiFi.status() != WL_CONNECTED) {
        delay(100);
        miau = !miau;
        digitalWrite(ledIR, miau);
    }
    miau = 150;
    // Connected!
    Serial.printf("[WIFI] STATION Mode, SSID: %s, IP address: %s\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());

}// end Wi-Fi setup
void setup() {
    pinMode(ledIR, OUTPUT);
    Serial.begin(115200);
    irsend.begin();
    wifiSetup();

    // By default, fauxmoESP creates it's own webserver on the defined port
    // The TCP port must be 80 for gen3 devices (default is 1901)
    // This has to be done before the call to enable()
    fauxmo.createServer(true); // not needed, this is the default value
    fauxmo.setPort(80); // This is required for gen3 devices

    // You have to call enable(true) once you have a WiFi connection
    // You can enable or disable the library at any moment
    // Disabling it will prevent the devices from being discovered and switched
    fauxmo.enable(true);

    // Add virtual devices
    fauxmo.addDevice(ID_TV);
    fauxmo.addDevice(ID_SOURCE);
    fauxmo.addDevice(ID_SOUND);

    fauxmo.onSetState([](unsigned char device_id, const char * device_name, bool state, unsigned char value) {
        
        // Callback when a command from Alexa is received. 
        // You can use device_id or device_name to choose the element to perform an action onto (relay, LED,...)
        // State is a boolean (ON/OFF) and value a number from 0 to 255 (if you say "set kitchen light to 50%" you will receive a 128 here).
        // Just remember not to delay too much here, this is a callback, exit as soon as possible.
        // If you have to do something more involved here set a flag and process it in your main loop.
        
        Serial.printf("[MAIN] Device #%d (%s) state: %s value: %d\n", device_id, device_name, state ? "ON" : "OFF", value);

        // Checking for device_id is simpler if you are certain about the order they are loaded and it does not change.
        // Otherwise comparing the device_name is safer.

        if ( strcmp(device_name, ID_SOUND) == 0 ) {
            if ( state == false )
            {
                command = mute;
            } else if (value > 127) {
                command = soundUp;
            } else if (value < 127) {
                command = soundDown;
            } else {
                command = nocmd;
            } // end if state from ID_SOUND
            
        } else if ( strcmp(device_name, ID_SOURCE) == 0 ) {
            if (value > 127) {
                command = sourceUp;
            } else if (value < 127) {
                command = sourceDown;
            } else {
                command = nocmd;
            } // end if state from ID_SOUND
        } else if ( strcmp(device_name, ID_TV) == 0 ) {
            command = state ? turnON : turnOFF;
                    
        } else {
            command = nocmd;    
        }
    } // end implicit function
    ); // end fauxmo.onSetState
}// end setup

void loop() {
    // fauxmoESP uses an async TCP server but a sync UDP server
    // Therefore, we have to manually poll for UDP packets
    fauxmo.handle();

    // If your device state is changed by any other means (MQTT, physical button,...)
    // you can instruct the library to report the new state to Alexa on next request:
    // fauxmo.setState(ID_YELLOW, true, 255);
    switch (command){
        case turnON:
            Serial.println("command: turnON");
            irsend.sendRC5(POWER);
            command = nocmd;
            break;
        case turnOFF:
            Serial.println("command: turnOFF");
            irsend.sendRC5(POWER);
            command = nocmd;
            break;
        case soundUp:
            Serial.println("command: soundUP");
            irsend.sendRC5(VOLp);
            delay(miau);
            irsend.sendRC5(VOLp);
            delay(miau);
            irsend.sendRC5(VOLp);
            delay(miau);
            irsend.sendRC5(VOLp);
            delay(miau);
            fauxmo.setState(ID_SOUND, true, 127);
            command = nocmd;
            break;
        case soundDown:
            Serial.println("command: soundOFF");
            irsend.sendRC5(VOLm);
            delay(miau);
            irsend.sendRC5(VOLm);
            delay(miau);
            irsend.sendRC5(VOLm);
            delay(miau);
            irsend.sendRC5(VOLm);
            delay(miau);
            fauxmo.setState(ID_SOUND, true, 127);
            command = nocmd;
            break;            
        case sourceUp:
            Serial.println("command: sourceUP");
            irsend.sendRC5(SOURCE);
            delay(miau);
            irsend.sendRC5(UP);
            delay(miau);
            irsend.sendRC5(ENTER);
            fauxmo.setState(ID_SOURCE, true, 127);
            command = nocmd;
            break;
        case sourceDown:
            Serial.println("command: sourceDOWN");
            irsend.sendRC5(SOURCE);
            delay(miau);
            irsend.sendRC5(DOWN);
            delay(miau);
            irsend.sendRC5(ENTER);
            fauxmo.setState(ID_SOURCE, true, 127);
            command = nocmd;
            break;
        case mute:
            Serial.println("command: MUTE");
            irsend.sendRC5(MUTE);
            command = nocmd;
            break;
        default:
            command = nocmd;
            break;            
        }// end switch    
}// end loop