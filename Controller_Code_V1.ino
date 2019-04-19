#include <SPI.h>
#include "nRF24L01.h"
#include <RF24.h>
#include "stdio.h"
#include <Nextion.h>
#include <string>

const int txPin = 1;
const int rxPin = 3;
const int misoPin = 19;
const int mosiPin = 23;
const int sclkPin = 18;
const int csnPin = 5;
const int cePin = 13;
const int buttonAPin = 25;
const int buttonALEDPin = 26;
const int buttonBPin = 27;
const int buttonBLEDPin = 14;
const int buttonCPin = 12;
const int buttonCLEDPin = 13;
const int buttonDPin = 4;
const int buttonDLEDPin = 16;

enum State_enum {INIT, BALL_SELECT, PROGRAM_SELECT, LEDS, SPEAKER, XL};

//List functions
//void arcadeLEDs(bool buttonEN[4], bool ledsVal );
//void arcadeLEDs_blink(bool buttonsBlink[4]);

// Set up nRF24L01 radio on ce/csn pins
RF24 radio(cePin, csnPin);

// Radio pipe addresses for all nodes. The first is for the controller, remaining are for balls.
const uint64_t pipes[6] = { 0xABCDABCDABLL, 0xABCDABCDE1LL, 0xABCDABCDD2LL, 0xABCDABCDC3LL, 0xABCDABCDB4LL, 0xABCDABCDA5LL };

//State Machine variables
uint8_t state = INIT;
char command[33];

//Declare Nextion objects
NexPage mainMenu = NexPage(0, 0, "mainMenu");
NexPage selectBalls = NexPage(1, 0, "selectBalls");
NexPage prog = NexPage(2, 0, "prog");
NexPage led = NexPage(3, 0, "led");
NexPage ledParams = NexPage(4, 0, "ledParams");
NexPage speaker = NexPage(5, 0, "speaker");
NexPage force = NexPage(6, 0, "force");
NexDSButton redBall = NexDSButton(1, 7, "redBall");
NexDSButton greenBall = NexDSButton(1, 8, "greenBall");
NexDSButton orangeBall = NexDSButton(1, 9, "orangeBall");
NexDSButton purpleBall = NexDSButton(1, 10, "purpleBall");
NexDSButton yellowBall = NexDSButton(1, 11, "yellowBall");
NexButton selected = NexButton(1, 4, "selected");
NexButton ledProg = NexButton(2, 6, "ledProg");
NexButton speakProg = NexButton(2, 7, "speakProg");
NexButton accelProg = NexButton(2, 8, "accelProg");
NexButton progBack = NexButton(2, 4, "progBack");
NexButton ledSelect = NexButton(3, 10, "ledSelect");
NexButton ledBack = NexButton(3, 4, "ledBack");
NexButton ledMain = NexButton(3, 3, "ledMain");
NexRadio r0 = NexRadio(4, 1, "r0");
NexRadio r1 = NexRadio(4, 3, "r1");
NexRadio r2 = NexRadio(4, 5, "r2");
NexRadio r3 = NexRadio(4, 7, "r3");
NexRadio r4 = NexRadio(4, 9, "r4");
NexRadio r5 = NexRadio(4, 11, "r5");
NexRadio r6 = NexRadio(4, 13, "r6");
NexRadio r7 = NexRadio(4, 15, "r7");
NexRadio r8 = NexRadio(4, 17, "r8");
NexRadio r9 = NexRadio(4, 19, "r9");
NexRadio r10 = NexRadio(4, 21, "r10");
NexText t0 = NexText(4, 2, "t0");
NexText t1 = NexText(4, 4, "t1");
NexText t2 = NexText(4, 6, "t2");
NexText t3 = NexText(4, 8, "t3");
NexText t4 = NexText(4, 10, "t4");
NexText t5 = NexText(4, 12, "t5");
NexText t6 = NexText(4, 14, "t6");
NexText t7 = NexText(4, 16, "t7");
NexText t8 = NexText(4, 18, "t8");
NexText t9 = NexText(4, 22, "t9");
NexText t10 = NexText(4, 23, "t10");
NexButton ledDone = NexButton(4, 24, "ledDone");
NexButton speakBack = NexButton(5, 4, "speakBack");
NexButton speakMain = NexButton(5, 3, "speakMain");
NexSlider softSlide = NexSlider(6, 6, "softSlide");
NexSlider noiseSlide = NexSlider(6, 7, "noiseSlide");
NexButton accelBack = NexButton(6, 4, "accelBack");
NexButton accelMain = NexButton(6, 3, "accelMain");

