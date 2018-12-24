/********************************************************************************************\
  Get numerical Uservar by name
  \*********************************************************************************************/
float getNvar(String varName) {
  for (byte x = 0; x < USER_VAR_MAX; x++) {
    if (nUserVar[x].Name == varName) {
      return nUserVar[x].Value;
    }
  }
  return -99999;
}


/********************************************************************************************\
  Set numerical Uservar by name
  \*********************************************************************************************/
void setNvar(String varName, float value, int decimals) {
  int pos = -1;
  for (byte x = 0; x < USER_VAR_MAX; x++) {
    if (nUserVar[x].Name == varName) {
      nUserVar[x].Value = value;
      if (decimals != -1)
        nUserVar[x].Decimals = decimals;
      return;
    }
    if (pos == -1 && nUserVar[x].Name.length() == 0)
      pos = x;
  }
  if (pos != -1) {
    nUserVar[pos].Name = varName;
    nUserVar[pos].Value = value;
    if (decimals != -1)
      nUserVar[pos].Decimals = decimals;
    else
      nUserVar[pos].Decimals = 2;
  }
}


/********************************************************************************************\
  Set numerical Uservar Decimals by name
  \*********************************************************************************************/
void setNvarDecimals(String varName, byte decimals) {

  boolean found = false;
  for (byte x = 0; x < USER_VAR_MAX; x++) {
    if (nUserVar[x].Name == varName) {
      nUserVar[x].Decimals = decimals;
      found = true;
      break;
    }
  }
}


/********************************************************************************************\
  Get string Uservar by name
  \*********************************************************************************************/
String getSvar(String varName) {
  for (byte x = 0; x < USER_STRING_VAR_MAX; x++) {
    if (sUserVar[x].Name == varName) {
      return sUserVar[x].Value;
    }
  }
  return "";
}


/********************************************************************************************\
  Set string Uservar by name
  \*********************************************************************************************/
void setSvar(String varName, String value) {
  int pos = -1;
  for (byte x = 0; x < USER_STRING_VAR_MAX; x++) {
    if (sUserVar[x].Name == varName) {
      sUserVar[x].Value = value;
      return;
    }
    if (pos == -1 && sUserVar[x].Name.length() == 0)
      pos = x;
  }
  if (pos != -1) {
    sUserVar[pos].Name = varName;
    sUserVar[pos].Value = value;
  }
}


/********************************************************************************************\
  Set Timer by name
  \*********************************************************************************************/
void setTimer(String varName, unsigned long value) {

  int pos = -1;
  for (byte x = 0; x < RULES_TIMER_MAX; x++) {
    if (RulesTimer[x].Name == varName) {
      RulesTimer[x].Value = value;
      return;
    }
    if (pos == -1 && RulesTimer[x].Name.length() == 0)
      pos = x;
  }
  if (pos != -1) {
    RulesTimer[pos].Name = varName;
    RulesTimer[pos].Value = value;
  }
}


//********************************************************************************************
// Maintain node list
//********************************************************************************************
void nodelist(IPAddress remoteIP, String msg) {

  boolean found = false;
  for (byte x = 0; x < UNIT_MAX; x++) {
    if (Nodes[x].nodeName == msg) {
      for (byte y = 0; y < 4; y++)
        Nodes[x].IP[y] = remoteIP[y];
      Nodes[x].age = 0;
      found = true;
      break;
    }
  }
  if (!found) {
    for (byte x = 0; x < UNIT_MAX; x++) {
      if (Nodes[x].IP[0] == 0) {
        Nodes[x].nodeName = msg;
        for (byte y = 0; y < 4; y++)
          Nodes[x].IP[y] = remoteIP[y];
        Nodes[x].age = 0;
        break;
      }
    }
  }
}


/*********************************************************************************************\
   Refresh aging for remote units, drop if too old...
  \*********************************************************************************************/
