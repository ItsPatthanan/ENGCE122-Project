#include <ESP32Servo.h>
Servo ControlS; 
extern int AngleC;
int pos = 0;

void setupSERVO() {
  ControlS.attach(21);
  Serial.begin(115200);
}

void loopSERVO() {
  if (AngleC == 0) {
    ControlS.write(40);
  } else if (AngleC == 1) {
    ControlS.write(53);
  } else if (AngleC == 2) {
    ControlS.write(66);
  } else if (AngleC == 3) {
    ControlS.write(79);
  } else if (AngleC == 4) {
    ControlS.write(92);
  } else if (AngleC == 5) {
    ControlS.write(105);
  } else if (AngleC == 6) {
    ControlS.write(118);
  } else if (AngleC == 7) {
    ControlS.write(131);
  } else if (AngleC == 8) {
    ControlS.write(144);
  } else {
    for (pos = 144; pos >= 0; pos -= 1) {
      ControlS.write(pos);
      delay(15);
    }
  }
}
