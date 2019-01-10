#include <SparkFunQuad.h>
#include <SwitchArray.h>
#include <LightArray.h>
#include <Slider.h>
#include <LineComm.h>
#include <Servo.h>

const int SFQ_COLOR_PINS[] = {45, 44, 43};
const int SFQ_LED_PINS[] = {52, 53, 50, 51};
const int SFQ_SWITCH_PINS[] = {48, 49, 46, 47};
SparkFunQuad sfq = SparkFunQuad(SFQ_COLOR_PINS, SFQ_LED_PINS, SFQ_SWITCH_PINS);

const byte SWA_NSWITCHES = 12;
const int SWA_PINS[] = {31, 32, 33, 34, 35, 36, 37, 38, 39, 42, 41, 40};
const int SWA_DOWN[] = {LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW};
SwitchArray swa = SwitchArray(SWA_NSWITCHES, SWA_PINS, SWA_DOWN);

const int LEDS_NLEDS = SWA_NSWITCHES;
const int LEDS_PINS[] = {2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 12};
const int LEDS_ON[] = {LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, HIGH, HIGH, HIGH};
LightArray leds = LightArray(LEDS_NLEDS, LEDS_PINS, LEDS_ON);

const int SLIDER_PIN = A0;
const int SLIDER_MIN_CHANGE = 10;
const char *DIGITS = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
const int SLIDER_BASE = 32;
Slider slider = Slider(SLIDER_PIN, SLIDER_MIN_CHANGE);

const int THROTTLE_PIN = 30;
const int THROTTLE_BASE = 14;
Servo throttle;

const long APP_SERIAL_BAUDRATE = 115200;
const int APP_SERIAL_BUFFER_LENGTH = 5*12*2;
char msg[APP_SERIAL_BUFFER_LENGTH];
int msgOffset = 0;
bool lastCharWasCR = false;

const int N_ANALOG_CHANNELS = 1;
bool pushAnalog[N_ANALOG_CHANNELS] = {false};
bool analogOnce[N_ANALOG_CHANNELS] = {false};

#define AppSerial Serial

void animate() {
  for (int i=0; i<LEDS_NLEDS; i++) {
    leds.SetLight(i, true);
    delay(30);
  }
  for (int i=0; i<4; i++) {
    for (int j=1; j<8; j++) {
      sfq.SetLedColor(i, j);
      sfq.UpdateState();
      delay(30);
    }
  }
  for (int i=0; i<LEDS_NLEDS; i++) {
    leds.SetLight(i, false);
    delay(30);
  }
  for (int i=0; i<4; i++) {
    sfq.SetLedColor(i, 0);
    sfq.UpdateState();
    delay(30);
  }
}

void setup() {
  AppSerial.begin(APP_SERIAL_BAUDRATE);
  
  sfq.Init();
  swa.Init();
  leds.Init();
  slider.Init();
  throttle.attach(THROTTLE_PIN);
  throttle.write(90);

  animate();

  AppSerial.print("*READY\n");
}