void refreshNodeList()
{
  // start at 1, 0 = myself and does not age...
  for (byte counter = 1; counter < UNIT_MAX; counter++)
  {
    if (Nodes[counter].IP[0] != 0)
    {
      Nodes[counter].age++;  // increment age counter
      if (Nodes[counter].age > 10) // if entry to old, clear this node ip from the list.
        for (byte x = 0; x < 4; x++)
          Nodes[counter].IP[x] = 0;
    }
  }
}

#include <utility/W5100.h>
void ShowSocketStatus() {

  Serial.println("ETHERNET SOCKET LIST");
  Serial.println("#:Status Port Destination DPort");
  Serial.println("0=avail,14=waiting,17=connected,22=UDP,1C=close wait");
  String l_line = "";
  l_line.reserve(64);
  char l_buffer[10] = "";
  for (uint8_t i = 0; i < MAX_SOCK_NUM; i++) {
    l_line = "#" + String(i);
    uint8_t s = W5100.readSnSR(i); //status
    l_line += ":0x";
    sprintf(l_buffer,"%x",s);
    l_line += l_buffer;
    l_line += " ";
    l_line += String(W5100.readSnPORT(i)); //port
    l_line += " D:";
    uint8_t dip[4];
    W5100.readSnDIPR(i, dip); //IP Address
    for (int j=0; j<4; j++) {
      l_line += int(dip[j]);
      if (j<3) l_line += ".";
    }
    l_line += " (";
    l_line += String(W5100.readSnDPORT(i)); //port on destination
    l_line += ") ";
    Serial.println(l_line);
  }
}

void debugRAM(byte id, int objsize)
{
  Serial.print(F("Function:"));
  Serial.print(id);
  Serial.print(F(" RAM:"));
  Serial.print(FreeMem());
  Serial.print(F(" objsize:"));
  Serial.println(objsize);
}

void SelectSDCard(boolean sd)
  {
  digitalWrite(EthernetShield_CS_W5100, sd);
  digitalWrite(EthernetShield_CS_SDCard, !sd);
  }


/*********************************************************************************************\
   Workaround for removing trailing white space when String() converts a float with 0 decimals
  \*********************************************************************************************/
String toString(float value, byte decimals)
{
  String sValue = String(value, decimals);
  sValue.trim();
  return sValue;
}

/*********************************************************************************************\
  Parse a string and get the xth command or parameter
  \*********************************************************************************************/
String parseString(String& string, byte indexFind, char separator)
{
  String tmpString = string;
  tmpString += separator;
  tmpString.replace(" ", (String)separator);
  String locateString = "";
  byte count = 0;
  int index = tmpString.indexOf(separator);
  while (index > 0)
  {
    count++;
    locateString = tmpString.substring(0, index);
    tmpString = tmpString.substring(index + 1);
    index = tmpString.indexOf(separator);
    if (count == indexFind)
    {
      return locateString;
    }
  }
  return "";
}


/*********************************************************************************************\
   Parse a string and get the xth command or parameter
  \*********************************************************************************************/
int getParamStartPos(String& string, byte indexFind)
{
  String tmpString = string;
  byte count = 0;
  tmpString.replace(" ", ",");
  for (int x = 0; x < tmpString.length(); x++)
  {
    if (tmpString.charAt(x) == ',')
    {
      count++;
      if (count == (indexFind - 1))
        return x + 1;
    }
  }
  return -1;
}



/********************************************************************************************\
  Unsigned long Timer timeOut check
  \*********************************************************************************************/

boolean timeOut(unsigned long timer)
{
  // This routine solves the 49 day bug without the need for separate start time and duration
  //   that would need two 32 bit variables if duration is not static
  // It limits the maximum delay to 24.9 days.

  unsigned long now = millis();
  if (((now >= timer) && ((now - timer) < 1 << 31))  || ((timer >= now) && (timer - now > 1 << 31)))
    return true;

  return false;
}


