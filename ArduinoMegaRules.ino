/****************************************************************************************************************************\
 * Arduino project "Arduino Easy" © Copyright www.letscontrolit.com
 *
 * This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 * You received a copy of the GNU General Public License along with this program in file 'License.txt'.
 *
 * IDE download    : https://www.arduino.cc/en/Main/Software
 *
 * Source Code     : https://github.com/ESP8266nu/ESPEasy
 * Support         : http://www.letscontrolit.com
 * Discussion      : http://www.letscontrolit.com/forum/
 *
 * Additional information about licensing can be found at : http://www.gnu.org/licenses
\*************************************************************************************************************************/

// This file incorporates work covered by the following copyright and permission notice:

/****************************************************************************************************************************\
* Arduino project "Nodo" © Copyright 2010..2015 Paul Tonkes
*
* This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
* This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
* of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
* You received a copy of the GNU General Public License along with this program in file 'License.txt'.
*
* Voor toelichting op de licentievoorwaarden zie    : http://www.gnu.org/licenses
* Uitgebreide documentatie is te vinden op          : http://www.nodo-domotica.nl
* Compiler voor deze programmacode te downloaden op : http://arduino.cc
\*************************************************************************************************************************/

// ********************************************************************************
//   User specific configuration
// ********************************************************************************


//#define DEBUG_WEB

// Set default configuration settings if you want (not mandatory)
// You can always change these during runtime and save to eeprom
// After loading firmware, issue a 'reset' command to load the defaults.

#define DEFAULT_NAME        "newdevice"         // Enter your device friendly name
#define DEFAULT_SERVER      "192.168.0.8"       // Enter your Domoticz Server IP address
#define DEFAULT_PORT        8080                // Enter your Domoticz Server port value
#define DEFAULT_DELAY       60                  // Enter your Send delay in seconds

#define DEFAULT_USE_STATIC_IP   false           // true or false enabled or disabled set static IP
#define DEFAULT_IP          "192.168.0.50"      // Enter your IP address
#define DEFAULT_DNS         "192.168.0.1"       // Enter your DNS
#define DEFAULT_GW          "192.168.0.1"       // Enter your gateway
#define DEFAULT_SUBNET      "255.255.255.0"     // Enter your subnet

#define DEFAULT_PROTOCOL    1                   // Protocol used for controller communications

#define FEATURE_RULES true

// ********************************************************************************
//   DO NOT CHANGE ANYTHING BELOW THIS LINE
// ********************************************************************************

// Challenges on Arduino/W5100 ethernet platform:
// Only 4 ethernet sockets:
//  1: UPD traffic server/send
//  2: Webserver
//  3: MQTT client
//  4: Webclient, active when webserver serves an incoming client or outgoing webclient calls.

#define socketdebug                     false

#define CMD_REBOOT                         89

#define UNIT_MAX                           32
#define PLUGIN_MAX                         16
#define RULES_TIMER_MAX                     8
#define RULES_MAX_SIZE                    512
#define RULES_MAX_NESTING_LEVEL             3

#define USER_VAR_MAX                        8
#define USER_STRING_VAR_MAX                 8
#define CONFIRM_QUEUE_MAX                   8

#define LOG_LEVEL_ERROR                     1
#define LOG_LEVEL_INFO                      2
#define LOG_LEVEL_DEBUG                     3
#define LOG_LEVEL_DEBUG_MORE                4

#define PLUGIN_INIT                         2
#define PLUGIN_READ                         3
#define PLUGIN_ONCE_A_SECOND                4
#define PLUGIN_TEN_PER_SECOND               5
#define PLUGIN_WRITE                       13
#define PLUGIN_EVENT_OUT                   14
#define PLUGIN_SERIAL_IN                   16
#define PLUGIN_UDP_IN                      17
#define PLUGIN_CLOCK_IN                    18
#define PLUGIN_TIMER_IN                    19

#define INT_MIN -32767
#define INT_MAX 32767

#define FILE_BOOT         "boot.txt"
#define FILE_RULES        "rules.txt"

#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <base64.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <DNS.h>

void(*Reboot)(void)=0;
void setNvar(String varName, float value, int decimals = -1);
String parseString(String& string, byte indexFind, char separator = ',');

byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};

EthernetServer MyWebServer(80);

#define EthernetShield_CS_SDCard     4
#define EthernetShield_CS_W5100     10  

// syslog stuff
EthernetUDP portUDP;

struct SecurityStruct
{
  char          ControllerUser[26];
  char          ControllerPassword[64];
  char          Password[26];
} SecuritySettings;

struct SettingsStruct
{
  byte          Mac;
  byte          IP[4];
  byte          Gateway[4];
  byte          Subnet[4];
  byte          DNS[4];
  char          NTPHost[64];
  unsigned long Delay;
  unsigned int  Port;
  char          Name[26];
  byte          SerialLogLevel;
  unsigned long BaudRate;
  unsigned long MessageDelay;
  boolean       UseNTP;
  boolean       DST;
  int8_t        PinBootStates[17];
  byte          UseDNS;
  boolean       UseRules;
  int8_t        Pin_status_led;
  boolean       UseSerial;
  int16_t       TimeZone;
  byte          SDLogLevel;
} Settings;

