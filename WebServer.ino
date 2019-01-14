String webdata = "";

class DummyWebServer
{
public:
  String arg(String name);        // get request argument value by name
protected:
};

String DummyWebServer::arg(String arg)
{
  #ifdef DEBUG_WEB2
    Serial.print("webdata3:");
    Serial.println(webdata);
  #endif
  
  arg = "&" + arg;

  #ifdef DEBUG_WEB2
    Serial.print("arg:");
    Serial.println(arg);
  #endif

  String returnarg = "";
  int pos = webdata.indexOf(arg);
  if (pos >= 0)
  {
    returnarg = webdata.substring(pos+1,pos+81); // max field content 80 ?
    pos = returnarg.indexOf("&");
    if (pos > 0)
      returnarg = returnarg.substring(0, pos);
    pos = returnarg.indexOf("=");
    if (pos > 0)
      returnarg = returnarg.substring(pos + 1);
  }
  return returnarg;
}

DummyWebServer WebServer;


void WebServerHandleClient() {
  EthernetClient client = MyWebServer.available();
  if (client) {
#if socketdebug
    ShowSocketStatus();
#endif
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    String request = "";
    boolean getrequest = true;
    webdata = "";
    webdata.reserve(500);
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        if (getrequest)
          request += c;

        #ifdef DEBUG_WEB
          Serial.write(c);
        #endif
        
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) {

          // request is known here, in case of 'rules' we could handle this smarter (save index+6)?
          while (client.available()) { // post data...
            char c = client.read();
            webdata += c;
          }

          if (webdata.length() !=0)
            webdata = "&" + webdata;
            
          #ifdef DEBUG_WEB
            Serial.print("webdata0:");
            Serial.println(webdata);
            Serial.print("len:");
            Serial.println(webdata.length());
          #endif

          int pos = request.indexOf("/");
          if (pos > 0)
            request = request.substring(pos + 1);
          pos = request.indexOf(" ");
          if (pos > 0)
            request = request.substring(0, pos);
          pos = request.indexOf("?");
          if (pos >= 0)
          {
            String args = request.substring(pos + 1);
            webdata += "&" + args;
            request = request.substring(0, pos);
          }

          webdata = URLDecode(webdata.c_str());

          #ifdef DEBUG_WEB
            Serial.print("webdata1:");
            Serial.println(webdata);
          #endif
          
          if (request.startsWith(F(" HTTP")) or request.length() == 0) // root page
          {
            addHeader(true, client);
            handle_root(client, webdata);
          }
          else if (request.equals(F("control")))
          {
            handle_control(client, webdata);
          }
          else if (request.equals(F("boot")))
          {
            addHeader(true, client);
            handle_boot(client, webdata);
          }
          else if (request.equals(F("rules")))
          {
            addHeader(true, client);
            handle_rules(client, webdata);
          }
          else if (request.equals(F("tools")))
          {
            addHeader(true, client);
            handle_tools(client, webdata);
          }
          else if (request.equals(F("SDfilelist")))
          {
            addHeader(true, client);
            handle_SDfilelist(client, webdata);
          }
          else
            handle_unknown(client, request);
          break;
        }

        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
          getrequest = false;
        } else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }
    // give the web browser time to receive the data
    delay(1);
    client.stop();
    webdata = "";
  }
}