/********************************************************************************************\
  delay in milliseconds with background processing
  \*********************************************************************************************/
void delayMillis(unsigned long delay)
{
  unsigned long timer = millis() + delay;
  while (millis() < timer);
    //backgroundtasks();
}


void fileSystemCheck()
{
  pinMode(EthernetShield_CS_SDCard, OUTPUT);
  pinMode(EthernetShield_CS_W5100, OUTPUT);
  SelectSDCard(true);
  
  if (SD.begin(EthernetShield_CS_SDCard))
  {
    String log = F("SD Mount successful");
    Serial.println(log);
    if (!SD.exists("boot.txt"))
    {

      String log = F("Creating boot.txt");
      Serial.println(log);
      File f = SD.open(F("boot.txt"), FILE_WRITE);
      
      log = F("Creating rules.txt");
      Serial.println(log);
      f = SD.open(F("rules.txt"), FILE_WRITE);
      f.close();
    }
  }
  else
  {
    String log = F("SD Mount failed");
    Serial.println(log);
  }
}


/********************************************************************************************\
  Find positional parameter in a char string
  \*********************************************************************************************/
boolean GetArgv(const char *string, char *argv, int argc)
{
  int string_pos = 0, argv_pos = 0, argc_pos = 0;
  char c, d;

  while (string_pos < strlen(string))
  {
    c = string[string_pos];
    d = string[string_pos + 1];

    if       (c == ' ' && d == ' ') {}
    else if  (c == ' ' && d == ',') {}
    else if  (c == ',' && d == ' ') {}
    else if  (c == ' ' && d >= 33 && d <= 126) {}
    else if  (c == ',' && d >= 33 && d <= 126) {}
    else
    {
      argv[argv_pos++] = c;
      argv[argv_pos] = 0;

      if (d == ' ' || d == ',' || d == 0)
      {
        argv[argv_pos] = 0;
        argc_pos++;

        if (argc_pos == argc)
        {
          return true;
        }

        argv[0] = 0;
        argv_pos = 0;
        string_pos++;
      }
    }
    string_pos++;
  }
  return false;
}


/********************************************************************************************\
  Convert a char string to integer
  \*********************************************************************************************/
unsigned long str2int(char *string)
{
  unsigned long temp = atof(string);
  return temp;
}


/********************************************************************************************\
  Convert a char string to IP byte array
  \*********************************************************************************************/
boolean str2ip(char *string, byte* IP)
{
  byte c;
  byte part = 0;
  int value = 0;

  for (int x = 0; x <= strlen(string); x++)
  {
    c = string[x];
    if (isdigit(c))
    {
      value *= 10;
      value += c - '0';
    }

    else if (c == '.' || c == 0) // next octet from IP address
    {
      if (value <= 255)
        IP[part++] = value;
      else
        return false;
      value = 0;
    }
    else if (c == ' ') // ignore these
      ;
    else // invalid token
      return false;
  }
  if (part == 4) // correct number of octets
    return true;
  return false;
}


/********************************************************************************************\
  Reset all settings to factory defaults
  \*********************************************************************************************/
void ResetFactory(void)
{
  SD.remove(F("boot.txt"));
  SD.remove(F("rules.txt"));
  File f = SD.open(F("boot.txt"), FILE_WRITE);
  f = SD.open(F("rules.txt"), FILE_WRITE);
  f.close();
  Reboot();
}


/********************************************************************************************\
  If RX and TX tied together, perform emergency reset to get the system out of boot loops
  \*********************************************************************************************/

void emergencyReset()
{
  // Direct Serial is allowed here, since this is only an emergency task.
  Serial.begin(115200);
  Serial.write(0xAA);
  Serial.write(0x55);
  delay(1);
  if (Serial.available() == 2)
    if (Serial.read() == 0xAA && Serial.read() == 0x55)
    {
      Serial.println(F("System will reset in 10 seconds..."));
      delay(10000);
      ResetFactory();
    }
}


