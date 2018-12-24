#define INPUT_COMMAND_SIZE          80
void ExecuteCommand(const char *Line)
{
    // first check the plugins
  String cmd = Line;
  String params = "";
  int parampos = getParamStartPos(cmd, 2);
  if (parampos != -1) {
    params = cmd.substring(parampos);
    cmd = cmd.substring(0, parampos - 1);
  }
  if (PluginCall(PLUGIN_WRITE, cmd, params)) {
    return;
  }

  String status = "";
  boolean success = false;
  char TmpStr1[80];
  TmpStr1[0] = 0;
  char Command[80];
  Command[0] = 0;
  int Par1 = 0;
  int Par2 = 0;
  int Par3 = 0;

  GetArgv(Line, Command, 1);
  if (GetArgv(Line, TmpStr1, 2)) Par1 = str2int(TmpStr1);
  if (GetArgv(Line, TmpStr1, 3)) Par2 = str2int(TmpStr1);
  if (GetArgv(Line, TmpStr1, 4)) Par3 = str2int(TmpStr1);


  // Config commands

  if (strcasecmp_P(Command, PSTR("Config")) == 0)
  {
    success = true;
    String strLine = Line;
    String setting = parseString(strLine, 2);
    String strP1 = parseString(strLine, 3);
    String strP2 = parseString(strLine, 4);
    String strP3 = parseString(strLine, 5);
    String strP4 = parseString(strLine, 6);

    if (setting.equalsIgnoreCase(F("Baudrate"))){
      if (Par2){
        Settings.BaudRate = Par2;
        Settings.UseSerial = 1;
        Serial.begin(Settings.BaudRate);
      }
      else
      {
        Settings.BaudRate = Par2;
        Settings.UseSerial = 0;
        pinMode(0, INPUT);
        pinMode(1, INPUT);
      }
    }

    if (setting.equalsIgnoreCase(F("DST"))){
      Settings.DST = (Par2 == 1);
    }

    if (setting.equalsIgnoreCase(F("Mac"))){
      Settings.Mac = Par2;
    }

    if (setting.equalsIgnoreCase(F("Name"))){
      strcpy(Settings.Name, strP1.c_str());
    }

    if (setting.equalsIgnoreCase(F("Network"))){
      char tmpString[26];
      strP1.toCharArray(tmpString, 26);
      str2ip(tmpString, Settings.IP);
      strP2.toCharArray(tmpString, 26);
      str2ip(tmpString, Settings.Subnet);
      strP3.toCharArray(tmpString, 26);
      str2ip(tmpString, Settings.Gateway);
      strP4.toCharArray(tmpString, 26);
      str2ip(tmpString, Settings.DNS);
      IPAddress ip = Settings.IP;
      IPAddress gw = Settings.Gateway;
      IPAddress subnet = Settings.Subnet;
      IPAddress dns = Settings.DNS;
      Ethernet.begin(mac, Settings.IP, Settings.DNS, Settings.Gateway, Settings.Subnet);
    }

    if (setting.equalsIgnoreCase(F("Port"))){
      Settings.Port = Par2;
    }

    if (setting.equalsIgnoreCase(F("Timezone"))){
      Settings.TimeZone = Par2;
    }

  }


  // operational commands

  if (strcasecmp_P(Command, PSTR("Delay")) == 0)
  {
    success = true;
    delayMillis(Par1);
  }

  if (strcasecmp_P(Command, PSTR("Event")) == 0)
  {
    success = true;
    String event = Line;
    event = event.substring(6);
    event.replace("$", "#");
    if (Settings.UseRules)
      rulesProcessing(FILE_RULES, event);
  }

  if (strcasecmp_P(Command, PSTR("MSGBus")) == 0)
  {
    success = true;
    String msg = Line;
    msg = msg.substring(7);
    if (msg[0] == '>') {
      for (byte x = 0; x < CONFIRM_QUEUE_MAX ; x++) {
        if (confirmQueue[x].State == 0) {
          confirmQueue[x].Name = msg;
          confirmQueue[x].Attempts = 9;
          confirmQueue[x].State = 1;
          confirmQueue[x].TimerTicks = 3;
          UDPSend(msg);
          break;
        }
      }
    }
    else
      UDPSend(msg);
  }

  if (strcasecmp_P(Command, PSTR("NTP")) == 0)
  {
    success = true;
    getNtpTime();
  }

  if (strcasecmp_P(Command, PSTR("sdcard")) == 0)
  {
    success = true;
    SelectSDCard(true);
    File root = SD.open("/");
    root.rewindDirectory();
    printDirectory(root, 0);
    root.close();
  }
  
  if (strcasecmp_P(Command, PSTR("SendToUDP")) == 0)
  {
    success = true;
    String strLine = Line;
    String ip = parseString(strLine,2);
    String port = parseString(strLine,3);
    int msgpos = getParamStartPos(strLine,4);
    String message = strLine.substring(msgpos);
    byte ipaddress[4];
    str2ip((char*)ip.c_str(), ipaddress);
    IPAddress UDP_IP(ipaddress[0], ipaddress[1], ipaddress[2], ipaddress[3]);
    portUDP.beginPacket(UDP_IP, port.toInt());
    portUDP.write(message.c_str(), message.length());
    portUDP.endPacket();
  }
  
  if (strcasecmp_P(Command, PSTR("SendToHTTP")) == 0)
  {
    success = true;
    String strLine = Line;
    String host = parseString(strLine,2);
    String port = parseString(strLine,3);
    int pathpos = getParamStartPos(strLine,4);
    String path = strLine.substring(pathpos);
    EthernetClient client;
    if (client.connect(host.c_str(), port.toInt()))
    {
      String request = F("GET ");
      request += path;
      request += F(" HTTP/1.1\r\n");
      request += F("Host: ");
      request += host;
      request += F("\r\n");
//      request += authHeader;
      request += F("Connection: close\r\n\r\n");
      client.print(request);
      
      unsigned long timer = millis() + 200;
      while (!client.available() && millis() < timer)
        delay(1);

      while (client.available()) {
        String line = client.readStringUntil('\n');
        if (line.substring(0, 15) == F("HTTP/1.1 200 OK"))
          addLog(LOG_LEVEL_DEBUG, line);
        delay(1);
      }
      client.flush();
      client.stop();
    }
  }

  if (strcasecmp_P(Command, PSTR("ValueSet")) == 0)
  {
    success = true;
    if (GetArgv(Line, TmpStr1, 3))
    {
      float result = 0;
      Calculate(TmpStr1, &result);
      if (GetArgv(Line, TmpStr1, 2)) {
        String varName = TmpStr1;
        if (GetArgv(Line, TmpStr1, 4))
          setNvar(varName, result, Par3);
        else
          setNvar(varName, result);
      }
    }
  }

  if (strcasecmp_P(Command, PSTR("StringSet")) == 0)
  {
    success = true;
    String sline = Line;
    int pos = getParamStartPos(sline, 3);
    if (pos != -1) {
      if (GetArgv(Line, TmpStr1, 2)) {
        String varName = TmpStr1;
        setSvar(varName, sline.substring(pos));
      }
    }
  }

  if (strcasecmp_P(Command, PSTR("TimerSet")) == 0)
  {
    if (GetArgv(Line, TmpStr1, 2)) {
      String varName = TmpStr1;
      success = true;
      if (Par2)
        setTimer(varName, millis() + (1000 * Par2));
      else
        setTimer(varName, 0L);
    }
  }

  if (strcasecmp_P(Command, PSTR("webPrint")) == 0)
  {
    success = true;
    String wprint = Line;
    if (wprint.length() == 8)
      printWebString = "";
    else
      printWebString += wprint.substring(9);
  }

  if (strcasecmp_P(Command, PSTR("webButton")) == 0)
  {
    success = true;
    String params = Line;
    printWebString += F("<a class=\"");
    printWebString += parseString(params, 2,';');
    printWebString += F("\" href=\"");
    printWebString += parseString(params, 3,';');
    printWebString += F("\">");
    printWebString += parseString(params, 4,';');
    printWebString += F("</a>");
  }

  if (strcasecmp_P(Command, PSTR("W5100")) == 0)
  {
    success = true;
    ShowSocketStatus();
  }
  
  if (strcasecmp_P(Command, PSTR("Reboot")) == 0)
  {
    success = true;
    Reboot();
  }

  if (strcasecmp_P(Command, PSTR("Reset")) == 0)
  {
    success = true;
    ResetFactory();
  }

  if (strcasecmp_P(Command, PSTR("Settings")) == 0)
  {
    success = true;
    char str[20];
    Serial.println();
    Serial.println(F("System Info"));
    IPAddress ip = Ethernet.localIP();
    sprintf_P(str, PSTR("%u.%u.%u.%u"), ip[0], ip[1], ip[2], ip[3]);
    Serial.print(F("  IP Address    : ")); Serial.println(str);
    Serial.print(F("  Free mem      : ")); Serial.println(FreeMem());
  }
  
  if (success)
    status += F("\nOk");
  else  
    status += F("\nUnknown command!");
}

void printDirectory(File dir, int numTabs) {
   while(true) {
     
     File entry =  dir.openNextFile();
     if (! entry) {
       // no more files
       break;
     }
     for (uint8_t i=0; i<numTabs; i++) {
       Serial.print('\t');
     }
     Serial.print(entry.name());
     if (entry.isDirectory()) {
       Serial.println("/");
       printDirectory(entry, numTabs+1);
     } else {
       // files have sizes, directories do not
       Serial.print("\t\t");
       Serial.println(entry.size(), DEC);
     }
     entry.close();
   }
}
