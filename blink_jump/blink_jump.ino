
byte occupied = 0;
byte facing = 0;
unsigned long facingSince = 0;
byte controlling = 0;

unsigned long jumpSendDebounce[6];
unsigned long jumpReceiveDebounce[6];
unsigned long orderDebounce[4];
byte orders = 0;

const byte C_EMIT_JUMP_ORDERS = 32;
const byte C_EMIT_JUMP = 16;
const byte C_CMD_SHIFT = 4;
const byte C_CMD_JUMP = 1;
const byte C_CMD_JUMPORDERS = 2;

const Color colors[] = {OFF, RED, GREEN, BLUE, CYAN};

const unsigned long FACE_INTERVAL = 600;
const unsigned long SEND_DURATION = 100;
const unsigned long DEBOUNCE_DURATION = 300;

void setup() {
  // put your setup code here, to run once:

}

void show() {
  if (!occupied)
    setColor(OFF);
  else {
    setColor(colors[occupied]);
    setColorOnFace(WHITE, facing);
  }
}

void communicate() {
  unsigned long now = millis();
  for (byte color = 0; color < 4; ++color) {
     byte mask = 1 << color;
     if ((orders & mask) && now-orderDebounce[color] > SEND_DURATION) {
        orders &= ~mask;
     }
  }
  for (byte f=0; f<6; ++f) {
    byte c = getLastValueReceivedOnFace(f);
    byte o = c >> C_CMD_SHIFT;
    if (o == C_CMD_JUMP) {
      if (now - jumpReceiveDebounce[f] > DEBOUNCE_DURATION) {
        jumpReceiveDebounce[f] = now;
        orderDebounce[c&3] = now;
        occupied = 1 + (c & 3);
        facing = f;
        facingSince = now;
      }
    }
    else if (o == C_CMD_JUMPORDERS) {
      byte incomingOrders = c & 15;
      for (byte color = 0; color < 4; ++color) {
        byte mask = 1 << color;
        if ((incomingOrders & mask) && !(orders & mask) && now - orderDebounce[color] > DEBOUNCE_DURATION) {
          orders |= mask;
          orderDebounce[color] = now;
          if (occupied == color + 1)
            doJump();
        }
        
      }
    }
    if (now - jumpSendDebounce[f] > SEND_DURATION) {
      setValueSentOnFace(orders ? C_EMIT_JUMP_ORDERS + orders : 0, f);
    } 
  }
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
  if (millis() - facingSince >= FACE_INTERVAL)
  {
    facing = (facing + 1)%6;
    facingSince = millis();
  }
  show();
  communicate();
}