NexTouch *nex_listen_list[] = {
  &redBall, &greenBall, &orangeBall, &purpleBall, &yellowBall, &selected, &ledProg, &speakProg, &accelProg, &progBack, &ledSelect, &ledBack, &ledMain, &r0, &r1, &r2, &r3, &r4, &r5, &r6, &r7, &r8, &r9, &r10, &t0, &t1, &t2, &t3, &t4, &t5, &t6, &t7, &t8, &t9, &t10, &ledDone, &speakBack, &speakMain, &softSlide, &noiseSlide, &accelBack, &accelMain, NULL 
};

char sendBuffer[33];
uint8_t counter = 1;
uint8_t selRval = 0;
uint8_t selGval = 0;
uint8_t selBval = 0;
uint8_t selPatVal = 0;
uint8_t softSelNum = 0;
bool receivers[5];
bool selectedBool;
bool boolTrue[] = {true, true, true, true};
bool boolBlue[] = {true, false, false, false};

void setup(void)
{
  //Baud rate for Nextion display and/or IDE monitor output
  Serial.begin(9600);
  //Disable this code once display is connected to Tx/Rx
  Serial.println("Controller Code V1");

  // Setup and configure rf radio
  radioInit(pipes, 1, RF24_1MBPS, HIGH, LOW);

  // Init the display
  nexInit();

  //Setup and configure arcade buttons
  pinMode(buttonAPin, INPUT);
  pinMode(buttonBPin, INPUT);
  pinMode(buttonCPin, INPUT);
  pinMode(buttonDPin, INPUT);

  digitalWrite(buttonAPin, HIGH);
  digitalWrite(buttonBPin, HIGH);
  digitalWrite(buttonCPin, HIGH);
  digitalWrite(buttonDPin, HIGH);

  pinMode(buttonALEDPin, OUTPUT);
  pinMode(buttonBLEDPin, OUTPUT);
  pinMode(buttonCLEDPin, OUTPUT);
  pinMode(buttonDLEDPin, OUTPUT);

  digitalWrite(buttonALEDPin, LOW);
  digitalWrite(buttonBLEDPin, LOW);
  digitalWrite(buttonCLEDPin, LOW);
  digitalWrite(buttonDLEDPin, LOW);

  redBall.attachPop(redBall_Release, &redBall);
  greenBall.attachPop(greenBall_Release, &greenBall);
  orangeBall.attachPop(orangeBall_Release, &orangeBall);
  purpleBall.attachPop(purpleBall_Release, &purpleBall);
  yellowBall.attachPop(yellowBall_Release, &yellowBall);
  selected.attachPop(selected_Release, &selected);
}

void loop(void)
{
  nexLoop(nex_listen_list);
    state_machine_run(didStuffHappen());
    Serial.print("State: ");
    Serial.println(state);
    // Try again 20ms later
    delay(20);
}