//********************************************************************************
// Add top menu
//********************************************************************************
void addHeader(boolean showMenu, EthernetClient client)
{
  client.println(F("HTTP/1.1 200 OK"));
  client.println(F("Content-Type: text/html"));
  client.println(F("Connection: close"));  // the connection will be closed after completion of the response
  client.println();

  client.println(F("<meta name=\"viewport\" content=\"width=width=device-width, initial-scale=1\">"));
  client.println(F("<STYLE>* {font-family:sans-serif; font-size:12pt;}"));
  client.println(F("h1 {font-size: 16pt; color: #07D; margin: 8px 0; font-weight: bold;}"));
  client.println(F(".button {margin:4px; padding:5px 15px; background-color:#07D; color:#FFF; text-decoration:none; border-radius:4px}"));
  client.println(F(".button-link {padding:5px 15px; background-color:#07D; color:#FFF; border:solid 1px #FFF; text-decoration:none}"));
  client.println(F(".button-widelink {display: inline-block; width: 100%; text-align: center; padding:5px 15px; background-color:#07D; color:#FFF; border:solid 1px #FFF; text-decoration:none}"));
  client.println(F(".button-nodelink {display: inline-block; width: 100%; text-align: center; padding:5px 15px; background-color:#888; color:#FFF; border:solid 1px #FFF; text-decoration:none}"));
  client.println(F(".button-nodelinkA {display: inline-block; width: 100%; text-align: center; padding:5px 15px; background-color:#28C; color:#FFF; border:solid 1px #FFF; text-decoration:none}"));
  client.println(F("td {padding:7px;}"));
  client.println(F("</STYLE>"));
  client.println(F("<h1>"));
  client.println(Settings.Name);
  client.println(F("</h1>"));
  
  String str = "";
  str += F("<script language=\"javascript\"><!--\n");
  str += F("function dept_onchange(frmselect) {frmselect.submit();}\n");
  str += F("//--></script>");
  str += F("<head><title>");
  str += Settings.Name;
  str += F("</title>");
  str += F("</head>");

  if (showMenu)
  {
    str += F("<a class=\"button-link\" href=\".\">Main</a>");
    str += F("<a class=\"button-link\" href=\"boot\">Boot</a>");
    str += F("<a class=\"button-link\" href=\"rules\">Rules</a>");
    str += F("<a class=\"button-link\" href=\"tools\">Tools</a><BR><BR>");
  }
  client.print(str);
}


//********************************************************************************
// Web Interface root page
//********************************************************************************
byte sortedIndex[UNIT_MAX + 1];
void handle_root(EthernetClient client, String &post) {

  String sCommand = WebServer.arg(F("cmd"));
  String group = WebServer.arg("group");
  boolean groupList = true;

  if (group != "")
    groupList = false;

  if (strcasecmp_P(sCommand.c_str(), PSTR("reboot")) != 0)
  {
    String reply = "";
    reply.reserve(500);

    if (sCommand.length() > 0)
      ExecuteCommand(sCommand.c_str());

    String event = F("Web#Print");
    rulesProcessing(FILE_RULES, event);
    
    client.print(printWebString);

    reply += F("<form><table>");

    // first get the list in alphabetic order
    for (byte x = 0; x <= UNIT_MAX; x++)
      sortedIndex[x] = x;

   if (groupList == true) {
      // Show Group list
      sortDeviceArrayGroup(); // sort on groupname
      String prevGroup = "?";
      for (byte x = 0; x < UNIT_MAX; x++)
      {
        byte index = sortedIndex[x];
        if (Nodes[index].IP[0] != 0) {
          String group = Nodes[index].group;
          if (group != prevGroup)
          {
            prevGroup = group;
            reply += F("<TR><TD><a class=\"");
            reply += F("button-nodelink");
            reply += F("\" ");
            reply += F("href='/?group=");
            reply += group;
            reply += "'>";
            reply += group;
            reply += F("</a>");
            reply += F("<TD>");
            client.print(reply);
            reply = "";
          }
        }
      }
      // All nodes group button
      reply += F("<TR><TD><a class=\"button-nodelink\" href='/?group=*'>_ALL_</a><TD>");
   }
   else{
     for (byte x = 0; x < UNIT_MAX; x++)
      {
        sortDeviceArray();
        byte index = sortedIndex[x];
        if (Nodes[index].IP[0] != 0 && (group == "*" || Nodes[index].group == group))
        {
          String buttonclass ="";
          if ((String)Settings.Name == Nodes[index].nodeName)
            buttonclass = F("button-nodelinkA");
          else
            buttonclass = F("button-nodelink");
          reply += F("<TR><TD><a class=\"");
          reply += buttonclass;
          reply += F("\" ");
          char url[40];
          sprintf_P(url, PSTR("href='http://%u.%u.%u.%u"), Nodes[index].IP[0], Nodes[index].IP[1], Nodes[index].IP[2], Nodes[index].IP[3]);
          reply += url;
          if (group != "") {
            reply += F("?group=");
            reply += Nodes[index].group;
          }
          reply += "'>";
          reply += Nodes[index].nodeName;
          reply += F("</a>");
          reply += F("<TD>");
          client.print(reply);
          reply = "";
        }
      }
    }
    reply += F("</table></form>");
    client.print(reply);
  }
  else
  {
    // have to disconnect or reboot from within the main loop
    // because the webconnection is still active at this point
    // disconnect here could result into a crash/reboot...
    if (strcasecmp_P(sCommand.c_str(), PSTR("reboot")) == 0)
    {
      String log = F("     : Rebooting...");
      addLog(LOG_LEVEL_INFO, log);
      cmd_within_mainloop = CMD_REBOOT;
    }
    client.print("ok");
  }
}