void loop() {
  sfq.UpdateState();
  swa.UpdateState();
  slider.UpdateState();

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
    //  a = group (Q for quad, B for blue row, G for green row, R for red row, A for arcade, S for square)
    //  b = index in group (0-based)
    //  c = 0 for off, anything else for on; 0-7 for colors
    if (msg[0] == 'S') {
      processed = true;
      if (msg[1] == 'Q') {
        byte index = msg[2] - '0';
        byte color = msg[3] - '0';
        sfq.SetLedColor(index, color);
      } else if (msg[1] == 'B') {
        leds.SetLight(msg[2] - '0', msg[3] != '0');
      } else if (msg[1] == 'G') {
        leds.SetLight(msg[2] - '0' + 3, msg[3] != '0');
      } else if (msg[1] == 'R') {
        leds.SetLight(msg[2] - '0' + 6, msg[3] != '0');
      } else if (msg[1] == 'A') {
        leds.SetLight(9, msg[3] != '0');
      } else if (msg[1] == 'S') {
        leds.SetLight(msg[2] - '0' + 10, msg[3] != '0');
      } else {
        processed = false;
      }

    // QSab: Query switch/button state
    //  a = group (see Sabc)
    //  b = index in group (see Sabc)
    } else if (msg[0] == 'Q' && msg[1] == 'S') {
      processed = true;
      if (msg[2] == 'Q') {
        sfq.ForceUpdate(msg[3] - '0');
      } else if (msg[2] == 'B') {
        swa.ForceUpdate(msg[3] - '0');
      } else if (msg[2] == 'G') {
        swa.ForceUpdate(msg[3] - '0' + 3);
      } else if (msg[2] == 'R') {
        swa.ForceUpdate(msg[3] - '0' + 6);
      } else if (msg[2] == 'A') {
        swa.ForceUpdate(msg[3] - '0' + 9);
      } else if (msg[2] == 'S') {
        swa.ForceUpdate(msg[3] - '0' + 10);
      } else {
        processed = false;
      }

    // QAa: Query analog channel
    //  a = analog index (only 0 for slider)
    } else if (msg[0] == 'Q' && msg[1] == 'A') {
      byte index = msg[2] - '0';
      if (index == 0) {
        slider.ForceUpdate();
        analogOnce[0] = true;
        processed = true;
      } else {
        processed = false;
      }

    // PAab: Set analog channel push notifications
    //  a = analog channel (only 0 for slider)
    //  b = 0 for off, anything else for on
    } else if (msg[0] == 'P' && msg[1] == 'A') {
      byte index = msg[2] - '0';
      if (index < N_ANALOG_CHANNELS) {
        pushAnalog[index] = msg[3] != '0';
        processed = true;
      } else {
        processed = false;
      }

    // Tabc: Set throttle servo to value
    //  a = servo name (only 0 for throttle servo)
    //  bc = base-14 value, digits starting with A
    } else if (msg[0] == 'T') {
      byte index = msg[1];
      if (index == '0') {
        int value = (msg[2] - 'A') * THROTTLE_BASE + (msg[3] - 'A');
        if (value <= 180) {
          processed = true;
          throttle.write(value);
        }
      }
    
    // *RID: Request the panel ID
    } else if (msg[0] == '*' && msg[1] == 'R' && msg[2] == 'I' && msg[3] == 'D') {
      processed = true;
      AppSerial.print("*PIDPanelA\n");

    // *ANI: Play startup animation sequence
    } else if (msg[0] == '*' && msg[1] == 'A' && msg[2] == 'N' && msg[3] == 'I') {
      processed = true;
      animate();

    // *READY: Echo back that device is ready
    } else if (msg[0] == '*' && msg[1] == 'R' && msg[2] == 'E' && msg[3] == 'A' && msg[4] == 'D' && msg[5] == 'Y') {
      processed = true;
      AppSerial.print("*READY\n");

    // ?: Print help message
    } else if (msg[0] == '?') {
      processed = true;
      AppSerial.print("*Help SQ{0-3}{0-7} S{BGR}{0-2}{01} SA0{01} SS{01}{01} QS{QBGRAS}{index} PA0{01} *RID *ANI *READY\n");
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

  // Send updates for any SparkFunQuad button changes
  for (byte i=0; i<SparkFunQuad::NBUTTONS; i++) {
    u = sfq.GetSwitchUpdate(i);
    if (u != SparkFunQuad::BUTTON_NO_CHANGE) {
      msg[0] = 'S';
      msg[1] = 'Q';
      msg[2] = '0' + i;
      msg[3] = u;
      msg[4] = '\0';
      AppSerial.print(msg);
      AppSerial.write('\n');
    }
  }

  // Send updates for any normal switch changes
  for (byte i=0; i<swa.nSwitches; i++) {
    u = swa.GetSwitchUpdate(i);
    if (u != SparkFunQuad::BUTTON_NO_CHANGE) {
      msg[0] = 'S';
      if (i < 3) { msg[1] = 'B'; msg[2] = '0' + i; }
      else if (i < 6) { msg[1] = 'G'; msg[2] = '0' + i - 3; }
      else if (i < 9) { msg[1] = 'R'; msg[2] = '0' + i - 6; }
      else if (i < 10) { msg[1] = 'A'; msg[2] = '0'; }
      else { msg[1] = 'S'; msg[2] = '0' + (i - 10); }
      msg[3] = u;
      msg[4] = '\0';
      AppSerial.print(msg);
      AppSerial.write('\n');
    }
  }

  int v;

  // Send update for slider (value is base SLIDER_BASE)
  if (slider.GetSliderUpdate(&v) && (pushAnalog[0] || analogOnce[0])) {
    msg[0] = 'A';
    msg[1] = '0';
    int h = v / SLIDER_BASE;
    msg[2] = DIGITS[h];
    int l = v - h * SLIDER_BASE;
    msg[3] = DIGITS[l];
    msg[4] = '\0';
    AppSerial.print(msg);
    AppSerial.write('\n');
    analogOnce[0] = false;
  }

  // Reset serial buffer after processing message
  if (msgReady) {
    msgOffset = 0;
  }
}