struct NodeStruct
{
  byte IP[4];
  byte age;
  String nodeName;
} Nodes[UNIT_MAX];

struct nvarStruct
{
  String Name;
  float Value;
  byte Decimals;
} nUserVar[USER_VAR_MAX];

struct svarStruct
{
  String Name;
  String Value;
} sUserVar[USER_STRING_VAR_MAX];

struct timerStruct
{
  String Name;
  unsigned long Value;
} RulesTimer[RULES_TIMER_MAX];

struct confirmQueueStruct
{
  String Name;
  byte Attempts;
  byte State;
  byte TimerTicks;
} confirmQueue[CONFIRM_QUEUE_MAX];

String printWebString = "";

unsigned long timer100ms;
unsigned long timer1s;
unsigned long timer60s;
unsigned long uptime = 0;

byte cmd_within_mainloop = 0;

boolean (*Plugin_ptr[PLUGIN_MAX])(byte, String&, String&);
byte Plugin_id[PLUGIN_MAX];
String dummyString = "";
int freeMem;

/*********************************************************************************************\
 * SETUP
\*********************************************************************************************/
void setup()
{
  fileSystemCheck();
  // Defaults when no config is set
  Settings.Mac=0;
  Settings.IP[0]=0;
  Settings.UseSerial=1;
  Settings.BaudRate=115200;
  Settings.UseRules=1;
  Settings.Name[0] = 'N';
  Settings.Name[1] = 0;
  Settings.Delay=60;
  Settings.SerialLogLevel=4;
  Serial.begin(Settings.BaudRate);

  String event = F("System#Config");
  rulesProcessing(FILE_BOOT, event);
  rulesProcessing(FILE_RULES, event);

  if (Settings.UseSerial)
    Serial.begin(Settings.BaudRate);

  mac[5] = Settings.Mac; // make sure every unit has a unique mac address
  
  if (Settings.IP[0] == 0)
    Ethernet.begin(mac);
  else
    Ethernet.begin(mac, Settings.IP, Settings.DNS, Settings.Gateway, Settings.Subnet);

  // Add myself to the node list
  IPAddress IP = Ethernet.localIP();
  for (byte x = 0; x < 4; x++)
    Nodes[0].IP[x] = IP[x];
  Nodes[0].age = 0;
  Nodes[0].nodeName = Settings.Name;

  portUDP.begin(Settings.Port); // setup for NTP and other stuff if no user port is selected
      
  PluginInit();

  MyWebServer.begin();

  String log = F("\nINIT : Booting");
  addLog(LOG_LEVEL_INFO, log);

  timer100ms = millis() + 100; // timer for periodic actions 10 x per/sec
  timer1s = millis() + 1000; // timer for periodic actions once per/sec
  timer60s = millis() + 60000; // timer for periodic actions once per 60 sec

  if (Settings.UseNTP)
    initTime();

  if (Settings.UseRules)
  {
    event = F("System#Boot");
    rulesProcessing(FILE_BOOT, event);
    rulesProcessing(FILE_RULES, event);
  }

  log = F("INIT : Boot OK");
  addLog(LOG_LEVEL_INFO, log);
  #if socketdebug
    ShowSocketStatus();
  #endif
}


/*********************************************************************************************\
 * MAIN LOOP
\*********************************************************************************************/
void loop()
{
  if (Settings.UseSerial)
    if (Serial.available())
      if (!PluginCall(PLUGIN_SERIAL_IN, dummyString, dummyString))
        serial();

    if (millis() > timer100ms)
      run10TimesPerSecond();

    if (millis() > timer1s)
      runOncePerSecond();

    if (millis() > timer60s)
      runEach60Seconds();

  WebServerHandleClient();
  MSGBusReceive();
}


/*********************************************************************************************\
 * Tasks that run 10 times per second
\*********************************************************************************************/
void run10TimesPerSecond()
{
  timer100ms = millis() + 100;
  MSGBusQueue();
  PluginCall(PLUGIN_TEN_PER_SECOND, dummyString, dummyString);
}


/*********************************************************************************************\
 * Tasks each second
\*********************************************************************************************/
void runOncePerSecond()
{
  freeMem = FreeMem();
  timer1s = millis() + 1000;
  if (cmd_within_mainloop != 0)
  {
    switch (cmd_within_mainloop)
    {
      case CMD_REBOOT:
        {
          Reboot();
          break;
        }
    }
    cmd_within_mainloop = 0;
  }

  // clock events
  if (Settings.UseNTP)
    checkTime();

  unsigned long timer = micros();
  PluginCall(PLUGIN_ONCE_A_SECOND, dummyString, dummyString);

  if (Settings.UseRules)
    rulesTimers();
}

/*********************************************************************************************\
 * Tasks each 30 seconds
\*********************************************************************************************/
void runEach60Seconds()
{
  uptime++;
  timer60s = millis() + 60000;
  MSGBusAnnounceMe();
  refreshNodeList();
}

