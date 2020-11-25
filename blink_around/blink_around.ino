
byte nColors = 2;
byte spinning;
byte offset;
byte roll[6];

byte debouncing = 0;
unsigned long debounceTime = 0;

const unsigned long SPEED = 400;
const Color colors[] = {OFF, RED, GREEN, BLUE};

const byte COMM_RESTART = 33;
const byte COMM_SWITCH = 34;

void reRoll2() {
  byte slots[3];
  slots[0] = randomWord()%6;
  while (true) {
    slots[1] = randomWord()%6;
    if (slots[1] != slots[0])
       break;
  }
  while (true) {
    slots[2] = randomWord()%6;
    if (slots[1] != slots[2] && slots[0] != slots[2])
      break;
  }
  for (byte f=0; f<6; ++f) {
    if (slots[0] == f || slots[1] == f || slots[2] == f)
      roll[f] = 1;
    else
      roll[f] = 2;  
  }
}

void reRoll3() {
  byte slot0[2];
  byte slot1[2];
  slot0[0] = randomWord()%6;
  do {
    slot0[1] = randomWord()%6;
  } while (slot0[0] == slot0[1]);
  do {
    slot1[0] = randomWord()%6;
  } while (slot1[0] == slot0[0] || slot1[0] == slot0[1]);
  do {
    slot1[1] = randomWord()%6;
  } while (slot1[1] == slot0[0] || slot1[1] == slot0[1] || slot1[1] == slot1[0]);
  for (byte f=0; f<6; ++f) {
    if (slot0[0] == f || slot0[1] == f)
      roll[f] = 1;
    else if (slot1[0] == f || slot1[1] == f)
      roll[f] = 2;
    else
      roll[f] = 3;
  }
}

void reRoll() {
  if (nColors == 2)
    reRoll2();
  else
    reRoll3();
}

void setup() {
  nColors = 2;
  spinning = 1;
  setValueSentOnAllFaces(0);
  randomize();
  reRoll();
}

void communication() {
  if (debouncing && millis() - debounceTime > 300)
    setValueSentOnAllFaces(0xFF);
  if (debouncing && millis() - debounceTime > 400) {
    debouncing = 0;
    emit();
  }
  for (byte f=0; f<6; ++f) {
    byte v = getLastValueReceivedOnFace(f);
    if ((v & 0xFC) == 0) {
      if (v != 0 && !spinning && roll[(f+offset)%6] != v) {
        roll[(f+offset)%6] = 0;
        setColorOnFace(OFF, f);
      }
    }
    else if (!debouncing && v != IR_DATA_VALUE_MAX) {
      debouncing = 1;
      debounceTime = millis();
      setValueSentOnAllFaces(v);
      if (v == COMM_RESTART) {
        spinning = 1;
        reRoll();
      }
      else if (v == COMM_SWITCH) {
        spinning = 1;
        nColors = 5 - nColors;
        reRoll();
      }
    }
  }
}
void emit() {
  if (spinning)
    setValueSentOnAllFaces(0xFF);
  else {
    for (byte f=0; f<6; ++f) {
      setValueSentOnFace(roll[(f+offset)%6], f);
      if (isValueReceivedOnFaceExpired(f))
        setColorOnFace(OFF, f);
    }
  }
}

void interaction() {
  if (buttonDown() && spinning) {
    spinning = 0;
    emit();
  }
  if (buttonDoubleClicked()) {
    spinning = 1;
    reRoll();
    emit();
    setValueSentOnAllFaces(COMM_RESTART);
  }
  if (buttonLongPressed()) {
    nColors = 5 - nColors;
    reRoll();
    spinning = 1;
    emit();
    setValueSentOnAllFaces(COMM_SWITCH);
    debouncing = 1;
    debounceTime = millis();
  }
}

void loop() {
  interaction();
  communication();
  if (spinning) {
    offset = (millis() / SPEED)%6;
    for (byte f=0; f<6; ++f) {
       setColorOnFace(colors[roll[(f+offset)%6]], f);
    }
  }
}
