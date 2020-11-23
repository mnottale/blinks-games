/*   REVERSI
 *     OX
 *     XO
 *     
 *  2 Players, a lot of blinks
 *  
 * Setup:
 *   arrange the board the way you want
 *   long-click pieces to set their initial color(red or green)
 *   
 * Play:
 *   each player in turn clicks a piece (green single-clicks, red double-clicks) which will swap colors of
 *   surrounding pieces according to the reversi game rules, except on a hex griv!
 */
const byte CHECK = 1 << 1;
const byte CHECK2 = 1 << 2;
const byte C_OK = 1 << 3;
const byte C_FAIL = 1 << 4;

byte received = 0; // bitmask
byte emiting = 0;  // bitmask

void setb(byte* bm, byte b)
{
  (*bm) |= 1 << b;
}
void clearb(byte* bm, byte b)
{
  (*bm) &= ~(1<<b);
}
byte getb(byte* bm, byte b)
{
  return *bm & (1<<b);
}

int state = 0;

byte opposite(byte f)
{
  return (f+3)%6;
}
byte booting = 0;
unsigned long bootTime = 0;

void setup() {
  // put your setup code here, to run once:
  setColor(WHITE);
  booting = 1;
  bootTime = millis();
}
long checking = -1; // or check time start
int checkingColor = 0;

byte checkResultOk = 0;
byte checkResultFail = 0;

byte clickColor = 0;
unsigned long clickTime = 0;
void loop() {
  if (booting)
  {
    if (millis()-bootTime > 1000)
      booting = 0;
    return;
  }
  if (clickColor != 0)
  {
    if (millis()-clickTime > 600)
    {
      clickColor = 0;
    }
    setColor(OFF);
    setColorOnFace((clickColor == 1) ? GREEN : RED, ((millis()-clickTime)/40)%6);
  }
  else
  {
    if (state == 0)
      setColor(OFF);
    else if (state == 1)
      setColor(GREEN);
    else if (state == 2)
      setColor(RED);
  }
  if (buttonLongPressed())
      state = (state + 1)%3;
  int setC = 0;
  if (buttonSingleClicked())
    setC = 1;
  if (buttonDoubleClicked())
    setC = 2;
  if (setC)
  {
    clickTime = millis();
    clickColor = setC;
    checkingColor = setC;
    setValueSentOnAllFaces(CHECK + (setC - 1));
    emiting = 63;
    checking = millis();
  }
  for (byte f=0; f<6; ++f)
  {
    byte d = getLastValueReceivedOnFace(f);
    if (d && getb(&received, f))
      continue;
    if (!d)
      clearb(&received, f);
    else
      setb(&received, f);
    if (!d && !getb(&emiting, f))
    {
      setValueSentOnFace(0, f);
      continue;
    }
    byte cc = d & 1;
    if (d & CHECK)
    {
      if (state == 0 || state == cc+1 || isValueReceivedOnFaceExpired(opposite(f)))
      {//fail
        setValueSentOnFace(C_FAIL, f);
      }
      else
      {//propagate
        setb(&emiting, opposite(f));
        setValueSentOnFace(CHECK2 | cc, opposite(f));
      }
    }
    else if (d & CHECK2)
    {
      if (state == 0 || (state != cc+1 && isValueReceivedOnFaceExpired(opposite(f))))
      {
        setValueSentOnFace(C_FAIL, f);
      }
      else if (state != cc+1)
      {
        setb(&emiting, opposite(f));
        setValueSentOnFace(CHECK2 | cc, opposite(f));
      }
      else
      {
        setValueSentOnFace(C_OK | cc, f);
      }
    }
    else if (d & C_OK)
    {
      clearb(&emiting, f);
      setValueSentOnFace(0, f);
      state = cc + 1;
      if (checking == -1)
        setValueSentOnFace(C_OK |cc, opposite(f));
      else
        checkResultOk |= 1 << f;
    }
    else if (d & C_FAIL)
    {
      clearb(&emiting, f);
      setValueSentOnFace(0, f);
      if (checking == -1)
        setValueSentOnFace(C_FAIL, opposite(f));
      else
        checkResultFail |= 1 << f;
    }
  }
  if (checking != -1 && (millis() - checking > 1000 || (checkResultOk | checkResultFail) == 63))
  {
    checking = -1;
    setValueSentOnAllFaces(0);
    emiting = 0;
  }
}