//********************************************************************************
// Device Sort routine, switch array entries
//********************************************************************************
void switchArray(byte value)
{
  byte temp;
  temp = sortedIndex[value - 1];
  sortedIndex[value - 1] = sortedIndex[value];
  sortedIndex[value] = temp;
}

//********************************************************************************
// Device Sort routine, compare two array entries
//********************************************************************************
boolean arrayLessThan(const String& ptr_1, const String& ptr_2)
{
  unsigned int i = 0;
  while (i < ptr_1.length())    // For each character in string 1, starting with the first:
  {
    if (ptr_2.length() < i)    // If string 2 is shorter, then switch them
    {
      return true;
    }
    else
    {
      const char check1 = (char)ptr_1[i];  // get the same char from string 1 and string 2
      const char check2 = (char)ptr_2[i];
      if (check1 == check2) {
        // they're equal so far; check the next char !!
        i++;
      } else {
        return (check2 > check1);
      }
    }
  }
  return false;
}


//********************************************************************************
// Device Sort routine, actual sorting
//********************************************************************************
void sortDeviceArray()
{
  int innerLoop ;
  int mainLoop ;
  for ( mainLoop = 1; mainLoop <= UNIT_MAX; mainLoop++)
  {
    innerLoop = mainLoop;
    while (innerLoop  >= 1)
    {
      String one = Nodes[sortedIndex[innerLoop]].nodeName;
      String two = Nodes[sortedIndex[innerLoop-1]].nodeName;
      if (arrayLessThan(one,two))
      {
        switchArray(innerLoop);
      }
      innerLoop--;
    }
  }
}


//********************************************************************************
// Device Sort routine, actual sorting
//********************************************************************************
void sortDeviceArrayGroup()
{
  int innerLoop ;
  int mainLoop ;
  for ( mainLoop = 1; mainLoop < UNIT_MAX; mainLoop++)
  {
    innerLoop = mainLoop;
    while (innerLoop  >= 1)
    {
      String one = Nodes[sortedIndex[innerLoop]].group;
      if(one.length()==0) one = "_";
      String two = Nodes[sortedIndex[innerLoop - 1]].group;
      if(two.length()==0) two = "_";
      if (arrayLessThan(one, two))
      {
        switchArray(innerLoop);
      }
      innerLoop--;
    }
  }
}


//********************************************************************************
// Web Interface boot rules page
//********************************************************************************
void handle_boot(EthernetClient client, String &post) {

  String rules = post.substring(7);
  webdata = "";

  if (rules.length() > 0)
  {
    SD.remove(F("boot.txt"));
    File f = SD.open(F("boot.txt"), FILE_WRITE);
    if (f)
    {
      f.print(rules);
      f.close();
    }
  }

  rules = "";
  String reply = "";
  reply += F("<table><TR><TD>");

  // load form data from flash
  reply += F("<form method='post'>");
  reply += F("<textarea name='rules' rows='15' cols='80' wrap='off'>");
  client.print(reply);
  reply = "";

  File dataFile = SD.open(F("boot.txt"));
  int filesize = dataFile.size();
  while (dataFile.available()) {
    client.write(dataFile.read());
  }
  reply += F("</textarea>");

  reply += F("<TR><TD>Current size: ");
  reply += filesize;
  reply += F(" characters (Max ");
  reply += RULES_MAX_SIZE;
  reply += F(")");

  reply += F("<TR><TD><input class=\"button-link\" type='submit' value='Submit'>");
  reply += F("</table></form>");
  client.print(reply);
}


