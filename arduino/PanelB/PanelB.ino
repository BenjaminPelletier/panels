#include <SwitchArray.h>
#include <LightArray.h>

const byte SWA_NSWITCHES = 19;
const int SWA_PINS[] = {53, 52, 51, 50, 49, 48, 45, 44, 42, 47, 43, 41, 46, 40, 38, 12, 11, 13, 10};
const int SWA_DOWN[] = {LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW};
SwitchArray swa = SwitchArray(SWA_NSWITCHES, SWA_PINS, SWA_DOWN);

const int LEDS_NLEDS = 16;
const int LEDS_PINS[] = {34, 33, 22, 30, 24, 23, 32, 25, 26, 35, 36, 37, 27, 28, 29, 31};
const int LEDS_ON[] = {HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH};
LightArray leds = LightArray(LEDS_NLEDS, LEDS_PINS, LEDS_ON);

const long APP_SERIAL_BAUDRATE = 115200;
const int APP_SERIAL_BUFFER_LENGTH = 5*12*2;
char msg[APP_SERIAL_BUFFER_LENGTH];
int msgOffset = 0;
bool lastCharWasCR = false;
#define AppSerial Serial

void animate() {
  for (int i=0; i<LEDS_NLEDS; i++) {
    leds.SetLight(i, true);
    delay(30);
  }
  for (int i=0; i<LEDS_NLEDS; i++) {
    leds.SetLight(i, false);
    delay(30);
  }
}


void setup() {
  AppSerial.begin(APP_SERIAL_BAUDRATE);
  
  swa.Init();
  leds.Init();

  animate();

  AppSerial.print("*READY\n");
}

void loop() {
  swa.UpdateState();

  // Read any queued serial data from the application interface
  bool msgReady = false;
  while (AppSerial.available() > 0) {
    char c = AppSerial.read();
    
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
    if (msgOffset >= APP_SERIAL_BUFFER_LENGTH) {
      AppSerial.println("*ERR Buffer overflow (line too long)");
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
      processed = true;
      if (msg[2] >= '0' && msg[2] <= '2' && msg[3] >= '0' && msg[3] <= '2')
        swa.ForceUpdate(3*(msg[2]-'0') + (msg[3]-'0'));
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
      AppSerial.print("*PIDPanelB\n");

    // *ANI: Play startup animation sequence
    } else if (msg[0] == '*' && msg[1] == 'A' && msg[2] == 'N' && msg[3] == 'I') {
      processed = true;
      animate();

    // *READY: Echo back that device is ready
    } else if (msg[0] == '*' && msg[1] == 'R' && msg[2] == 'E' && msg[3] == 'A' && msg[4] == 'D' && msg[5] == 'Y') {
      processed = true;
      AppSerial.print("*READY\n");
    }

    // Send error message if incoming message not processed
    if (!processed) {
      AppSerial.print("*ERR Could not parse '");
      AppSerial.print(msg);
      AppSerial.write('\'');
      AppSerial.write('\n');
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
      AppSerial.print(msg);
      AppSerial.write('\n');
    }
  }

  // Reset serial buffer after processing message
  if (msgReady) {
    msgOffset = 0;
  }
}
