int ledR = 3;    // LED connected to digital pin 3
int ledG = 5;
int ledB = 6;

int delayVal = 100;

void setup() {
  pinMode(ledR, OUTPUT);
  pinMode(ledG, OUTPUT);
  pinMode(ledB, OUTPUT);
}

void loop() {
  
  // fade from red to green, in 5 point increments
  for (int fadeValue = 0 ; fadeValue <= 255; fadeValue += 5) {
    // sets the value (range from 0 to 255):
    analogWrite(ledR, 255-fadeValue);
    analogWrite(ledG, fadeValue);
    analogWrite(ledB, 0);
    //analogWrite(ledPinB, fadeValue);
    // wait for 30 milliseconds to see the dimming effect
    delay(delayVal);
  }

  // fade from green to blue, in 5 point increments
  for (int fadeValue = 0 ; fadeValue <= 255; fadeValue += 5) {
    // sets the value (range from 0 to 255):
    analogWrite(ledR, 0);
    analogWrite(ledG, 255-fadeValue);
    analogWrite(ledB, fadeValue);
    // wait for 30 milliseconds to see the dimming effect
    delay(delayVal);
  }

  // fade from blue to red, in 5 point increments
  for (int fadeValue = 0 ; fadeValue <= 255; fadeValue += 5) {
    // sets the value (range from 0 to 255):
    analogWrite(ledR, fadeValue);
    analogWrite(ledG, 0);
    analogWrite(ledB, 255-fadeValue);
    // wait for 30 milliseconds to see the dimming effect
    delay(delayVal);
  }
}
