#include <SwitchArray.h>
#include <LightArray.h>
#include <LineComm.h>

const byte SWA_NSWITCHES = 19;
const int SWA_PINS[] = {53, 52, 51, 50, 49, 48, 45, 44, 42, 47, 43, 41, 46, 40, 38, 12, 11, 13, 10};
const int SWA_DOWN[] = {LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW};
SwitchArray swa = SwitchArray(SWA_NSWITCHES, SWA_PINS, SWA_DOWN);

const int LEDS_NLEDS = 16;
const int LEDS_PINS[] = {34, 33, 22, 30, 24, 23, 32, 25, 26, 35, 36, 37, 27, 28, 29, 31};
const int LEDS_ON[] = {HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH};
LightArray leds = LightArray(LEDS_NLEDS, LEDS_PINS, LEDS_ON);

const long BLUETOOTH_BAUDRATE = 38400;
const int BLUETOOTH_BUFFER_LENGTH = 5*12*2;
char msg[BLUETOOTH_BUFFER_LENGTH];
int msgOffset = 0;
bool lastCharWasCR = false;

int randomSwitches[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};

void setup() {
  Serial.begin(9600);
  Serial.println("PC serial started");

  //Serial1.begin(BLUETOOTH_BAUDRATE);
  
  swa.Init();
  leds.Init();

  randomSeed(analogRead(A1));
  for (int i=0; i<16; i++)
    randomSwitches[i] = random(32000) + 20;
  for (int i=0; i<16; i++) {
    int highestValue = 0;
    int highestIndex;
    for (int j=0; j<16; j++) {
      int r = randomSwitches[j];
      if (r > highestValue) {
        highestValue = r;
        highestIndex = j;
      }
    }
    randomSwitches[highestIndex] = i;
  }
  for (int i=0; i<16; i++) {
    Serial.println(randomSwitches[i]);
  }
}

