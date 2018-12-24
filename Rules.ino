/********************************************************************************************\
  Rules processing
  \*********************************************************************************************/
void rulesProcessing(String fileName, String& event)
{
  static uint8_t* data;
  static byte nestingLevel;

  String log = "";
  nestingLevel++;
  if (nestingLevel > RULES_MAX_NESTING_LEVEL)
  {
    log = F("EVENT: Error: Nesting level exceeded!");
    addLog(LOG_LEVEL_ERROR, log);
    nestingLevel--;
    return;
  }

  log = F("EVENT: ");
  log += event;
  addLog(LOG_LEVEL_INFO, log);

  if (data == NULL)
  {
    data = new uint8_t[RULES_MAX_SIZE];
    File f = SD.open(fileName);
    if (f)
    {
      byte *pointerToByteToRead = data;
      for (int x = 0; x < f.size(); x++)
      {
        *pointerToByteToRead = f.read();
        pointerToByteToRead++;// next byte
      }
      data[f.size()] = 0;
      f.close();
    }
    data[RULES_MAX_SIZE - 1] = 0; // make sure it's terminated!
  }

  int pos = 0;
  String line = "";
  boolean match = false;
  boolean codeBlock = false;
  boolean isCommand = false;
  boolean conditional = false;
  boolean condition = false;
  boolean ifBranche = false;

  while (data[pos] != 0)
  {
    if (data[pos] != 0 && data[pos] != 10)
      line += (char)data[pos];

    if (data[pos] == 10)    // if line complete, parse this rule
    {
      line.replace("\r", "");
      if (line.substring(0, 2) != "//" && line.length() > 0)
      {
        isCommand = true;

        int comment = line.indexOf("//");
        if (comment > 0)
          line = line.substring(0, comment);

        line = parseTemplate(line, line.length());
        line.trim();

        String lineOrg = line; // store original line for future use
        line.toLowerCase(); // convert all to lower case to make checks easier

        String eventTrigger = "";
        String action = "";

        if (!codeBlock)  // do not check "on" rules if a block of actions is to be processed
        {
          if (line.startsWith("on "))
          {
            line = line.substring(3);
            int split = line.indexOf(F(" do"));
            if (split != -1)
            {
              eventTrigger = line.substring(0, split);
              action = lineOrg.substring(split + 7);
              action.trim();
            }
            if (eventTrigger == "*") // wildcard, always process
              match = true;
            else
              match = ruleMatch(event, eventTrigger);
            if (action.length() > 0) // single on/do/action line, no block
            {
              isCommand = true;
              codeBlock = false;
            }
            else
            {
              isCommand = false;
              codeBlock = true;
            }
          }
        }
        else
        {
          action = lineOrg;
        }

        String lcAction = action;
        lcAction.toLowerCase();
        if (lcAction == "endon") // Check if action block has ended, then we will wait for a new "on" rule
        {
          isCommand = false;
          codeBlock = false;
          match = false;
        }

        if (match) // rule matched for one action or a block of actions
        {
          int split = lcAction.indexOf(F("if ")); // check for optional "if" condition
          if (split != -1)
          {
            conditional = true;
            String check = lcAction.substring(split + 3);
            condition = conditionMatch(check);
            ifBranche = true;
            isCommand = false;
          }

          if (lcAction == F("else")) // in case of an "else" block of actions, set ifBranche to false
          {
            ifBranche = false;
            isCommand = false;
          }

          if (lcAction == F("endif")) // conditional block ends here
          {
            conditional = false;
            isCommand = false;
          }

          // process the action if it's a command and unconditional, or conditional and the condition matches the if or else block.
          if (isCommand && ((!conditional) || (conditional && (condition == ifBranche))))
          {
            int equalsPos = event.indexOf("=");
            if (equalsPos > 0)
            {
              String tmpString = event.substring(equalsPos + 1);
              action.replace(F("%eventvalue%"), tmpString); // substitute %eventvalue% in actions with the actual value from the event
              action.replace(F("%event%"), event); // substitute %event% with literal event string
            }
            log = F("ACT  : ");
            log += action;
            addLog(LOG_LEVEL_INFO, log);
            ExecuteCommand(action.c_str());
          }
        }
      }

      line = "";
    }
    pos++;
  }

  nestingLevel--;
  if (nestingLevel == 0)
  {
    delete [] data;
    data = NULL;
  }
}


/********************************************************************************************\
  Check if an event matches to a given rule
  \*********************************************************************************************/