//********************************************************************************
// Web Interface rules page
//********************************************************************************
void handle_rules(EthernetClient client, String &post) {

  String rules = post.substring(7);
  webdata = "";

  if (rules.length() > 0)
  {
    SD.remove(F("rules.txt"));
    File f = SD.open(F("rules.txt"), FILE_WRITE);
    if (f)
    {
      f.print(rules);
      f.close();
    }
  }

  rules = "";
  String reply = "";
  reply += F("<table><TR><TD>");

  // load form data from flash
  reply += F("<form method='post'>");
  reply += F("<textarea name='rules' rows='15' cols='80' wrap='off'>");
  client.print(reply);
  reply = "";

  File dataFile = SD.open(F("rules.txt"));
  int filesize = dataFile.size();
  while (dataFile.available()) {
    client.write(dataFile.read());
  }
  reply += F("</textarea>");

  reply += F("<TR><TD>Current size: ");
  reply += filesize;
  reply += F(" characters (Max ");
  reply += RULES_MAX_SIZE;
  reply += F(")");

  reply += F("<TR><TD><input class=\"button-link\" type='submit' value='Submit'>");
  reply += F("</table></form>");
  client.print(reply);
}


//********************************************************************************
// Web Interface debug page
//********************************************************************************
void handle_tools(EthernetClient client, String &post) {

  String reply = "";
  reply += F("<form>");
  reply += F("<table><TH>Tools<TH>");
  reply += F("<TR><TD>Filesystem<TD><a class=\"button-link\" href=\"/SDfilelist\">Files</a><BR><BR>");
  reply += F("<TR><TD>System<TD><a class=\"button-link\" href=\"/?cmd=reboot\">Reboot</a>");
  reply += F("<TR><TD>Uptime:<TD>");

  char strUpTime[40];
  int minutes = uptime;
  int days = minutes / 1440;
  minutes = minutes % 1440;
  int hrs = minutes / 60;
  minutes = minutes % 60;
  sprintf_P(strUpTime, PSTR("%d days %d hours %d minutes"), days, hrs, minutes);
  reply += strUpTime;

  reply += F("<TR><TD>Free Mem:<TD>");
  reply += freeMem;

  reply += F("</table></form>");
  client.print(reply);
  printWebString = "";
}


//********************************************************************************
// Web Interface control page (no password!)
//********************************************************************************
void handle_control(EthernetClient client, String &post) {

  String webrequest = WebServer.arg(F("cmd"));

  if (webrequest.length() > 0)
    ExecuteCommand(webrequest.c_str());

  String reply = "";

  client.println(F("HTTP/1.1 200 OK"));
  client.print(F("Content-Type:"));
  client.print(F("Content-Type: text/html"));
  client.println(F("Connection: close"));
  client.println("");
  client.print(reply);
}


//********************************************************************************
// Web Interface SD card file list
//********************************************************************************
void handle_SDfilelist(EthernetClient client, String &post) {

  String fdelete = WebServer.arg(F("delete"));

  if (fdelete.length() > 0)
  {
    SD.remove((char*)fdelete.c_str());
  }

  String reply = "";
  reply += F("<table cellpadding='4' border='1' frame='box' rules='all'><TH><TH>Filename:<TH>Size");

  File root = SD.open("/");
  root.rewindDirectory();
  File entry = root.openNextFile();
  while (entry)
  {
    if (!entry.isDirectory())
    {
      reply += F("<TR><TD>");
      if (entry.name() != "CONFIG.TXT" && entry.name() != "SECURITY.TXT")
      {
        reply += F("<a class=\"button-link\" href=\"SDfilelist?delete=");
        reply += entry.name();
        reply += F("\">Del</a>");
      }
      reply += F("<TD><a href=\"");
      reply += entry.name();
      reply += F("\">");
      reply += entry.name();
      reply += F("</a>");
      reply += F("<TD>");
      reply += entry.size();
    }
    entry.close();
    entry = root.openNextFile();
  }
  //entry.close();
  root.close();
  reply += F("</table></form>");
  //reply += F("<BR><a class=\"button-link\" href=\"/upload\">Upload</a>");
  client.print(reply);
}