void loop() {
  swa.UpdateState();

  // Read any queued serial data from the Bluetooth interface
  bool msgReady = false;
  while (Serial.available() > 0) {
    char c = Serial.read();
    //Serial.write(c);
    
    if (lastCharWasCR && c == '\n') {
      lastCharWasCR = false;
      continue;
    }
      
    if (c == '\r' || c == '\n') {
      msgReady = true;
      if (c == '\r')
        lastCharWasCR = true;
      c = '\0';
    }
    
    msg[msgOffset] = c;
    if (c == '\0')
      break;
      
    msgOffset++;
    if (msgOffset >= BLUETOOTH_BUFFER_LENGTH) {
      //Serial.println("*ERR Buffer overflow (line too long)");
      Serial.println("*ERR Buffer overflow (line too long)");
      msgOffset = 0;
      break;
    }
  }

  // Act on any incoming messages
  if (msgReady) {
    bool processed = false;
    
    // Sabc: Set light state
    //  ab = light ([L|R][G|Y|R] or RC where 0<=R<=2 and 0<=C<=2 or JC for joystick)
    //  c = 0 for off, anything else for on
    if (msg[0] == 'S') {
      processed = true;
      if (msg[1] >= '0' && msg[1] <= '2' && msg[2] >= '0' && msg[2] <= '2')
        leds.SetLight(3*(msg[1]-'0') + (msg[2]-'0'), msg[3] != '0');
      else if (msg[1] == 'L' && msg[2] == 'R')
        leds.SetLight(9, msg[3] != '0');
      else if (msg[1] == 'L' && msg[2] == 'Y')
        leds.SetLight(10, msg[3] != '0');
      else if (msg[1] == 'L' && msg[2] == 'G')
        leds.SetLight(11, msg[3] != '0');
      else if (msg[1] == 'R' && msg[2] == 'R')
        leds.SetLight(12, msg[3] != '0');
      else if (msg[1] == 'R' && msg[2] == 'Y')
        leds.SetLight(13, msg[3] != '0');
      else if (msg[1] == 'R' && msg[2] == 'G')
        leds.SetLight(14, msg[3] != '0');
      else if (msg[1] == 'J' && msg[2] == 'C')
        leds.SetLight(15, msg[3] != '0');
      else
        processed = false;

    // QSab: Query switch/button state
    //  a = group (see Sabc)
    //  b = index in group (see Sabc)
    } else if (msg[0] == 'Q' && msg[1] == 'S') {
      if (msg[2] >= '0' && msg[2] <= '2' && msg[3] >= '0' && msg[3] <= '2')
        swa.ForceUpdate(3*(msg[1]-'0') + (msg[2]-'0'));
      else if (msg[2] == 'L' && msg[3] == 'R')
        swa.ForceUpdate(9);
      else if (msg[2] == 'L' && msg[3] == 'Y')
        swa.ForceUpdate(10);
      else if (msg[2] == 'L' && msg[3] == 'G')
        swa.ForceUpdate(11);
      else if (msg[2] == 'R' && msg[3] == 'R')
        swa.ForceUpdate(12);
      else if (msg[2] == 'R' && msg[3] == 'Y')
        swa.ForceUpdate(13);
      else if (msg[2] == 'R' && msg[3] == 'G')
        swa.ForceUpdate(14);
      else if (msg[2] == 'J' && msg[3] == 'U')
        swa.ForceUpdate(15);
      else if (msg[2] == 'J' && msg[3] == 'R')
        swa.ForceUpdate(16);
      else if (msg[2] == 'J' && msg[3] == 'D')
        swa.ForceUpdate(17);
      else if (msg[2] == 'J' && msg[3] == 'L')
        swa.ForceUpdate(18);
      else
        processed = false;
    
    // *RID: Request the panel ID
    } else if (msg[0] == '*' && msg[1] == 'R' && msg[2] == 'I' && msg[3] == 'D') {
      processed = true;
      Serial.print("*PIDPanelB\n");
    }

    // Send error message if incoming message not processed
    if (!processed) {
      //Serial.print("*ERR Could not parse '");
      //Serial.print(msg);
      //Serial.write('\'');
      //Serial.write('*');
      Serial.print("*ERR Could not parse '");
      Serial.print(msg);
      Serial.write('\'');
      Serial.write('\n');
    }
  }

  char u;

  // Send updates for any normal switch changes
  for (byte i=0; i<swa.nSwitches; i++) {
    u = swa.GetSwitchUpdate(i);
    if (u != SwitchArray::BUTTON_NO_CHANGE) {
      msg[0] = 'S';
      if (i < 9) {
        msg[1] = '0' + (i/3); msg[2] = '0' + (i%3);
      } else if (i < 12) {
        msg[1] = 'L';
        if (i == 9) msg[2] = 'R';
        else if (i == 10) msg[2] = 'Y';
        else if (i == 11) msg[2] = 'G';
      } else if (i < 15) {
        msg[1] = 'R';
        if (i == 12) msg[2] = 'R';
        else if (i == 13) msg[2] = 'Y';
        else if (i == 14) msg[2] = 'G';
      } else {
        msg[1] = 'J';
        if (i == 15) msg[2] = 'U';
        else if (i == 16) msg[2] = 'R';
        else if (i == 17) msg[2] = 'D';
        else msg[2] = 'L';
      }
      msg[3] = u;
      msg[4] = '\0';
      Serial.print(msg);
      Serial.write('\n');

      int ir = i >= leds.nLights ? leds.nLights - 1 : i;
      int rs = 0;
      for (int r=0; r < leds.nLights; r++) {
        if (ir == randomSwitches[r]) {
          rs = randomSwitches[(r+1)%16];
          break;
        }
      }
      if (u == SwitchArray::BUTTON_DOWN) {
        leds.SetLight(rs, true);
      } else if (u == SwitchArray::BUTTON_UP) {
        leds.SetLight(rs, false);
      }
    }
  }

  // Reset serial buffer after processing message
  if (msgReady) {
    msgOffset = 0;
  }
}