boolean ruleMatch(String& event, String& rule)
{
  boolean match = false;
  String tmpEvent = event;
  String tmpRule = rule;

  int pos = rule.indexOf('*');
  if (pos != -1) // a * sign in rule, so use a'wildcard' match on message
    {
      tmpEvent = event.substring(0, pos - 1);
      tmpRule = rule.substring(0, pos - 1);
      return tmpEvent.equalsIgnoreCase(tmpRule);
    }

  if (event.startsWith(F("Clock#Time"))) // clock events need different handling...
  {
    int pos1 = event.indexOf("=");
    int pos2 = rule.indexOf("=");
    if (pos1 > 0 && pos2 > 0)
    {
      tmpEvent = event.substring(0, pos1);
      tmpRule  = rule.substring(0, pos2);
      if (tmpRule.equalsIgnoreCase(tmpEvent)) // if this is a clock rule
      {
        tmpEvent = event.substring(pos1 + 1);
        tmpRule  = rule.substring(pos2 + 1);
        unsigned long clockEvent = string2TimeLong(tmpEvent);
        unsigned long clockSet = string2TimeLong(tmpRule);
        unsigned long Mask;
        for (byte y = 0; y < 8; y++)
        {
          if (((clockSet >> (y * 4)) & 0xf) == 0xf)  // if nibble y has the wildcard value 0xf
          {
            Mask = 0xffffffff  ^ (0xFUL << (y * 4)); // Mask to wipe nibble position y.
            clockEvent &= Mask;                      // clear nibble
            clockEvent |= (0xFUL << (y * 4));        // fill with wildcard value 0xf
          }
        }
        if (clockEvent == clockSet)
          return true;
        else
          return false;
      }
    }
  }


  // parse event into verb and value
  float value = 0;
  pos = event.indexOf("=");
  if (pos)
  {
    tmpEvent = event.substring(pos + 1);
    value = tmpEvent.toFloat();
    tmpEvent = event.substring(0, pos);
  }

  // parse rule
  int comparePos = 0;
  char compare = ' ';
  comparePos = rule.indexOf(">");
  if (comparePos > 0)
  {
    compare = '>';
  }
  else
  {
    comparePos = rule.indexOf("<");
    if (comparePos > 0)
    {
      compare = '<';
    }
    else
    {
      comparePos = rule.indexOf("=");
      if (comparePos > 0)
      {
        compare = '=';
      }
    }
  }

  float ruleValue = 0;

  if (comparePos > 0)
  {
    tmpRule = rule.substring(comparePos + 1);
    ruleValue = tmpRule.toFloat();
    tmpRule = rule.substring(0, comparePos);
  }

  switch (compare)
  {
    case '>':
      if (tmpRule.equalsIgnoreCase(tmpEvent) && value > ruleValue)
        match = true;
      break;

    case '<':
      if (tmpRule.equalsIgnoreCase(tmpEvent) && value < ruleValue)
        match = true;
      break;

    case '=':
      if (tmpRule.equalsIgnoreCase(tmpEvent) && value == ruleValue)
        match = true;
      break;

    case ' ':
      if (tmpRule.equalsIgnoreCase(tmpEvent))
        match = true;
      break;
  }

  return match;
}


/********************************************************************************************\
  Check expression
  \*********************************************************************************************/
boolean conditionMatch(String& check)
{
  boolean match = false;

  int comparePos = 0;
  char compare = ' ';
  comparePos = check.indexOf(">");
  if (comparePos > 0)
  {
    compare = '>';
  }
  else
  {
    comparePos = check.indexOf("<");
    if (comparePos > 0)
    {
      compare = '<';
    }
    else
    {
      comparePos = check.indexOf("=");
      if (comparePos > 0)
      {
        compare = '=';
      }
    }
  }

  float Value1 = 0;
  float Value2 = 0;

  if (comparePos > 0)
  {
    String tmpCheck = check.substring(comparePos + 1);
    Value2 = tmpCheck.toFloat();
    tmpCheck = check.substring(0, comparePos);
    Value1 = tmpCheck.toFloat();
  }
  else
    return false;

  switch (compare)
  {
    case '>':
      if (Value1 > Value2)
        match = true;
      break;

    case '<':
      if (Value1 < Value2)
        match = true;
      break;

    case '=':
      if (Value1 == Value2)
        match = true;
      break;
  }
  return match;
}


/********************************************************************************************\
  Check rule timers
  \*********************************************************************************************/
void rulesTimers()
{
  for (byte x = 0; x < RULES_TIMER_MAX; x++)
  {
    if (RulesTimer[x].Value != 0L) // timer active?
    {
      if (timeOutReached(RulesTimer[x].Value)) // timer finished?
      {
        RulesTimer[x].Value = 0L; // turn off this timer
        String event = F("Timer#");
        event += RulesTimer[x].Name;
        rulesProcessing(FILE_RULES, event);
      }
    }
  }
}
// Return the time difference as a signed value, taking into account the timers may overflow.
// Returned timediff is between -24.9 days and +24.9 days.
// Returned value is positive when "next" is after "prev"
#define ULONG_MAX  4294967295UL
long timeDiff(unsigned long prev, unsigned long next)
{
  long signed_diff = 0;
  // To cast a value to a signed long, the difference may not exceed half the ULONG_MAX
  const unsigned long half_max_unsigned_long = 2147483647u; // = 2^31 -1
  if (next >= prev) {
    const unsigned long diff = next - prev;
    if (diff <= half_max_unsigned_long) {
      // Normal situation, just return the difference.
      // Difference is a positive value.
      signed_diff = static_cast<long>(diff);
    } else {
      // prev has overflow, return a negative difference value
      signed_diff = static_cast<long>((ULONG_MAX - next) + prev + 1u);
      signed_diff = -1 * signed_diff;
    }
  } else {
    // next < prev
    const unsigned long diff = prev - next;
    if (diff <= half_max_unsigned_long) {
      // Normal situation, return a negative difference value
      signed_diff = static_cast<long>(diff);
      signed_diff = -1 * signed_diff;
    } else {
      // next has overflow, return a positive difference value
      signed_diff = static_cast<long>((ULONG_MAX - prev) + next + 1u);
    }
  }
  return signed_diff;
}

// Compute the number of milliSeconds passed since timestamp given.
// N.B. value can be negative if the timestamp has not yet been reached.
long timePassedSince(unsigned long timestamp) {
  return timeDiff(timestamp, millis());
}

// Check if a certain timeout has been reached.
boolean timeOutReached(unsigned long timer)
{
  const long passed = timePassedSince(timer);
  return passed >= 0;
}