//********************************************************************************
// URNEncode char string to string object
//********************************************************************************
String URLEncode(const char* msg)
{
  const char *hex = "0123456789abcdef";
  String encodedMsg = "";

  while (*msg != '\0') {
    if ( ('a' <= *msg && *msg <= 'z')
         || ('A' <= *msg && *msg <= 'Z')
         || ('0' <= *msg && *msg <= '9') ) {
      encodedMsg += *msg;
    } else {
      encodedMsg += '%';
      encodedMsg += hex[*msg >> 4];
      encodedMsg += hex[*msg & 15];
    }
    msg++;
  }
  return encodedMsg;
}


//********************************************************************************
// Web Interface server web file from SPIFFS
//********************************************************************************
bool handle_unknown(EthernetClient client, String path) {

  String dataType = F("text/plain");
  if (path.endsWith("/")) path += F("index.htm");

  if (path.endsWith(".src")) path = path.substring(0, path.lastIndexOf("."));
  else if (path.endsWith(F(".htm"))) dataType = F("text/html");
  else if (path.endsWith(F(".css"))) dataType = F("text/css");
  else if (path.endsWith(F(".js"))) dataType = F("application/javascript");
  else if (path.endsWith(F(".png"))) dataType = F("image/png");
  else if (path.endsWith(F(".gif"))) dataType = F("image/gif");
  else if (path.endsWith(F(".jpg"))) dataType = F("image/jpeg");
  else if (path.endsWith(F(".ico"))) dataType = F("image/x-icon");
  else if (path.endsWith(F(".txt"))) dataType = F("application/octet-stream");
  else if (path.endsWith(F(".dat"))) dataType = F("application/octet-stream");
  else if (path.endsWith(F(".esp"))) return handle_custom(client, path);
  
  File dataFile = SD.open(path.c_str());
  if (!dataFile)
    return false;

  client.println(F("HTTP/1.1 200 OK"));
  client.print(F("Content-Type:"));
  client.println(dataType);
  client.println(F("Connection: close"));

  if (path.endsWith(F(".TXT")))
  {
    client.println(F("Content-Disposition:attachment"));
  }

  client.println();

  while (dataFile.available()) {
    client.write(dataFile.read());
  }
  dataFile.close();
  return true;
}

//********************************************************************************
// Web Interface custom page handler
//********************************************************************************
boolean handle_custom(EthernetClient client, String path) {
  //path = path.substring(1);
  String reply = "";
  client.println(F("HTTP/1.1 200 OK"));
  client.println(F("Content-Type: text/html"));
  client.println(F("Connection: close"));  // the connection will be closed after completion of the response
  client.println();
  

  // handle commands from a custom page
  String webrequest = WebServer.arg(F("cmd"));
  if (webrequest.length() > 0){
    ExecuteCommand(webrequest.c_str());

    // handle some update processes first, before returning page update...
    PluginCall(PLUGIN_TEN_PER_SECOND, dummyString, dummyString);
  }

  // create a dynamic custom page, parsing task values into [<taskname>#<taskvalue>] placeholders and parsing %xx% system variables
  File dataFile = SD.open(path.c_str(), FILE_READ);
  if (dataFile)
  {
    String page = "";
    while (dataFile.available())
      page += ((char)dataFile.read());

    reply += parseTemplate(page,0);
    dataFile.close();
  }
  else // if the requestef file does not exist, create a default action in case the page is named "dashboard*"
  {
    return false; // unknown file that does not exist...
  }
  client.print(reply);
  return true;
}


//********************************************************************************
// Decode special characters in URL of get/post data
//********************************************************************************
String URLDecode(const char *src)
{
  String rString;
  const char* dst = src;
  char a, b;

  while (*src) {

    if (*src == '+')
    {
      rString += ' ';
      src++;
    }
    else
    {
      if ((*src == '%') &&
          ((a = src[1]) && (b = src[2])) &&
          (isxdigit(a) && isxdigit(b))) {
        if (a >= 'a')
          a -= 'a' - 'A';
        if (a >= 'A')
          a -= ('A' - 10);
        else
          a -= '0';
        if (b >= 'a')
          b -= 'a' - 'A';
        if (b >= 'A')
          b -= ('A' - 10);
        else
          b -= '0';
        rString += (char)(16 * a + b);
        src += 3;
      }
      else {
        rString += *src++;
      }
    }
  }
  return rString;
}

