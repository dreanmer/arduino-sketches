/*
 ESP32RET.ino

 Created: June 1, 2020
 Author: Collin Kidder

Copyright (c) 2014-2020 Collin Kidder, Michael Neuweiler

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "config.h"
#include <esp32_can.h>
#include <SPI.h>
#include <esp32_mcp2517fd.h>
#include <Preferences.h>
#include <FastLED.h>
#include "ELM327_Emulator.h"
#include "SerialConsole.h"
#include "wifi_manager.h"
#include "gvret_comm.h"
#include "can_manager.h"
#include "lawicel.h"

byte i = 0;

uint32_t lastFlushMicros = 0;

bool markToggle[6];
uint32_t lastMarkTrigger = 0;

EEPROMSettings settings;
SystemSettings SysSettings;
Preferences nvPrefs;
char deviceName[20];
char otaHost[40];
char otaFilename[100];

uint8_t espChipRevision;

ELM327Emu elmEmulator;

WiFiManager wifiManager;

GVRET_Comm_Handler serialGVRET; //gvret protocol over the serial to USB connection
GVRET_Comm_Handler wifiGVRET; //GVRET over the wifi telnet port
CANManager canManager; //keeps track of bus load and abstracts away some details of how things are done
LAWICELHandler lawicel;

SerialConsole console;

CRGB leds[A5_NUM_LEDS]; // precisa ser definido para não dar erros nos imports

CAN_COMMON *canBuses[NUM_BUSES];

//initializes all the system EEPROM values. Chances are this should be broken out a bit but
//there is only one checksum check for all of them so it's simple to do it all here.
void loadSettings()
{
    Logger::console("Loading settings....");

    //Logger::console("%i\n", espChipRevision);

    for (int i = 0; i < NUM_BUSES; i++) canBuses[i] = nullptr;

    nvPrefs.begin(PREF_NAME, false);

    settings.useBinarySerialComm = nvPrefs.getBool("binarycomm", false);
    settings.logLevel = nvPrefs.getUChar("loglevel", 1); //info
    settings.wifiMode = nvPrefs.getUChar("wifiMode", 2); //Wifi defaults to creating an AP
    settings.enableBT = nvPrefs.getBool("enable-bt", false);
    settings.enableLawicel = nvPrefs.getBool("enableLawicel", true);

    uint8_t defaultVal = 0; //0 = A0, 1 = EVTV ESP32

    settings.systemType = nvPrefs.getUChar("systype", defaultVal);

    if (settings.systemType == 0)
    {
        Logger::console("Running on ESP32C6");
        canBuses[0] = &CAN0;
        //canBuses[1] = &CAN1;
        SysSettings.LED_CANTX = 255;
        SysSettings.LED_CANRX = 255;
        SysSettings.LED_LOGGING = 255;
        SysSettings.LED_CONNECTION_STATUS = 0;
        SysSettings.fancyLED = false;
        SysSettings.logToggle = false;
        SysSettings.txToggle = true;
        SysSettings.rxToggle = true;
        SysSettings.lawicelAutoPoll = false;
        SysSettings.lawicelMode = false;
        SysSettings.lawicellExtendedMode = false;
        SysSettings.lawicelTimestamping = false;
        SysSettings.numBuses = 1;
        SysSettings.isWifiActive = false;
        SysSettings.isWifiConnected = false;
        strcpy(deviceName, "canbus");
        delay(100);
        FastLED.addLeds<LED_TYPE, LED_BUILTIN, COLOR_ORDER>(leds, A0_NUM_LEDS).setCorrection( TypicalLEDStrip );
        FastLED.setBrightness(  BRIGHTNESS );
        leds[0] = CRGB::Red;
        FastLED.show();
        CAN0.setCANPins(GPIO_NUM_2, GPIO_NUM_3);
        // CAN1.setCANPins(GPIO_NUM_2, GPIO_NUM_3);
    }

    if (nvPrefs.getString("SSID", settings.SSID, 32) == 0)
    {
        strcpy(settings.SSID, deviceName);
        // strcat(settings.SSID, "SSID");
    }

    if (nvPrefs.getString("wpa2Key", settings.WPA2Key, 64) == 0)
    {
        strcpy(settings.WPA2Key, "aBigSecret");
    }
    if (nvPrefs.getString("btname", settings.btName, 32) == 0)
    {
        strcpy(settings.btName, deviceName);
        // strcat(settings.btName, deviceName);
    }
    
    char buff[80];
    // for (int i = 0; i < SysSettings.numBuses; i++)
    // {
    //     sprintf(buff, "can%ispeed", i);
    //     settings.canSettings[i].nomSpeed = nvPrefs.getUInt(buff, 500000);
    //     sprintf(buff, "can%i_en", i);
    //     settings.canSettings[i].enabled = nvPrefs.getBool(buff, (i < 2)?true:false);
    //     sprintf(buff, "can%i-listenonly", i);
    //     settings.canSettings[i].listenOnly = nvPrefs.getBool(buff, false);
    //     sprintf(buff, "can%i-fdspeed", i);
    //     settings.canSettings[i].fdSpeed = nvPrefs.getUInt(buff, 5000000);
    //     sprintf(buff, "can%i-fdmode", i);
    //     settings.canSettings[i].fdMode = nvPrefs.getBool(buff, false);
    // }

    sprintf(buff, "can%ispeed", 0);
    settings.canSettings[0].nomSpeed = nvPrefs.getUInt(buff, 50000);
    sprintf(buff, "can%i_en", 0);
    settings.canSettings[0].enabled = nvPrefs.getBool(buff, true);
    sprintf(buff, "can%i-listenonly", 0);
    settings.canSettings[0].listenOnly = nvPrefs.getBool(buff, false);
    sprintf(buff, "can%i-fdspeed", 0);
    settings.canSettings[0].fdSpeed = nvPrefs.getUInt(buff, 5000000);
    sprintf(buff, "can%i-fdmode", 0);
    settings.canSettings[0].fdMode = nvPrefs.getBool(buff, false);

    // sprintf(buff, "can%ispeed", 1);
    // settings.canSettings[1].nomSpeed = nvPrefs.getUInt(buff, 50000);
    // sprintf(buff, "can%i_en", 1);
    // settings.canSettings[1].enabled = nvPrefs.getBool(buff, true);
    // sprintf(buff, "can%i-listenonly", 1);
    // settings.canSettings[1].listenOnly = nvPrefs.getBool(buff, false);
    // sprintf(buff, "can%i-fdspeed", 1);
    // settings.canSettings[1].fdSpeed = nvPrefs.getUInt(buff, 5000000);
    // sprintf(buff, "can%i-fdmode", 1);
    // settings.canSettings[1].fdMode = nvPrefs.getBool(buff, false);

    nvPrefs.end();

    Logger::setLoglevel((Logger::LogLevel)settings.logLevel);

    for (int rx = 0; rx < NUM_BUSES; rx++) SysSettings.lawicelBusReception[rx] = true; //default to showing messages on RX 
}

void setup()
{
    Serial.begin(1000000); //for production
    //Serial.begin(115200); //for testing
    //delay(2000); //just for testing. Don't use in production

    espChipRevision = ESP.getChipRevision();

    Serial.print("Build number: ");
    Serial.println(CFG_BUILD_NUM);

    SysSettings.isWifiConnected = false;

    loadSettings();

    //CAN0.setDebuggingMode(true);
    //CAN1.setDebuggingMode(true);

    canManager.setup();

    if (settings.enableBT) 
    {
        Serial.println("Starting bluetooth");
        elmEmulator.setup();
        // if (SysSettings.fancyLED && (settings.wifiMode == 0) )
        // {
        //     leds[0] = CRGB::Green;
        //     FastLED.show();
        // }
    }
    
    /*else*/ wifiManager.setup();

    SysSettings.lawicelMode = false;
    SysSettings.lawicelAutoPoll = false;
    SysSettings.lawicelTimestamping = false;
    SysSettings.lawicelPollCounter = 0;
    
    elmEmulator.setup();

    Serial.print("Free heap after setup: ");
    Serial.println(esp_get_free_heap_size());

    Serial.print("Done with init\n");
}