void state_machine_run(bool stuffHappened) {
  
  
  switch(state) {
    case INIT:
    {
      arcadeLEDs_blink(boolTrue, 500, 250);
      state = BALL_SELECT;
      break;
    }

    case BALL_SELECT:
    {
      char selStr[9];
      uint8_t chanNum;
      char sendBufferSoftSel[] = "soft_select";
      char sendBufferSel[] = "selected";
      char sendBufferNS[] = "notselected";
      arcadeLEDs_blink(boolBlue, 1000, 250);

      if( !selectedBool ) {
        if(softSelNum == 1) {
          transmit_single(sendBufferSoftSel, strlen(sendBufferSoftSel), pipes[softSelNum], false, 500);
        }
        else if(softSelNum == 2) {
          transmit_single(sendBufferNS, strlen(sendBufferNS), pipes[softSelNum], false, 500);
        }
        else if(softSelNum == 3) {
          transmit_single(sendBufferNS, strlen(sendBufferNS), pipes[softSelNum], false, 500);
        }
        else if(softSelNum == 4) {
          transmit_single(sendBufferNS, strlen(sendBufferNS), pipes[softSelNum], false, 500);
        }
        else if(softSelNum == 5) {
          transmit_single(sendBufferNS, strlen(sendBufferNS), pipes[softSelNum], false, 500);
        }
      }
      else {
        for(uint8_t i = 0; i < 5; i++) {
          if(receivers[i]) {
            transmit_single(sendBufferSel, strlen(sendBufferSel), pipes[i], false, 500);
            delay(500);
          }
          else {
            transmit_single(sendBufferNS, strlen(sendBufferNS), pipes[i], false, 500);
            delay(500);
          }
        }
        state = PROGRAM_SELECT;
        selectedBool = false;
      }
      break;
    }

    case PROGRAM_SELECT:
    {
      if(strcmp(command, "leds") == 0) {
        //sendBuffer = "leds";
        strcpy(sendBuffer, "leds");
        transmit(sendBuffer, strlen(sendBuffer), pipes[0], false, 3000);
        state = LEDS;
      }
      else if(strcmp(command, "speaker") == 0) {
        //sendBuffer = "speaker";
        strcpy(sendBuffer, "speaker");
        transmit(sendBuffer, strlen(sendBuffer), pipes[0], false, 3000);
        state = SPEAKER;
      }
      else if(strcmp(command, "accel") == 0) {
        //sendBuffer = "accel";
        strcpy(sendBuffer, "accel");
        transmit(sendBuffer, strlen(sendBuffer), pipes[0], false, 3000);
        state = XL;
      }
      else if(strcmp(command, "back") == 0) {
        //make sure all receivers can listen
        //transmit "back" to ball and add if case to ball state machine
        state = BALL_SELECT;
      }
    }

    case LEDS:
    {
      /*
      uint8_t rVal;
      uint8_t gVal;
      uint8_t bVal;
      uint8_t patVal;
      sscanf(command, "%d,%d,%d,%d", rVal, gVal, bVal, patVal);
      */
      transmit(command, strlen(command), pipes[0], false, 500);
      break;
    }
  }
}

bool didStuffHappen() {
  Serial.println("didStuffHappen ran");
  bool stuffDidHappen = false;
  if(nexLoop(nex_listen_list)) {
    stuffDidHappen = true;
    Serial.println("nexLoop Ran and saw stuff");
  }
  else if(digitalRead(buttonAPin) || digitalRead(buttonBPin) || digitalRead(buttonCPin) || digitalRead(buttonDPin) ) {
    stuffDidHappen = true;
  }
  /*if(Serial.available() > 0) {
    String serialStr = Serial.readStringUntil("\n");
    strcpy(serialCharArr, serialStr.c_str());
    return serialCharArr;
  }*/
  return stuffDidHappen;
}

