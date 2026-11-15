// Sammy Mohamed - EECE 3624 final project
// ZumoBot Co-Pilot: Zumo robot (Arduino) side

#include <SoftwareSerial.h>
#include <ZumoShield.h>

// bluetooth (pin 4 is a line sensor, so TX is on 6)
const uint8_t BT_RX_PIN = 2;
const uint8_t BT_TX_PIN = 6;

// speeds and timing
const int SPEED          = 300;
const int TURN_SPEED     = 250;
const unsigned int EDGE_THRESHOLD = 1500;  // black edge reads above this (lab 9)
const int ESCAPE_SPEED   = 300;
const int ESCAPE_BACK_MS = 200;
const int ESCAPE_TURN_MS = 250;
const unsigned long WATCHDOG_MS = 200;     // stop if no letter for this long

SoftwareSerial bt(BT_RX_PIN, BT_TX_PIN);
ZumoMotors motors;
ZumoReflectanceSensorArray sensors(QTR_NO_EMITTER_PIN);
unsigned int lineVals[6];

char lastCmd = 'S';
unsigned long lastCmdTime = 0;
char lastSent = 0;

// send status to the controller (only when it changes)
void report(char s) {
  if (s == lastSent) return;
  bt.write(s);
  lastSent = s;
}

// keep the newest valid letter
void readCommands() {
  while (bt.available()) {
    char c = bt.read();
    if (c == 'F' || c == 'B' || c == 'L' || c == 'R' || c == 'S') {
      lastCmd = c;
      lastCmdTime = millis();
    }
  }
}

void drive(char c) {
  switch (c) {
    case 'F': motors.setSpeeds( SPEED,       SPEED);      break;
    case 'B': motors.setSpeeds(-SPEED,      -SPEED);      break;
    case 'L': motors.setSpeeds(-TURN_SPEED,  TURN_SPEED); break;
    case 'R': motors.setSpeeds( TURN_SPEED, -TURN_SPEED); break;
    default:  motors.setSpeeds(0, 0);                     break;
  }
  report('M');
}

// 0 = no edge, 1 = left, 2 = right
int edgeSide() {
  sensors.read(lineVals);
  if (lineVals[0] > EDGE_THRESHOLD) return 1;
  if (lineVals[5] > EDGE_THRESHOLD) return 2;
  return 0;
}

// back up, then turn away from the edge
void escapeEdge(int side) {
  motors.setSpeeds(-ESCAPE_SPEED, -ESCAPE_SPEED);
  delay(ESCAPE_BACK_MS);
  if (side == 1) motors.setSpeeds( ESCAPE_SPEED, -ESCAPE_SPEED);
  else           motors.setSpeeds(-ESCAPE_SPEED,  ESCAPE_SPEED);
  delay(ESCAPE_TURN_MS);
  motors.setSpeeds(0, 0);
}

void setup() {
  bt.begin(9600);
  Serial.begin(9600);
  motors.setSpeeds(0, 0);
}

// edge first, then watchdog, then joystick
void loop() {
  readCommands();

  int e = edgeSide();
  if (e != 0) {
    report('E');
    escapeEdge(e);
    return;
  }

  if (millis() - lastCmdTime > WATCHDOG_MS) {
    motors.setSpeeds(0, 0);
    report('M');
    return;
  }

  drive(lastCmd);
}