/********************************************************************************************\
  Get free system mem
\*********************************************************************************************/
uint8_t *heapptr, *stackptr;

unsigned long FreeMem(void)
  {
  stackptr = (uint8_t *)malloc(4);        // use stackptr temporarily
  heapptr = stackptr;                     // save value of heap pointer
  free(stackptr);                         // free up the memory again (sets stackptr to 0)
  stackptr =  (uint8_t *)(SP);            // save value of stack pointer
  return (stackptr-heapptr);
}

/********************************************************************************************\
  In memory convert float to long
  \*********************************************************************************************/
unsigned long float2ul(float f)
{
  unsigned long ul;
  memcpy(&ul, &f, 4);
  return ul;
}


/********************************************************************************************\
  In memory convert long to float
  \*********************************************************************************************/
float ul2float(unsigned long ul)
{
  float f;
  memcpy(&f, &ul, 4);
  return f;
}


/********************************************************************************************\
  Add to log
  \*********************************************************************************************/
void addLog(byte loglevel, String& string)
{
  addLog(loglevel, string.c_str());
}

void addLog(byte loglevel, const char *line)
{
  if (Settings.UseSerial)
    if (loglevel <= Settings.SerialLogLevel)
      Serial.println(line);

  if (loglevel <= Settings.SDLogLevel)
  {
    File logFile = SD.open(F("log.txt"), FILE_WRITE);
    if (logFile)
      {
        String time = "";
        time += hour();
        time += ":";
        if (minute() < 10)
          time += "0";
        time += minute();
        time += ":";
        if (second() < 10)
          time += "0";
        time += second();
        time += F(" : ");
        logFile.print(time);
        logFile.println(line);
      }
    logFile.close();
  }
    
}


/********************************************************************************************\
  Delayed reboot, in case of issues, do not reboot with high frequency as it might not help...
  \*********************************************************************************************/
void delayedReboot(int rebootDelay)
{
  // Direct Serial is allowed here, since this is only an emergency task.
  while (rebootDelay != 0 )
  {
    Serial.print(F("Delayed Reset "));
    Serial.println(rebootDelay);
    rebootDelay--;
    delay(1000);
  }
  Reboot();
}


/********************************************************************************************\
  Convert a string like "Sun,12:30" into a 32 bit integer
  \*********************************************************************************************/
unsigned long string2TimeLong(String &str)
{
  // format 0000WWWWAAAABBBBCCCCDDDD
  // WWWW=weekday, AAAA=hours tens digit, BBBB=hours, CCCC=minutes tens digit DDDD=minutes

  char command[20];
  char TmpStr1[10];
  int w, x, y;
  unsigned long a;
  str.toLowerCase();
  str.toCharArray(command, 20);
  unsigned long lngTime = 0;

  if (GetArgv(command, TmpStr1, 1))
  {
    String day = TmpStr1;
    String weekDays = F("allsunmontuewedthufrisat");
    y = weekDays.indexOf(TmpStr1) / 3;
    if (y == 0)
      y = 0xf; // wildcard is 0xf
    lngTime |= (unsigned long)y << 16;
  }

  if (GetArgv(command, TmpStr1, 2))
  {
    y = 0;
    for (x = strlen(TmpStr1) - 1; x >= 0; x--)
    {
      w = TmpStr1[x];
      if (w >= '0' && w <= '9' || w == '*')
      {
        a = 0xffffffff  ^ (0xfUL << y); // create mask to clean nibble position y
        lngTime &= a; // maak nibble leeg
        if (w == '*')
          lngTime |= (0xFUL << y); // fill nibble with wildcard value
        else
          lngTime |= (w - '0') << y; // fill nibble with token
        y += 4;
      }
      else if (w == ':');
      else
      {
        break;
      }
    }
  }
  return lngTime;
}


/********************************************************************************************\
  Convert  a 32 bit integer into a string like "Sun,12:30"
  \*********************************************************************************************/