void addFormSelectorI2C(String& str, const String& id, int addressCount, const int addresses[], int selectedIndex)
{
  String options[addressCount];
  for (byte x = 0; x < addressCount; x++)
  {
    options[x] = F("0x");
    options[x] += String(addresses[x], HEX);
    if (x == 0)
      options[x] += F(" - (default)");
  }
  addFormSelector(str, F("I2C Address"), id, addressCount, options, addresses, NULL, selectedIndex, false);
}

void addFormSelector(String& str, const String& label, const String& id, int optionCount, const String options[], const int indices[], int selectedIndex)
{
  addFormSelector(str, label, id, optionCount, options, indices, NULL, selectedIndex, false);
}

void addFormSelector(String& str, const String& label, const String& id, int optionCount, const String options[], const int indices[], const String attr[], int selectedIndex, boolean reloadonchange)
{
  addRowLabel(str, label);
  addSelector(str, id, optionCount, options, indices, attr, selectedIndex, reloadonchange);
}

void addSelector(String& str, const String& id, int optionCount, const String options[], const int indices[], const String attr[], int selectedIndex, boolean reloadonchange)
{
  int index;

  str += F("<select name='");
  str += id;
  str += F("'");
  if (reloadonchange)
    str += F(" onchange=\"return dept_onchange(frmselect)\"");
  str += F(">");
  for (byte x = 0; x < optionCount; x++)
  {
    if (indices)
      index = indices[x];
    else
      index = x;
    str += F("<option value=");
    str += index;
    if (selectedIndex == index)
      str += F(" selected");
    if (attr)
    {
      str += F(" ");
      str += attr[x];
    }
    str += F(">");
    str += options[x];
    str += F("</option>");
  }
  str += F("</select>");
}


void addSelector_Head(String& str, const String& id, boolean reloadonchange)
{
  str += F("<select name='");
  str += id;
  str += F("'");
  if (reloadonchange)
    str += F(" onchange=\"return dept_onchange(frmselect)\"");
  str += F(">");
}

void addSelector_Item(String& str, const String& option, int index, boolean selected, boolean disabled, const String& attr)
{
  str += F("<option value=");
  str += index;
  if (selected)
    str += F(" selected");
  if (disabled)
    str += F(" disabled");
  if (attr && attr.length() > 0)
  {
    str += F(" ");
    str += attr;
  }
  str += F(">");
  str += option;
  str += F("</option>");
}

void addSelector_Foot(String& str)
{
  str += F("</select>");
}

void addUnit(String& str, const String& unit)
{
  str += F(" [");
  str += unit;
  str += F("]");
}


void addRowLabel(String& str, const String& label)
{
  str += F("<TR><TD>");
  str += label;
  str += F(":<TD>");
}

void addButton(String& str, const String &url, const String &label)
{
  str += F("<a class='button link' href='");
  str += url;
  str += F("'>");
  str += label;
  str += F("</a>");
}

void addSubmitButton(String& str)
{
  str += F("<input class='button link' type='submit' value='Submit'>");
}

//********************************************************************************
// Add a header
//********************************************************************************
void addFormHeader(String& str, const String& header1, const String& header2)
{
  str += F("<TR><TH>");
  str += header1;
  str += F("<TH>");
  str += header2;
  str += F("");
}

void addFormHeader(String& str, const String& header)
{
  str += F("<TR><TD colspan='2'><h2>");
  str += header;
  str += F("</h2>");
}


//********************************************************************************
// Add a sub header
//********************************************************************************
void addFormSubHeader(String& str, const String& header)
{
  str += F("<TR><TD colspan='2'><h3>");
  str += header;
  str += F("</h3>");
}


//********************************************************************************
// Add a note as row start
//********************************************************************************
void addFormNote(String& str, const String& text)
{
  str += F("<TR><TD><TD><div class='note'>Note: ");
  str += text;
  str += F("</div>");
}