/*
Send a fake frame out USB and maybe to file to show where the mark was triggered at. The fake frame has bits 31 through 3
set which can never happen in reality since frames are either 11 or 29 bit IDs. So, this is a sign that it is a mark frame
and not a real frame. The bottom three bits specify which mark triggered.
*/
void sendMarkTriggered(int which)
{
    CAN_FRAME frame;
    frame.id = 0xFFFFFFF8ull + which;
    frame.extended = true;
    frame.length = 0;
    frame.rtr = 0;
    canManager.displayFrame(frame, 0);
}

/*
Loop executes as often as possible all the while interrupts fire in the background.
The serial comm protocol is as follows:
All commands start with 0xF1 this helps to synchronize if there were comm issues
Then the next byte specifies which command this is.
Then the command data bytes which are specific to the command
Lastly, there is a checksum byte just to be sure there are no missed or duped bytes
Any bytes between checksum and 0xF1 are thrown away

Yes, this should probably have been done more neatly but this way is likely to be the
fastest and safest with limited function calls
*/
void loop()
{
    //uint32_t temp32;    
    bool isConnected = false;
    int serialCnt;
    uint8_t in_byte;

    /*if (Serial)*/ isConnected = true;

    if (SysSettings.lawicelPollCounter > 0) SysSettings.lawicelPollCounter--;
    //}

    canManager.loop();
    /*if (!settings.enableBT)*/ wifiManager.loop();

    size_t wifiLength = wifiGVRET.numAvailableBytes();
    size_t serialLength = serialGVRET.numAvailableBytes();
    size_t maxLength = (wifiLength>serialLength) ? wifiLength : serialLength;

    //If the max time has passed or the buffer is almost filled then send buffered data out
    if ((micros() - lastFlushMicros > SER_BUFF_FLUSH_INTERVAL) || (maxLength > (WIFI_BUFF_SIZE - 40)) ) 
    {
        lastFlushMicros = micros();
        if (serialLength > 0) 
        {
            Serial.write(serialGVRET.getBufferedBytes(), serialLength);
            serialGVRET.clearBufferedBytes();
        }
        if (wifiLength > 0)
        {
            wifiManager.sendBufferedData();
        }
    }

    serialCnt = 0;
    while ( (Serial.available() > 0) && serialCnt < 128 ) 
    {
        serialCnt++;
        in_byte = Serial.read();
        serialGVRET.processIncomingByte(in_byte);
    }

    elmEmulator.loop();
}