void radioInit(const uint64_t* addresses, int powerLevel, rf24_datarate_e dataRate, bool enDyn, bool printDet) {
  radio.begin();

  // optionally, increase the delay between retries & # of retries
  // radio.setRetries(15,15);

  // optionally, reduce the payload size.  seems to
  // improve reliability
  // radio.setPayloadSize(8);

  // Open 'our' pipe for writing
  // Open the 'other' pipes for reading (we can have up to 5 pipes open for reading)

  radio.openWritingPipe(addresses[0]);
  radio.openReadingPipe(1,addresses[1]);
  radio.openReadingPipe(2,addresses[2]);
  radio.openReadingPipe(3,addresses[3]);
  radio.openReadingPipe(4,addresses[4]);
  radio.openReadingPipe(5,addresses[5]);

  radio.setPALevel( powerLevel ); //0=MIN, 1=LOW, 2=HIGH, 3=MAX, 4=ERR
  radio.setDataRate( dataRate ); //0=1MBPS, 1=2MBPS, 2=250KBPS
  if(enDyn) {
    radio.enableDynamicPayloads();
    radio.enableDynamicAck();
  }
  
  //radio.setAutoAck( false );
  //radio.powerUp();
  radio.startListening();

  if(printDet) {
   radio.printDetails(); 
  }
}
bool transmit_single( char* sendBuffer, int bufferLen, uint64_t recAddress, bool lookForAck, int retryPeriod) {
  radio.stopListening();
  radio.openWritingPipe(recAddress);
  
  if(lookForAck) {
    bool ok = radio.write( &sendBuffer, bufferLen);

    if(ok) Serial.println("Acknowledged.");
    else Serial.println("No Ack.");
  }
  else {
    radio.write( &sendBuffer, bufferLen);
  }

  radio.startListening();

  unsigned long time_started_waiting = millis();
  uint8_t pipe_num;
  bool timeout = false;
  while( !timeout ) {
    if(radio.available(&pipe_num)) {
      uint8_t len = radio.getDynamicPayloadSize();
      char ackPayload[33];

      radio.read( ackPayload, len);
      ackPayload[len] = 0;

      Serial.print("Got repaly ");
      Serial.print(ackPayload);
      Serial.print(" from pipe ");
      Serial.println(pipe_num);
    }

    if( (millis() - time_started_waiting) > retryPeriod ) {
      timeout = true;
      Serial.println("Timeout! This round is over.\r\n");
    }
  }
  return timeout;
}

bool transmit( char* sendBuffer, int bufferLen, uint64_t esp32Address, bool lookForAck, int retryPeriod) {
  radio.stopListening();
  radio.openWritingPipe(esp32Address);
  
  if(lookForAck) {
    bool ok = radio.write( &sendBuffer, bufferLen);

    if(ok) Serial.println("Acknowledged.");
    else Serial.println("No Ack.");
  }
  else {
    radio.write( &sendBuffer, bufferLen);
  }

  radio.startListening();

  unsigned long time_started_waiting = millis();
  uint8_t pipe_num;
  bool timeout = false;
  while( !timeout ) {
    if(radio.available(&pipe_num)) {
      uint8_t len = radio.getDynamicPayloadSize();
      char ackPayload[33];

      radio.read( ackPayload, len);
      ackPayload[len] = 0;

      Serial.print("Got repaly ");
      Serial.print(ackPayload);
      Serial.print(" from pipe ");
      Serial.println(pipe_num);
    }

    if( (millis() - time_started_waiting) > retryPeriod ) {
      timeout = true;
      Serial.println("Timeout! This round is over.\r\n");
    }
  }
  return timeout;
}

void arcadeLEDs( bool buttons[], bool buttonValue) {
  if(buttons[0]) {
    digitalWrite(buttonALEDPin, buttonValue);
  }
  if(buttons[1]) {
    digitalWrite(buttonBLEDPin, buttonValue);
  }
  if(buttons[2]) {
    digitalWrite(buttonCLEDPin, buttonValue);
  }
  if(buttons[3]) {
    digitalWrite(buttonDLEDPin, buttonValue);
  }
}

void arcadeLEDs_blink( bool buttons[], int duration, int interval) {
  unsigned long currentMillis = millis();
  unsigned long firstMillis = currentMillis;
  unsigned long previousMillis = 0;
  bool ledState = HIGH;
  arcadeLEDs(buttons, HIGH);

  while(millis() - firstMillis < duration) {
    currentMillis = millis();

    if(currentMillis - previousMillis > interval) {
      previousMillis = currentMillis;

      if(ledState == HIGH) {
        ledState = LOW;
        arcadeLEDs(buttons, ledState);
      }
      else {
        ledState = HIGH;
        arcadeLEDs(buttons, ledState);
      }
    }
  }
  arcadeLEDs(buttons, LOW);
}

