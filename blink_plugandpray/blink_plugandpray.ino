/* DRILLERS
 * 
 * 2-3 players, enough blinks
 * 
 * Lore:
 * Each player starts with 12 mining dart, and choses how many to use each turn. Use other people's darts to mine further!
 * 
 * Setup:
 *   Set one blink apart per player (the insertion blinks).
 *   Long-click it multiple times until it shows your color, one per player.
 *
 * Turn:
 *   Phase 1: each player single-clicks his driller to enter dart selection mode, and click to select between
 *     1 and 4 darts to use. Double-click when done to enter insertion mode.
 *   Phase 2, insertion: in ascending order of number of darts, each player picks an insertion point in the cluster
 *     and plug his dart there (white border facing the cluster). You can plug the cluster directly, or the dart
 *     of an other player, which will drill one blink deeper in the direction you've chosen.
 *     Drilling will transfer zones to you, in that order, until all points are spent:
 *       - 2 empty zone on front-facing side per dart point
 *       - 1 empty zone on back-facing side per point
 *       - 1 other-player-occupied front-facing side per 2 points
 *       - 1 other player occupied back-facing side per 4 points
 *     When all players are played, remove then double-click your insertion blink to show how many darts you have remaining.
 *     Then start next turn.
 */
byte boardPart = 1; // board or dart

byte pendingDG[3];
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

const Color colors[] = {OFF, RED, GREEN, BLUE};

const byte P_ACCEPT_DRILL = 0x1;
const byte P_GOT_DRILL = 0x2;
const byte P_RESET = 0x3;

void setup() {
  // put your setup code here, to run once:
  setValueSentOnAllFaces(P_ACCEPT_DRILL);
}

void displayState()
{
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

void checkDrill(byte face, byte drillColor, byte& drillAmount, byte cost, byte phase2)
{
  if (drillAmount < cost)
    return;
  if (boardState[face] == 0)
  {
    boardState[face] = drillColor;
    drillAmount -= cost;
  }
  else if (phase2 && boardState[face] != drillColor && drillAmount >= cost*2)
  {
    boardState[face] = drillColor;
    drillAmount -= cost*2;
  }
}
void doDrill(byte face, byte drillColor, byte drillAmount)
{
  drillAmount *= 2;
  checkDrill((face+0)%6, drillColor, drillAmount, 1, 0);
  checkDrill((face+1)%6, drillColor, drillAmount, 1, 0);
  checkDrill((face+5)%6, drillColor, drillAmount, 1, 0);
  checkDrill((face+3)%6, drillColor, drillAmount, 2, 0);
  checkDrill((face+4)%6, drillColor, drillAmount, 2, 0);
  checkDrill((face+2)%6, drillColor, drillAmount, 2, 0);
  checkDrill((face+0)%6, drillColor, drillAmount, 1, 1);
  checkDrill((face+1)%6, drillColor, drillAmount, 1, 1);
  checkDrill((face+5)%6, drillColor, drillAmount, 1, 1);
  checkDrill((face+3)%6, drillColor, drillAmount, 2, 1);
  checkDrill((face+4)%6, drillColor, drillAmount, 2, 1);
  checkDrill((face+2)%6, drillColor, drillAmount, 2, 1);
}
void processDatagram(const byte* data, byte face)
{
  byte fw1 = data[0];
  byte fw2 = data[1];
  byte drill = data[2];
  if (boardPart)
  {
    if (fw1 == 0xFF && fw2 == 0xFF)
    {
      doDrill(face, drill >> 4, (drill &0xF)+1);
    }
    else
    {
      pendingDG[0] = fw2;
      pendingDG[1] = 0xFF;
      pendingDG[2] = drill;
      pendingSendFace = fw1;
    }
  }
  else
  {
    pendingDG[0] = (face + 3)%6;
    pendingDG[1] = fw1;
    pendingDG[2] = drill;
    pendingSendFace = 0;
  }
}
void doComm()
{
  if (pendingSendFace != -1)
  {
    byte recv = getLastValueReceivedOnFace(pendingSendFace);
    if ((recv == P_ACCEPT_DRILL || recv == P_RESET) && millis()-lastSent > 30)
    {
      lastSent = millis();
      sendDatagramOnFace(pendingDG, 3, pendingSendFace);
    }
    else if (recv == P_GOT_DRILL)
    {
      setValueSentOnFace(P_RESET, pendingSendFace);
      pendingSendFace = -1;
    }
  }
  
  for (byte f=0; f<6; ++f)
  {
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
void loop() {
  // put your main code here, to run repeatedly:
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
  if (buttonDoubleClicked() && !boardPart)
  {
    dartMode = (dartMode + 1)%3;
    if (dartMode == 2)
    {
      pendingDG[0] = 0xFF;
      pendingDG[1] = 0xFF;
      pendingDG[2] = (color << 4) + dartsPicked-1;
      pendingSendFace = 0;
      dartsRemaining -= dartsPicked;
    }
  }
  displayState();
  doComm();
}
