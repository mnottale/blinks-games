/*
 JUMP
   2-4 players
   6+ blinks

   Move the jumpers around the board and jump on your oponents until you're the last one standing.

   Setup:
     Assemble the blinks in any way you like. Each player choses a controlling blink and long-press it
     until it displays his color.
   Play:
     Click once your controlling blink to have your jumper move in the direction of the white arrow.
     If there was an other jumper there, it got destroyed, congratulation.
   Optional extra rule:
     At any time, double-click your controller blink to bunker your jumper. If anyone jumps your jumper
     while bunkered it will be destroyed instead of you. Bunkering lasts 3 seconds, and can only be
     done once every 30 seconds. Your jumper cannot move while bunkered.
   Reset:
     Triple-click a controlling blink to spawn a jumper again.
     Trilple-click any other blink to remove the jumper there.

 */
// all blinks state
byte occupied = 0;
byte facing = 0;
unsigned long facingSince = 0;
unsigned long bunkerSince = 0;

// control state
byte controlling = 0;
unsigned long lastBunkerOrder = 0;

// debounce times
unsigned long jumpSendDebounce[6];
unsigned long jumpReceiveDebounce[6];
unsigned long orderDebounce[4];
unsigned long bunkerDebounce[4];

// current broadcasting orders
byte orders = 0;
byte bunkerOrders = 0;

const byte C_EMIT_JUMP_ORDERS = 32;
const byte C_EMIT_JUMP = 16;
const byte C_EMIT_BUNKER_ORDERS = 48;

const byte C_CMD_SHIFT = 4;
const byte C_CMD_JUMP = 1;
const byte C_CMD_JUMPORDERS = 2;
const byte C_CMD_BUNKERORDERS = 3;

const Color colors[] = {OFF, RED, GREEN, BLUE, YELLOW};

const unsigned long FACE_INTERVAL = 600;   // face turn
const unsigned long SEND_DURATION = 100;   // broadcast duration
const unsigned long DEBOUNCE_DURATION = 300; // brodacast debounce duration
const unsigned long SEND_SWITCH_FACTOR = 5; // switch between the two bytes every x ms
const unsigned long BUNKER_DURATION = 3000; // how long bunker lasts
const unsigned long BUNKER_INTERVAL = 30000; // min time between two bunkers

void setup() {
  // put your setup code here, to run once:

}

void show() {
  if (!occupied)
    setColor(OFF);
  else {
    setColor(colors[occupied]);
    setColorOnFace(WHITE, facing);
    if (millis() - bunkerSince < BUNKER_DURATION)
    {
      setColorOnFace(WHITE, (facing+2)%6);
      setColorOnFace(WHITE, (facing+4)%6);
    }
  }
}

void communicate() {
  unsigned long now = millis();
  for (byte color = 0; color < 4; ++color) {
     byte mask = 1 << color;
     if ((orders & mask) && now-orderDebounce[color] > SEND_DURATION) {
        orders &= ~mask;
     }
     if ((bunkerOrders & mask) && now-bunkerDebounce[color] > SEND_DURATION) {
        bunkerOrders &= ~mask;
     }
  }
  for (byte f=0; f<6; ++f) {
    byte c = getLastValueReceivedOnFace(f);
    byte o = c >> C_CMD_SHIFT;
    if (o == C_CMD_JUMP) {
      if (now - jumpReceiveDebounce[f] > DEBOUNCE_DURATION) {
        jumpReceiveDebounce[f] = now;
        orderDebounce[c&3] = now;
        if (occupied && now - bunkerSince < BUNKER_DURATION)
        { // splotch
        }
        else
        {
          occupied = 1 + (c & 3);
          facing = (f+3)%6; // face forward, facing back would be less interesting I think as you could double-tap
          facingSince = now;
        }
      }
    }
    else if (o == C_CMD_JUMPORDERS) {
      byte incomingOrders = c & 15;
      for (byte color = 0; color < 4; ++color) {
        byte mask = 1 << color;
        if ((incomingOrders & mask) && !(orders & mask) && now - orderDebounce[color] > DEBOUNCE_DURATION) {
          orders |= mask;
          orderDebounce[color] = now;
          if (occupied == color + 1 && now - bunkerSince > BUNKER_DURATION)
            doJump();
        }
      }
    }
    else if (o == C_CMD_BUNKERORDERS) {
      byte incomingOrders = c & 15;
      for (byte color = 0; color < 4; ++color) {
        byte mask = 1 << color;
        if ((incomingOrders & mask) && !(bunkerOrders & mask) && now - bunkerDebounce[color] > DEBOUNCE_DURATION) {
          bunkerOrders |= mask;
          bunkerDebounce[color] = now;
          if (occupied == color + 1)
            doBunker();
        }
      }
    }
    if (now - jumpSendDebounce[f] > SEND_DURATION) {
      if ( (now / SEND_SWITCH_FACTOR)%2 || !bunkerOrders)
        setValueSentOnFace(orders ? C_EMIT_JUMP_ORDERS + orders : 0, f);
      else
        setValueSentOnFace(bunkerOrders ? C_EMIT_BUNKER_ORDERS + bunkerOrders : 0, f);
    } 
  }
}

void doBunker() {
  bunkerSince = millis();
}
void doJump() {
  if (isValueReceivedOnFaceExpired(facing))
    return; // nice mode
  setValueSentOnFace(C_EMIT_JUMP + occupied-1, facing);
  jumpSendDebounce[facing] = millis();
  occupied = 0;
}

void addJumpOrder(byte color) {
  orders |= (1 << color);
  orderDebounce[color] = millis();
}

void addBunkerOrder(byte color) {
  bunkerOrders |= (1 << color);
  bunkerDebounce[color] = millis();
}

void loop() {
  // put your main code here, to run repeatedly:
  if (buttonLongPressed())
  {
    controlling = (controlling + 1)%5;
    occupied = controlling;
  }
  if (buttonSingleClicked())
  {
    if (controlling)
    {
      if (occupied && occupied == controlling)
        doJump();
      else
        addJumpOrder(controlling-1);
    }
  }
  if (buttonDoubleClicked())
  {
    if (lastBunkerOrder == 0 || millis() - lastBunkerOrder > BUNKER_INTERVAL)
    {
      if (occupied && occupied == controlling)
          doBunker();
        else
          addBunkerOrder(controlling-1);
      lastBunkerOrder = millis();
    }
  }
  if (buttonMultiClicked())
  {
    occupied = controlling;
    lastBunkerOrder = 0;
  }
  if (millis() - facingSince >= FACE_INTERVAL)
  {
    facing = (facing + 1)%6;
    facingSince = millis();
  }
  show();
  communicate();
}