void redBall_Release(void *ptr) {
  bool softSelBool = false;
  uint32_t boolInt = 0;
  redBall.getValue(&boolInt);
  if(boolInt = 1) {
    receivers[0] = true;
    softSelNum = 1;
  }
  else {
    receivers[0] = false;
  }
}
void greenBall_Release(void *ptr) {
  bool softSelBool = false;
  uint32_t boolInt = 0;
  redBall.getValue(&boolInt);
  if(boolInt = 1) {
    receivers[1] = true;
    softSelNum = 2;
  }
  else {
    receivers[1] = false;
  }
}

void orangeBall_Release(void *ptr) {
  bool softSelBool = false;
  uint32_t boolInt = 0;
  redBall.getValue(&boolInt);
  if(boolInt = 1) {
    receivers[2] = true;
    softSelNum = 3;
  }
  else {
    receivers[2] = false;
  }
}

void purpleBall_Release(void *ptr) {
  bool softSelBool = false;
  uint32_t boolInt = 0;
  redBall.getValue(&boolInt);
  if(boolInt = 1) {
    receivers[3] = true;
    softSelNum = 4;
  }
  else {
    receivers[3] = false;
  }
}

void yellowBall_Release(void *ptr) {
  bool softSelBool = false;
  uint32_t boolInt = 0;
  redBall.getValue(&boolInt);
  if(boolInt = 1) {
    receivers[4] = true;
    softSelNum = 5;
  }
  else {
    receivers[4] = false;
  }
}

void selected_Release(void *ptr) {
  selectedBool = true;
}

void r0_Release(void *ptr) {
  selRval = 255;
  selGval = 0;
  selBval = 0;
}

void t0_Release(void *ptr) {
  selRval = 255;
  selGval = 0;
  selBval = 0;
}

void r1_Release(void *ptr) {
  selRval = 0;
  selGval = 255;
  selBval = 0;
}

void t1_Release(void *ptr) {
  selRval = 0;
  selGval = 255;
  selBval = 0;
}

void r2_Release(void *ptr) {
  selRval = 0;
  selGval = 0;
  selBval = 255;
}

void t2_Release(void *ptr) {
  selRval = 0;
  selGval = 0;
  selBval = 255;
}

void r3_Release(void *ptr) {
  selRval = 255;
  selGval = 255;
  selBval = 0;
}

void t3_Release(void *ptr) {
  selRval = 255;
  selGval = 255;
  selBval = 0;
}

void r4_Release(void *ptr) {
  selRval = 255;
  selGval = 127;
  selBval = 0;
}

void t4_Release(void *ptr) {
  selRval = 255;
  selGval = 127;
  selBval = 0;
}

void r5_Release(void *ptr) {
  selRval = 255;
  selGval = 0;
  selBval = 127;
}

void t5_Release(void *ptr) {
  selRval = 255;
  selGval = 0;
  selBval = 127;
}

void r6_Release(void *ptr) {
  selRval = 255;
  selGval = 0;
  selBval = 255;
}

void t6_Release(void *ptr) {
  selRval = 255;
  selGval = 0;
  selBval = 255;
}

void r7_Release(void *ptr) {
  selRval = 255;
  selGval = 255;
  selBval = 255;
}

void t7_Release(void *ptr) {
  selRval = 255;
  selGval = 255;
  selBval = 255;
}

void r8_Release(void *ptr) {
  selRval = 0;
  selGval = 0;
  selBval = 0;
}

void t8_Release(void *ptr) {
  selRval = 0;
  selGval = 0;
  selBval = 0;
}

void r9_Release(void *ptr) {
  selPatVal = 0;
}

void t9_Release(void *ptr) {
  selPatVal = 0;
}

void r10_Release(void *ptr) {
  selPatVal = 1;
}

void t10_Release(void *ptr) {
  selPatVal = 1;
}

void endNextionCommand()
{
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.write(0xff);
}