String timeLong2String(unsigned long lngTime)
{
  unsigned long x = 0;
  String time = "";

  x = (lngTime >> 16) & 0xf;
  if (x == 0x0f)
    x = 0;
  String weekDays = F("AllSunMonTueWedThuFriSat");
  time = weekDays.substring(x * 3, x * 3 + 3);
  time += ",";

  x = (lngTime >> 12) & 0xf;
  if (x == 0xf)
    time += "*";
  else if (x == 0xe)
    time += "-";
  else
    time += x;

  x = (lngTime >> 8) & 0xf;
  if (x == 0xf)
    time += "*";
  else if (x == 0xe)
    time += "-";
  else
    time += x;

  time += ":";

  x = (lngTime >> 4) & 0xf;
  if (x == 0xf)
    time += "*";
  else if (x == 0xe)
    time += "-";
  else
    time += x;

  x = (lngTime) & 0xf;
  if (x == 0xf)
    time += "*";
  else if (x == 0xe)
    time += "-";
  else
    time += x;

  return time;
}


/********************************************************************************************\
  Parse string template
  \*********************************************************************************************/
String parseTemplate(String &tmpString, byte lineSize)
{
  String newString = tmpString;

  // check named uservars
  for (byte x = 0; x < USER_VAR_MAX; x++) {
    String varname = "%" + nUserVar[x].Name + "%";
    String svalue = toString(nUserVar[x].Value, nUserVar[x].Decimals);
    newString.replace(varname, svalue);
  }

  // check named uservar strings
  for (byte x = 0; x < USER_STRING_VAR_MAX; x++) {
    String varname = "%" + sUserVar[x].Name + "%";
    String svalue = String(sUserVar[x].Value);
    newString.replace(varname, svalue);
  }

  newString.replace(F("%sysname%"), Settings.Name);
  newString.replace(F("%systime%"), getTimeString(':'));

  return newString;
}


String getTimeString(char delimiter)
{
  String reply;
  if (hour() < 10)
    reply += F("0");
  reply += String(hour());
  if (delimiter != '\0')
    reply += delimiter;
  if (minute() < 10)
    reply += F("0");
  reply += minute();
  return reply;
}


/********************************************************************************************\
  Calculate function for simple expressions
  \*********************************************************************************************/
#define CALCULATE_OK                            0
#define CALCULATE_ERROR_STACK_OVERFLOW          1
#define CALCULATE_ERROR_BAD_OPERATOR            2
#define CALCULATE_ERROR_PARENTHESES_MISMATCHED  3
#define CALCULATE_ERROR_UNKNOWN_TOKEN           4
#define STACK_SIZE 10 // was 50
#define TOKEN_MAX 20

float globalstack[STACK_SIZE];
float *sp = globalstack - 1;
float *sp_max = &globalstack[STACK_SIZE - 1];

#define is_operator(c)  (c == '+' || c == '-' || c == '*' || c == '/' || c == '^')

int push(float value)
{
  if (sp != sp_max) // Full
  {
    *(++sp) = value;
    return 0;
  }
  else
    return CALCULATE_ERROR_STACK_OVERFLOW;
}

float pop()
{
  if (sp != (globalstack - 1)) // empty
    return *(sp--);
}

float apply_operator(char op, float first, float second)
{
  switch (op)
  {
    case '+':
      return first + second;
    case '-':
      return first - second;
    case '*':
      return first * second;
    case '/':
      return first / second;
    case '^':
      return pow(first, second);
    default:
      return 0;
  }
}

char *next_token(char *linep)
{
  while (isspace(*(linep++)));
  while (*linep && !isspace(*(linep++)));
  return linep;
}

