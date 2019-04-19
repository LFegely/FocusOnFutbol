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
const int cePin = 17;
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

void setup(void)
{
  //Baud rate for Nextion display and/or IDE monitor output
  Serial.begin(9600);
  //Disable this code once display is connected to Tx/Rx
  Serial.println("Controller Code V1");

  // Setup and configure rf radio
  radioInit(pipes, 1, 0, HIGH, LOW);

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
}

char sendBuffer[33];
uint8_t counter = 1;

void loop(void)
{
    state_machine_run(get_Serial_Message());
    Serial.print("State: ");
    Serial.println(state);
    // Try again 20ms later
    delay(20);
}

void state_machine_run(char* serialMessage) {
  for(uint8_t i = 0; i < 32; ++i) {
    command[i] = serialMessage[i];
  }
  Serial.print("Payload: ");
  Serial.println(payload);
  Serial.print("radio_message: ");
  Serial.println(radio_message);
  delay(20);
  
  switch(state) {
    case INIT:
      arcadeLEDs_blink({true, true, true, true}, 500, 250);
      state = BALL_SELECT;
      break;

    case BALL_SELECT:
      bool receivers[5];
      char selStr[9];
      uint8_t chanNum;
      if(sscanf(command, "%8c,%b,%b,%b,%b,%b", &selStr, &receivers[0], &receivers[1], &receivers[2], &receivers[3], &receivers[4]) > 2) {
        char sendBufferSel[] = "selected";
        char sendBufferNS[] = "notselected";
        for(uint8_t i = 0; i < 6; i++) {
          if(receivers[i]) {
            transmit_single(&sendBufferSel, strlen(sendBufferSel), pipes[i], false, 500);
            delay(500);
          }
          else {
            transmit_single(&sendBufferNS, strlen(sendBufferNS), pipes[i], false, 500);
            delay(500);
          }
        }
        state = PROGRAM_SELECT;
      }
      else {
        sscanf(command, "%d", &chanNum);
        sendBuffer = "soft_select";
        transmit_single(&sendBuffer, strlen(sendBuffer), pipes[chanNum], false, 3000);
      }
      break;

    case PROGRAM_SELECT:
      if(strcmp(command, "leds") == 0) {
        sendBuffer = "leds";
        transmit(&sendBuffer, strlen(sendBuffer), pipes[0], false, 3000);
        state = LEDS;
      }
      else if(strcmp(command, "speaker") == 0) {
        sendBuffer = "speaker";
        transmit(&sendBuffer, strlen(sendBuffer), pipes[0], false, 3000);
        state = SPEAKER;
      }
      else if(strcmp(command, "accel") == 0) {
        sendBuffer = "accel";
        transmit(&sendBuffer, strlen(sendBuffer), pipes[0], false, 3000);
        state = XL;
      }

    case LEDS:
      
  }
}

char* get_Serial_Message() {
  char serialCharArr[33]:
  if(Serial.available() > 0) {
    String serialStr = Serial.readStringUntil("\n");
    strcpy(serialCharArr, serialStr.c_str());
    return serialCharArr;
  }
}

void radioInit(uint64_t* addresses, int powerLevel, int dataRate, bool enDyn, bool printDet) {
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
    if(radio.available(&pipenum)) {
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
    if(radio.available(&pipenum)) {
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

void endNextionCommand()
{
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.write(0xff);
}
