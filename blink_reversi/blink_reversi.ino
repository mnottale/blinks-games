/*   REVERSI
 *     OX
 *     XO
 *     
 *  2 Players, a lot of blinks
 *  
 * Setup:
 *   arrange the board the way you want
 *   long-click pieces to set their initial color 
 *   
 * Play:
 *   each player in turn clicks a piece (green single-clicks, red double-clicks) which will swap colors of
 *   surrounding pieces according to the reversi game rules, except on a hex griv!
 */
const byte CHECK = 1 << 1;
const byte CHECK2 = 1 << 2;
const byte C_OK = 1 << 3;
const byte C_FAIL = 1 << 4;

byte debouncing = 0;
int state = 0;

byte opposite(byte f)
{
  return (f+3)%6;
}
void setup() {
  // put your setup code here, to run once:
  setColor(OFF);
}
long checking = -1; // or check time start
int checkingColor = 0;

byte checkResultOk = 0;
byte checkResultFail = 0;

void loop() {
  // put your main code here, to run repeatedly:
  if (state == 0)
    setColor(OFF);
  else if (state == 1)
    setColor(GREEN);
  else if (state == 2)
    setColor(RED);
  if (buttonLongPressed())
      state = (state + 1)%3;
  int setC = 0;
  if (buttonSingleClicked())
    setC = 1;
  if (buttonDoubleClicked())
    setC = 2;
  if (setC)
  {
    checkingColor = setC;
    setValueSentOnAllFaces(CHECK + setC - 1);
    checking = millis();
  }
  for (byte f=0; f<6; ++f)
  {
    if (debouncing & (1 << f))
      continue;
    byte d = getLastValueReceivedOnFace(f);
    if (d)
      debouncing |= 1 << f;
    else
    {
      debouncing &= ~(1 << f);
      if (checking == -1)
        setValueSentOnFace(0, f);
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
        setValueSentOnFace(CHECK2, opposite(f));
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
        setValueSentOnFace(CHECK2, opposite(f));
      }
      else
      {
        setValueSentOnFace(C_OK, f);
      }
    }
    else if (d & C_OK)
    {
      setValueSentOnFace(0, f);
      state = cc + 1;
      if (checking == -1)
        setValueSentOnFace(C_OK, opposite(f));
      else
        checkResultOk |= 1 << f;
    }
    else if (d & C_FAIL)
    {
      setValueSentOnFace(0, f);
      state = cc + 1;
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
  }
}