//********************************************************************************
// Add a separator as row start
//********************************************************************************
void addFormSeparator(String& str)
{
  str += F("<TR><TD colspan='2'><hr>");
}


//********************************************************************************
// Add a checkbox
//********************************************************************************
void addCheckBox(String& str, const String& id, boolean checked)
{
  str += F("<input type=checkbox id='");
  str += id;
  str += F("' name='");
  str += id;
  str += F("'");
  if (checked)
    str += F(" checked");
  str += F(">");
}

void addFormCheckBox(String& str, const String& label, const String& id, boolean checked)
{
  addRowLabel(str, label);
  addCheckBox(str, id, checked);
}


//********************************************************************************
// Add a numeric box
//********************************************************************************
void addNumericBox(String& str, const String& id, int value, int min, int max)
{
  str += F("<input type='number' name='");
  str += id;
  str += F("'");
  if (min != INT_MIN)
  {
    str += F(" min=");
    str += min;
  }
  if (max != INT_MAX)
  {
    str += F(" max=");
    str += max;
  }
  str += F(" style='width:5em;' value=");
  str += value;
  str += F(">");
}

void addNumericBox(String& str, const String& id, int value)
{
  addNumericBox(str, id, value, INT_MIN, INT_MAX);
}

void addFormNumericBox(String& str, const String& label, const String& id, int value, int min, int max)
{
  addRowLabel(str,  label);
  addNumericBox(str, id, value, min, max);
}

void addFormNumericBox(String& str, const String& label, const String& id, int value)
{
  addFormNumericBox(str, label, id, value, INT_MIN, INT_MAX);
}



void addTextBox(String& str, const String& id, const String&  value, int maxlength)
{
  str += F("<input type='text' name='");
  str += id;
  str += F("' maxlength=");
  str += maxlength;
  str += F(" value='");
  str += value;
  str += F("'>");
}

void addFormTextBox(String& str, const String& label, const String& id, const String&  value, int maxlength)
{
  addRowLabel(str, label);
  addTextBox(str, id, value, maxlength);
}


void addFormPasswordBox(String& str, const String& label, const String& id, const String& password, int maxlength)
{
  addRowLabel(str, label);
  str += F("<input type='password' name='");
  str += id;
  str += F("' maxlength=");
  str += maxlength;
  str += F(" value='");
  if (password != F(""))   //no password?
    str += F("*****");
  //str += password;   //password will not published over HTTP
  str += F("'>");
}

void copyFormPassword(const String& id, char* pPassword, int maxlength)
{
  String password = WebServer.arg(id);
  if (password == F("*****"))   //no change?
    return;
  strncpy(pPassword, password.c_str(), maxlength);
}

void addFormIPBox(String& str, const String& label, const String& id, const byte ip[4])
{
  char strip[20];
  if (ip[0] == 0 && ip[1] == 0 && ip[2] == 0 && ip[3] == 0)
    strip[0] = 0;
  else
    sprintf_P(strip, PSTR("%u.%u.%u.%u"), ip[0], ip[1], ip[2], ip[3]);

  addRowLabel(str, label);
  str += F("<input type='text' name='");
  str += id;
  str += F("' value='");
  str += strip;
  str += F("'>");
}

// adds a Help Button with points to the the given Wiki Subpage
void addHelpButton(String& str, const String& url)
{
  str += F(" <a class=\"button help\" href=\"http://www.letscontrolit.com/wiki/index.php/");
  str += url;
  str += F("\" target=\"_blank\">&#10068;</a>");
}


void addEnabled(String& str, boolean enabled)
{
  if (enabled)
    str += F("<span class='enabled on'>&#10004;</span>");
  else
    str += F("<span class='enabled off'>&#10008;</span>");
}

bool isFormItemChecked(const String& id)
{
  return WebServer.arg(id) == "on";
}

int getFormItemInt(const String& id)
{
  String val = WebServer.arg(id);
  return val.toInt();
}

float getFormItemFloat(const String& id)
{
  String val = WebServer.arg(id);
  return val.toFloat();
}

bool isFormItem(const String& id)
{
  return (WebServer.arg(id).length() != 0);
}

