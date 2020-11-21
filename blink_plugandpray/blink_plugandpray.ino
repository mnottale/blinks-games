/* DRILLERS
 * 
 * 2-3 players, enough blinks
 * 
 * Lore:
 * Each player starts with 12 mining drills, and choses how many to use each turn. Use other people's drills to mine further!
 * 
 * Setup:
 *   Set one blink apart per player (the driller), connect remaining blinks together in any shape you want.
 *   Long-click your driller  multiple times until it shows your color, one per player.
 *   Pick a rule for turn order (recomended: first player changes every turn).
 *
 * Turn:
 *   Phase 1: each player single-clicks his driller to enter drill selection mode, and click to select between
 *     1 and 4 drills to use. Double-click when done to enter insertion mode.
 *   Phase 2, insertion: each player in turn picks an insertion point in the board
 *     and plug his driller there (white face facing the insertion point). You can plug the board directly, or the driller
 *     of an other player who already played this turn, which will drill one blink deeper in the direction you've chosen.
 *     Drilling will transfer zones to you, in that order, until all drills are spent:
 *       - 2 empty zone on front-facing side per drill
 *       - 1 empty zone on back-facing side per drill
 *       - 1 other-player-occupied front-facing side per drill
 *       - 1 other player occupied back-facing side per 2 drills
 *     When all players have played, remove then double-click your insertion blink to show how many darts you have remaining.
 *     Then start next turn.
 * End:
 *   When all drills are spent, the player with most zones in his color wins.
 * Reset:
 *   Link all blinks together and double-click a board blink to reset everything.
 */
byte boardPart = 1; // board or dart
byte booting = 1;
byte wiping = 0;
unsigned long wipeTime = 0;
unsigned long bootTime = 0;

byte pendingDG[4];
byte pendingSendFace = -1;
unsigned long lastSent = 0; // last sent time

// board state
byte boardState[6] = {0,0,0,0,0,0};
byte debouncing[6] = {0,0,0,0,0,0};
// dart state
byte color = 0; // 0-2: player color
byte dartsRemaining = 12;
byte dartMode = 0; // 0 display remaining 1 picking 2 inserting
byte dartsPicked = 0;
// comm debounce
byte lastMessage[3] = {0,0,0};
byte lastMessageSent = 0;

const Color colors[] = {OFF, RED, GREEN, BLUE};

const byte P_ACCEPT_DRILL = 0x1;
const byte P_GOT_DRILL = 0x2;
const byte P_RESET = 0x3;
const byte P_WIPE = 0x4;

void setup() {
  // put your setup code here, to run once:
  setValueSentOnAllFaces(P_ACCEPT_DRILL);
  boardPart = 1;
  pendingSendFace = -1;
  dartsRemaining = 12;
  booting = 1;
  bootTime = millis();
}

void displayState()
{
  if (wiping)
  {
    for (byte f = 0; f < 6; ++f)
    {
      setColorOnFace(WHITE, f);
    }
    return;
  }
  if (booting)
  {
    if (millis()-bootTime > 1000)
      booting = 0;
    else
    {
      for (byte f = 0; f < 6; ++f)
      {
        setColorOnFace(WHITE, f);
      }
      return;
    }
  }
  if (boardPart)
  {
    for (byte f = 0; f < 6; ++f)
    {
      setColorOnFace(colors[boardState[f]], f);
    }
  }
  else
  {
    if (dartMode == 0)
    {
      byte count = dartsRemaining;
      for (byte f = 0; f < 6; ++f)
      {
        if (count >= 2)
        {
          setColorOnFace(colors[color+1], f);
          count -= 2;
        }
        else if (count == 1)
        {
          setColorOnFace(dim(colors[color+1], 127), f);
          count --;
        }
        else
          setColorOnFace(OFF, f);
      }
    }
    else if (dartMode == 1)
    {
      byte count = dartsPicked;
      for (byte f = 0; f < 6; ++f)
      {
        if (count > 0)
        {
          setColorOnFace(colors[color+1], f);
          --count;
        }
        else
          setColorOnFace(OFF, f);
      }
    }
    else if (dartMode == 2)
    {
      setColorOnFace(WHITE, 0);
      setColorOnFace(OFF, 1);
      setColorOnFace(OFF, 5);
      switch (dartsPicked)
      {
        case 1:
          setColorOnFace(colors[color+1], 3);
          setColorOnFace(OFF, 2);
          setColorOnFace(OFF, 4);
          break;
        case 2:
          setColorOnFace(colors[color+1], 2);
          setColorOnFace(colors[color+1], 4);
          setColorOnFace(OFF, 3);
          break;
        case 3:
          setColorOnFace(colors[color+1], 2);
          setColorOnFace(colors[color+1], 4);
          setColorOnFace(colors[color+1], 3);
          break;
        case 4:
          setColorOnFace(dim(colors[color+1], 127), 2);
          setColorOnFace(dim(colors[color+1], 127), 4);
          setColorOnFace(colors[color+1], 3);
      }
    }
  }
  if (pendingSendFace != -1)
    setColorOnFace(WHITE, pendingSendFace);
}