int RPNCalculate(char* token)
{
  if (token[0] == 0)
    return 0; // geen moeite doen voor een lege string

  if (is_operator(token[0]))
  {
    float second = pop();
    float first = pop();

    if (push(apply_operator(token[0], first, second)))
      return CALCULATE_ERROR_STACK_OVERFLOW;
  }
  else // Als er nog een is, dan deze ophalen
    if (push(atof(token))) // is het een waarde, dan op de stack plaatsen
      return CALCULATE_ERROR_STACK_OVERFLOW;

  return 0;
}

// operators
// precedence   operators         associativity
// 3            !                 right to left
// 2            * / %             left to right
// 1            + - ^             left to right
int op_preced(const char c)
{
  switch (c)
  {
    case '^':
      return 3;
    case '*':
    case '/':
      return 2;
    case '+':
    case '-':
      return 1;
  }
  return 0;
}

bool op_left_assoc(const char c)
{
  switch (c)
  {
    case '^':
    case '*':
    case '/':
    case '+':
    case '-':
      return true;     // left to right
      //case '!': return false;    // right to left
  }
  return false;
}

unsigned int op_arg_count(const char c)
{
  switch (c)
  {
    case '^':
    case '*':
    case '/':
    case '+':
    case '-':
      return 2;
      //case '!': return 1;
  }
  return 0;
}


int Calculate(const char *input, float* result)
{
  const char *strpos = input, *strend = input + strlen(input);
  char token[25];
  char c, *TokenPos = token;
  char stack[32];       // operator stack
  unsigned int sl = 0;  // stack length
  char     sc;          // used for record stack element
  int error = 0;

  //*sp=0; // bug, it stops calculating after 50 times
  sp = globalstack - 1;

  while (strpos < strend)
  {
    // read one token from the input stream
    c = *strpos;
    if (c != ' ')
    {
      // If the token is a number (identifier), then add it to the token queue.
      if ((c >= '0' && c <= '9') || c == '.')
      {
        *TokenPos = c;
        ++TokenPos;
      }

      // If the token is an operator, op1, then:
      else if (is_operator(c))
      {
        *(TokenPos) = 0;
        error = RPNCalculate(token);
        TokenPos = token;
        if (error)return error;
        while (sl > 0)
        {
          sc = stack[sl - 1];
          // While there is an operator token, op2, at the top of the stack
          // op1 is left-associative and its precedence is less than or equal to that of op2,
          // or op1 has precedence less than that of op2,
          // The differing operator priority decides pop / push
          // If 2 operators have equal priority then associativity decides.
          if (is_operator(sc) && ((op_left_assoc(c) && (op_preced(c) <= op_preced(sc))) || (op_preced(c) < op_preced(sc))))
          {
            // Pop op2 off the stack, onto the token queue;
            *TokenPos = sc;
            ++TokenPos;
            *(TokenPos) = 0;
            error = RPNCalculate(token);
            TokenPos = token;
            if (error)return error;
            sl--;
          }
          else
            break;
        }
        // push op1 onto the stack.
        stack[sl] = c;
        ++sl;
      }
      // If the token is a left parenthesis, then push it onto the stack.
      else if (c == '(')
      {
        stack[sl] = c;
        ++sl;
      }
      // If the token is a right parenthesis:
      else if (c == ')')
      {
        bool pe = false;
        // Until the token at the top of the stack is a left parenthesis,
        // pop operators off the stack onto the token queue
        while (sl > 0)
        {
          *(TokenPos) = 0;
          error = RPNCalculate(token);
          TokenPos = token;
          if (error)return error;
          sc = stack[sl - 1];
          if (sc == '(')
          {
            pe = true;
            break;
          }
          else
          {
            *TokenPos = sc;
            ++TokenPos;
            sl--;
          }
        }
        // If the stack runs out without finding a left parenthesis, then there are mismatched parentheses.
        if (!pe)
          return CALCULATE_ERROR_PARENTHESES_MISMATCHED;

        // Pop the left parenthesis from the stack, but not onto the token queue.
        sl--;

        // If the token at the top of the stack is a function token, pop it onto the token queue.
        if (sl > 0)
          sc = stack[sl - 1];

      }
      else
        return CALCULATE_ERROR_UNKNOWN_TOKEN;
    }
    ++strpos;
  }
  // When there are no more tokens to read:
  // While there are still operator tokens in the stack:
  while (sl > 0)
  {
    sc = stack[sl - 1];
    if (sc == '(' || sc == ')')
      return CALCULATE_ERROR_PARENTHESES_MISMATCHED;

    *(TokenPos) = 0;
    error = RPNCalculate(token);
    TokenPos = token;
    if (error)return error;
    *TokenPos = sc;
    ++TokenPos;
    --sl;
  }

  *(TokenPos) = 0;
  error = RPNCalculate(token);
  TokenPos = token;
  if (error)
  {
    *result = 0;
    return error;
  }
  *result = *sp;
  return CALCULATE_OK;
}


