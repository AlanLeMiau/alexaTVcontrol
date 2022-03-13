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
    turnOnOff,
    soundUp,
    soundUpBig,
    soundDown,
    soundDownBig,
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
    WiFi.mode(WIFI_STA);
    Serial.printf("\n [WIFI] Connecting to %s \n", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    while (WiFi.status() != WL_CONNECTED) {
        delay(100);
        miau = !miau;
        digitalWrite(ledIR, miau);
    }
    miau = 50;
    Serial.printf("[WIFI] STATION Mode, SSID: %s, IP address: %s\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
}// end Wi-Fi setup

void changeSound(uint64_t soundCommand, uint8_t repetition){
    for (uint8_t r = 0; r < repetition; r++)
    {
        irsend.sendRC5(soundCommand);
        delay(miau);
    }
} // end changeSound

void changeSource(uint64_t sourceCommand){
    irsend.sendRC5(SOURCE);
    delay(miau);
    irsend.sendRC5(sourceCommand);
    delay(miau);
    irsend.sendRC5(ENTER);
    delay(miau);
} // end changeSource

void setup() {
    Serial.begin(115200);
    irsend.begin();
    wifiSetup();

    fauxmo.createServer(true);
    fauxmo.setPort(80); // This is required for gen3 devices
    fauxmo.enable(true);
    fauxmo.addDevice(ID_TV);
    fauxmo.addDevice(ID_SOURCE);
    fauxmo.addDevice(ID_SOUND);

    fauxmo.onSetState([](unsigned char device_id, const char * device_name, bool state, unsigned char value) {
        Serial.printf("[MAIN] Device #%d (%s) state: %s value: %d\n", device_id, device_name, state ? "ON" : "OFF", value);
        if ( strcmp(device_name, ID_SOUND) == 0 ) {
            if ( state == false )
            {
                command = mute;
            } else if (value > 127) {
                if (value > 250)
                {
                    command = soundUpBig;
                }
                else
                {
                    command = soundUp;
                }
            } else if (value < 127) {
                if (value < 5)
                {
                    command = soundDownBig;
                }
                else
                {
                    command = soundDown;
                }
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
            } // end if state from ID_SOURCE

        } else if ( strcmp(device_name, ID_TV) == 0 ) {
            command = turnOnOff;
                    
        } else {
            command = nocmd;    
        }
    } // end lambda function handler
    ); // end fauxmo.onSetState

    fauxmo.setState(ID_TV, true, 127);
    fauxmo.setState(ID_SOURCE, true, 127);
    fauxmo.setState(ID_SOUND, true, 127);
    Serial.println(ESP.getFreeSketchSpace());
}// end setup

void loop() {
    fauxmo.handle();
    switch (command){
        case turnOnOff:
            Serial.println("command: turnOnOff");
            irsend.sendRC5(POWER);
            command = nocmd;
            break;
        case soundUp:
            Serial.println("command: soundUp");
            changeSound(VOLp, 7);
            fauxmo.setState(ID_SOUND, true, 127);
            command = nocmd;
            break;
        case soundUpBig:
            Serial.println("command: soundUpBig");
            changeSound(VOLp, 13);
            fauxmo.setState(ID_SOUND, true, 127);
            command = nocmd;
            break;
        case soundDown:
            Serial.println("command: soundDown");
            changeSound(VOLm, 7);
            fauxmo.setState(ID_SOUND, true, 127);
            command = nocmd;
            break;            
        case soundDownBig:
            Serial.println("command: soundDownBig");
            changeSound(VOLm, 13);
            fauxmo.setState(ID_SOUND, true, 127);
            command = nocmd;
            break;            
        case sourceUp:
            Serial.println("command: sourceUP");
            changeSource(UP);
            fauxmo.setState(ID_SOURCE, true, 127);
            command = nocmd;
            break;
        case sourceDown:
            Serial.println("command: sourceDOWN");
            changeSource(DOWN);
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