void checkDrill(byte face, byte drillColor, byte* drillAmount, byte cost, byte phase2)
{
  drillColor += 1;
  if (*drillAmount < cost)
    return;
  if (boardState[face] == 0)
  {
    boardState[face] = drillColor;
    *drillAmount -= cost;
  }
  else if (phase2 && boardState[face] != drillColor && *drillAmount >= cost*2)
  {
    boardState[face] = drillColor;
    *drillAmount -= cost*2;
  }
}
void doDrill(byte face, byte drillColor, byte drillAmount)
{
  drillAmount *= 2;
  checkDrill((face+0)%6, drillColor, &drillAmount, 1, 0);
  checkDrill((face+1)%6, drillColor, &drillAmount, 1, 0);
  checkDrill((face+5)%6, drillColor, &drillAmount, 1, 0);
  checkDrill((face+3)%6, drillColor, &drillAmount, 2, 0);
  checkDrill((face+4)%6, drillColor, &drillAmount, 2, 0);
  checkDrill((face+2)%6, drillColor, &drillAmount, 2, 0);
  checkDrill((face+0)%6, drillColor, &drillAmount, 1, 1);
  checkDrill((face+1)%6, drillColor, &drillAmount, 1, 1);
  checkDrill((face+5)%6, drillColor, &drillAmount, 1, 1);
  checkDrill((face+3)%6, drillColor, &drillAmount, 2, 1);
  checkDrill((face+4)%6, drillColor, &drillAmount, 2, 1);
  checkDrill((face+2)%6, drillColor, &drillAmount, 2, 1);
}
void processDatagram(const byte* data, byte face)
{
  byte fw1 = data[0];
  byte fw2 = data[1];
  byte drill = data[2];
  byte idx = data[3];
  byte col = drill >> 4;
  if (idx <= lastMessage[col])
    return;
  lastMessage[col] = idx;
  if (boardPart)
  {
    if (fw1 == 0xFF && fw2 == 0xFF)
    {
      doDrill(face, drill >> 4, (drill &0xF)+1);
    }
    else
    {
      pendingDG[0] = (fw2 == 0xFF) ? 0xFF : (fw2+face)%6;
      pendingDG[1] = 0xFF;
      pendingDG[2] = drill;
      pendingDG[3] = idx;
      pendingSendFace = (fw1+face)%6;
    }
  }
  else
  {
    pendingDG[0] = face;
    pendingDG[1] = (fw1 == 0xFF) ? 0xFF : (fw1+face)%6;
    pendingDG[2] = drill;
    pendingDG[3] = idx;
    pendingSendFace = 0;
  }
}
void doComm()
{
  if (pendingSendFace != -1)
  {
    byte recv = getLastValueReceivedOnFace(pendingSendFace);
    if ((recv == P_ACCEPT_DRILL || recv == P_RESET) && millis()-lastSent > 300)
    {
      lastSent = millis();
      sendDatagramOnFace(pendingDG, 4, pendingSendFace);
    }
    else if (recv == P_GOT_DRILL)
    {
      setValueSentOnFace(P_RESET, pendingSendFace);
      pendingSendFace = -1;
    }
  }
  
  for (byte f=0; f<6; ++f)
  {
    if (getLastValueReceivedOnFace(f) == P_WIPE && millis() - wipeTime > 8000)
    {
      wipe();
      return;
    }
    if (debouncing[f])
    {
      if (getLastValueReceivedOnFace(f) == P_RESET)
      {
        debouncing[f] = 0;
        setValueSentOnFace(P_ACCEPT_DRILL, f);
      }
      continue;
    }
    if (isDatagramReadyOnFace(f))
    {
      const byte* drillReq = getDatagramOnFace(f);
      processDatagram(drillReq, f);
      markDatagramReadOnFace(f);
      debouncing[f] = 1;
      setValueSentOnFace(P_GOT_DRILL, f);
    }
  }
}
void wipe() {
 setValueSentOnAllFaces(P_WIPE);
 wipeTime = millis();
 wiping = 1;
 for (byte f=0; f<6; ++f)
   boardState[f] = 0;
 dartsRemaining = 12;
}

void loop() {
  // put your main code here, to run repeatedly:
  if (wiping && millis() - wipeTime > 2000)
  {
    wiping = 0;
    setValueSentOnAllFaces(P_ACCEPT_DRILL);
  }
  if (buttonLongPressed())
  {
    boardPart = 0;
    if (dartsRemaining == 0)
      dartsRemaining = 12;
    else
      color = (color + 1)%3;
  }
  if (buttonSingleClicked() && !boardPart)
  {
    if (dartMode != 1)
    {
      dartMode = 1;
      dartsPicked = 0;
    }
    ++dartsPicked;
    if (dartsPicked > 4)
      dartsPicked = 1;
  }
  if (buttonDoubleClicked())
  {
    if (!boardPart)
    {
      dartMode = (dartMode + 1)%3;
      if (dartMode == 2)
      {
        pendingDG[0] = 0xFF;
        pendingDG[1] = 0xFF;
        pendingDG[2] = (color << 4) + (dartsPicked-1);
        pendingDG[3] = ++lastMessageSent;
        pendingSendFace = 0;
        dartsRemaining -= dartsPicked;
      }
    }
    else
    {
      wipe();
    }
  }
  displayState();
  doComm();
}