/********************************************************************************************\
  Time stuff
  \*********************************************************************************************/

#define SECS_PER_MIN  (60UL)
#define SECS_PER_HOUR (3600UL)
#define SECS_PER_DAY  (SECS_PER_HOUR * 24UL)
#define DAYS_PER_WEEK (7UL)
#define SECS_PER_WEEK (SECS_PER_DAY * DAYS_PER_WEEK)
#define SECS_PER_YEAR (SECS_PER_WEEK * 52UL)
#define SECS_YR_2000  (946684800UL) // the time at the start of y2k
#define LEAP_YEAR(Y)     ( ((1970+Y)>0) && !((1970+Y)%4) && ( ((1970+Y)%100) || !((1970+Y)%400) ) )

struct  timeStruct {
  uint8_t Second;
  uint8_t Minute;
  uint8_t Hour;
  uint8_t Wday;   // day of week, sunday is day 1
  uint8_t Day;
  uint8_t Month;
  uint8_t Year;   // offset from 1970;
} tm;

uint32_t syncInterval = 3600;  // time sync will be attempted after this many seconds
uint32_t sysTime = 0;
uint32_t prevMillis = 0;
uint32_t nextSyncTime = 0;

byte PrevMinutes = 0;

void breakTime(unsigned long timeInput, struct timeStruct &tm) {
  uint8_t year;
  uint8_t month, monthLength;
  uint32_t time;
  unsigned long days;
  const uint8_t monthDays[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

  time = (uint32_t)timeInput;
  tm.Second = time % 60;
  time /= 60; // now it is minutes
  tm.Minute = time % 60;
  time /= 60; // now it is hours
  tm.Hour = time % 24;
  time /= 24; // now it is days
  tm.Wday = ((time + 4) % 7) + 1;  // Sunday is day 1

  year = 0;
  days = 0;
  while ((unsigned)(days += (LEAP_YEAR(year) ? 366 : 365)) <= time) {
    year++;
  }
  tm.Year = year; // year is offset from 1970

  days -= LEAP_YEAR(year) ? 366 : 365;
  time  -= days; // now it is days in this year, starting at 0

  days = 0;
  month = 0;
  monthLength = 0;
  for (month = 0; month < 12; month++) {
    if (month == 1) { // february
      if (LEAP_YEAR(year)) {
        monthLength = 29;
      } else {
        monthLength = 28;
      }
    } else {
      monthLength = monthDays[month];
    }

    if (time >= monthLength) {
      time -= monthLength;
    } else {
      break;
    }
  }
  tm.Month = month + 1;  // jan is month 1
  tm.Day = time + 1;     // day of month
}

void setTime(unsigned long t) {
  sysTime = (uint32_t)t;
  nextSyncTime = (uint32_t)t + syncInterval;
  prevMillis = millis();  // restart counting from now (thanks to Korman for this fix)
}

unsigned long now() {
  // calculate number of seconds passed since last call to now()
  while (millis() - prevMillis >= 1000) {
    // millis() and prevMillis are both unsigned ints thus the subtraction will always be the absolute value of the difference
    sysTime++;
    prevMillis += 1000;
  }
  if (nextSyncTime <= sysTime) {
    unsigned long  t = getNtpTime();
    if (t != 0) {
      if (Settings.DST)
        t += SECS_PER_HOUR; // add one hour if DST active
      setTime(t);
    } else {
      nextSyncTime = sysTime + syncInterval;
    }
  }
  breakTime(sysTime, tm);
  return (unsigned long)sysTime;
}

int hour()
{
  return tm.Hour;
}

int minute()
{
  return tm.Minute;
}

int second()
{
  return tm.Second;
}

int weekday()
{
  return tm.Wday;
}

void initTime()
{
  nextSyncTime = 0;
  now();
}

void checkTime()
{
  now();
  if (tm.Minute != PrevMinutes)
  {
    PluginCall(PLUGIN_CLOCK_IN, dummyString, dummyString);
    PrevMinutes = tm.Minute;
    if (Settings.UseRules)
    {
      String weekDays = F("AllSunMonTueWedThuFriSat");
      String event = F("Clock#Time=");
      event += weekDays.substring(weekday() * 3, weekday() * 3 + 3);
      event += ",";
      if (hour() < 10)
        event += "0";
      event += hour();
      event += ":";
      if (minute() < 10)
        event += "0";
      event += minute();
      rulesProcessing(FILE_RULES, event);
    }
  }
}


unsigned long getNtpTime()
{
  const char* ntpServerName = "pool.ntp.org";
  unsigned long result=0;
  
  IPAddress timeServerIP;

  int ret = 0;
  DNSClient dns;
  dns.begin(Ethernet.dnsServerIP());
  #if socketdebug
    ShowSocketStatus();
  #endif
  ret = dns.getHostByName(ntpServerName, timeServerIP);
  portUDP.begin(Settings.Port);  // re-use UDP socket for system packets if it was used before
  #if socketdebug
    ShowSocketStatus();
  #endif

  if (ret){
    for (byte x = 1; x < 4; x++)
    {
      String log = F("NTP  : NTP sync request:");
      log += x;
      addLog(LOG_LEVEL_DEBUG_MORE, log);

      const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
      byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

      memset(packetBuffer, 0, NTP_PACKET_SIZE);
      packetBuffer[0] = 0b11100011;   // LI, Version, Mode
      packetBuffer[1] = 0;     // Stratum, or type of clock
      packetBuffer[2] = 6;     // Polling Interval
      packetBuffer[3] = 0xEC;  // Peer Clock Precision
      packetBuffer[12]  = 49;
      packetBuffer[13]  = 0x4E;
      packetBuffer[14]  = 49;
      packetBuffer[15]  = 52;

      portUDP.beginPacket(timeServerIP, 123); //NTP requests are to port 123
      portUDP.write(packetBuffer, NTP_PACKET_SIZE);
      portUDP.endPacket();

      uint32_t beginWait = millis();
      while (millis() - beginWait < 2000) {
        int size = portUDP.parsePacket();
        if (size == NTP_PACKET_SIZE && portUDP.remotePort() == 123) {
          portUDP.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
          unsigned long secsSince1900;
          // convert four bytes starting at location 40 to a long integer
          secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
          secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
          secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
          secsSince1900 |= (unsigned long)packetBuffer[43];
          log = F("NTP  : NTP replied: ");
          log += millis() - beginWait;
          log += F(" mSec");
          addLog(LOG_LEVEL_DEBUG_MORE, log);
          result = secsSince1900 - 2208988800UL + Settings.TimeZone * SECS_PER_MIN;
          break; // exit wait loop
        }
      }
      if (result)
        break; // exit for loop
      log = F("NTP  : No reply");
      addLog(LOG_LEVEL_DEBUG_MORE, log);
    } // for
  } // ret
  return result;
}


