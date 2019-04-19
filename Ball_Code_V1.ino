#include <SPI.h>
#include "nRF24L01.h"
#include <RF24.h>
//#include "printf.h"
#include "stdio.h"
#include <string.h>
//#include "colors.h"
#include <SD.h>
#include <TMRpcm.h>
#include "adxl372.h"
#include "Communication.h"

#define SD_ChipSelectPin 4
const int ledR = 3;
const int ledG = 5;
const int ledB = 6;
const int sdCS = 4;
const int speakPin = 9;
const int rf24CS = 7;
const int rf24CE = 8;
const int xlCS = 10;
const int xlINT = 2;

enum State_enum {INIT, LISTENING, NOT_SELECTED, SELECTED, LED_PROGRAM, SPEAKER_PROGRAM, XL_PROGRAM, RADIO_ERR_STATE, PAYLOAD_ERR_STATE, DEBUG};
//enum Speak_enum {BEEP, DRIBBLE, PASS, SHOOT, GOAL}; 

//void state_machine_run(char radio_message[33]);
//void LEDs_ON(int color);
//void LEDs_blink(int color, int duration);
//void speak(uint8_t message);
//void soft_touch(uint8_t hard_touch, uint8_t noise_threshold);
//char get_radio_message();

//Radio variables
RF24 radio(rf24CE,rf24CS);
const uint64_t pipes[6] = { 0xABCDABCDABLL, 0xABCDABCDE1LL, 0xABCDABCDD2LL, 0xABCDABCDC3LL, 0xABCDABCDB4LL, 0xABCDABCDA5LL };
int devNum = 4;

//LED variables
uint8_t red = 0;
uint8_t green = 0;
uint8_t blue = 0;
uint8_t pattern = 0;
bool ledState = LOW;

//Speaker variables
TMRpcm tmrpcm;

//XL variables
struct adxl372_device adxl372;
//unsigned char devId;
AccelTriplet_t accel_data;

typedef struct {
  float x;
  float y;
  float z;
}acceleration_G_t;

acceleration_G_t data_G;

float threshold_low = 0.1;
float threshold_high = 0.8;

//State Machine variables
uint8_t state = INIT;
char payload[33];

void setup() {
  Serial.begin(9600);
  while(!Serial) {
    ;
  }
  Serial.println("Ball Code V1");

  radio.begin();
  radio.openWritingPipe(pipes[devNum]);
  radio.openReadingPipe(1, pipes[0]);

  radio.enableDynamicPayloads();
  radio.enableDynamicAck();
  radio.startListening();

  radio.printDetails();

  pinMode(ledR, OUTPUT);
  pinMode(ledG, OUTPUT);
  pinMode(ledB, OUTPUT);

  tmrpcm.speakerPin = 9;

  pinMode(xlCS, OUTPUT);
  pinMode(xlINT, INPUT);

  adxl372_Set_Op_mode(&adxl372, FULL_BW_MEASUREMENT);
  Set_Impact_Detection();

  //INIT state
  LEDs_blink(0, 255, 0, 1000);
}

void loop() {  
  state_machine_run(get_radio_message());
  Serial.print("State: ");
  Serial.println(state);
  delay(20);
}

void state_machine_run(char* radio_message)
{
  for(uint8_t i = 0; i < 32; ++i) {
    payload[i] = radio_message[i];
  }
  Serial.print("Payload: ");
  Serial.println(payload);
  Serial.print("radio_message: ");
  Serial.println(radio_message);
  delay(20);
  switch(state)
  {
    case INIT:
      {
        LEDs_blink(0, 255, 0, 1000);
        state = LISTENING;
      }
      break;

    case LISTENING:
      //Code to listen for soft select and light up LEDs if highlighted on screen
      if(strcmp(payload, "soft_select") == 0) {
        LEDs_blink(0, 0, 255, 2000);
      }
      else if(strcmp(payload, "selected") == 0) {
        LEDs_blink(255, 0, 255, 1000);
        state = SELECTED;
      }
      else if(strcmp(payload, "notselected") == 0) {
        LEDs_blink(255, 0, 0, 500);
        stopMultiListen();
        state = NOT_SELECTED;
      }
      else if(strcmp(payload, "noRadio") == 0) {
        LEDs_blink(255, 255, 255, 1000);
      }
      else {
        state = PAYLOAD_ERR_STATE;
      }
      break;

    case NOT_SELECTED:
      if(strcmp(payload, "program_escape") == 0) {
        startMultiListen();
        state = LISTENING;
      }
      break;

    case SELECTED:
      //Code to add ball to channel group and listen for next payload
      if(strcmp(payload, "leds") == 0) {
        LEDs_blink(255, 255, 255, 1000);
        state = LED_PROGRAM;
      }
      else if(strcmp(payload, "speaker") == 0) {
        LEDs_ON(0,0,0);
        speak("beep.wav", 6);
        state = SPEAKER_PROGRAM;
      }
      else if(strcmp(payload, "accel") == 0) {
        LEDs_ON(0,0,0);
        state = XL_PROGRAM;
      }
      else {
        state = PAYLOAD_ERR_STATE;
      }
      break;

    case LED_PROGRAM:
      //Code to parse payload color and pattern
      //FIX THIS! do {LEDs} while (payload != program_escape) 
      red = parse_LEDpayload(payload, 0);
      green = parse_LEDpayload(payload, 1);
      blue = parse_LEDpayload(payload, 2);
      pattern = parse_LEDpayload(payload, 3);
      
      do {
        if(pattern == 0) {
          if(ledState == LOW) {
            LEDs_ON(red, green, blue);
          }
          else if(ledState == HIGH) {
            LEDs_ON(0,0,0);
          }
        }
        else if(pattern == 1) {
          if(ledState == LOW) {
            LEDs_blinking(red, green, blue);
          }
          else if(ledState == HIGH) {
            LEDs_blinking(0,0,0);
          }
        }
      } while (strcmp(payload, "program_escape") != 0);
      if(strcmp(payload, "program_escape") == 0) {
        LEDs_ON(0,0,0);
        LEDs_blinking(0,0,0);
        state = LISTENING;
      }
      else {
        state = PAYLOAD_ERR_STATE;
      }
      break;

    case SPEAKER_PROGRAM:
      //Listen for transmission and parse
      if(strcmp(payload, "dribble") == 0) {
        speak("drib.wav", 6);
      }
      else if(strcmp(payload, "pass") == 0) {
        speak("pass.wav", 6);
      }
      else if(strcmp(payload, "shoot") == 0) {
        speak("shoot.wav", 6);
      }
      else if(strcmp(payload, "goal") == 0) {
        speak("goal.wav", 6);
        LEDs_blink(255, 255, 255, 3000);
      }
      else if(strcmp(payload, "program_escape") == 0) {
        state = LISTENING;
      }
      else {
        state = PAYLOAD_ERR_STATE;
      }
      break;

     case XL_PROGRAM:
        //Integrate Impact Demo code
        //Make it able to dynamically adjust threshold levels
        threshold_low = parse_XLpayload(payload, LOW);
        threshold_high = parse_XLpayload(payload, HIGH);
        
        while (strcmp(payload, "program_escape") != 0) {
          if(digitalRead(xlINT)) {
            delay(500);
            adxl372_Get_Highest_Peak_Accel_data(&adxl372, &accel_data);

            //Transform to G values
            data_G.x = (float)accel_data.x * 100/1000;
            data_G.y = (float)accel_data.y * 100/1000;
            data_G.z = (float)accel_data.z * 100/1000;

            float totalGs = sqrt(sq(data_G.x) + sq(data_G.y) + sq(data_G.z));

            if( totalGs > threshold_low && totalGs < threshold_high) {
              LEDs_blink(0, 0, 255, 300);
            }
          }
        }
        if(strcmp(payload, "program_escape") == 0) {
          state = LISTENING;
        }
        break;

      case RADIO_ERR_STATE:
        LEDs_blink(255, 0, 0, 2000);
        state = LISTENING;
        break;

      case PAYLOAD_ERR_STATE:
        LEDs_blink(255, 127, 0, 2000);
        state = LISTENING;
        break;

      case DEBUG:
        LEDs_blink(255, 0, 127, 2000);
        state = LISTENING;
        break;
        
  }
}

char* get_radio_message() {
  uint8_t* openPipe = new uint8_t;
  char defaultMessage[33] = "noRadio";

  if( radio.available(openPipe) && openPipe == 1) {
    char receivePayload[33];
    bool done = false;
    uint8_t len = radio.getDynamicPayloadSize();

    radio.read( &receivePayload, len );

    receivePayload[len] = 0;

    Serial.print("Received pipe: ");
    //Serial.print(openPipe);
    Serial.print("1, ");
    Serial.println(receivePayload);

    //Do dumb receive by commenting out this code
    radio.stopListening();
    radio.openWritingPipe(pipes[devNum]);

    delay(random(10, 20)*3);
    radio.write(&receivePayload, strlen(receivePayload), true);

    radio.startListening();
    return receivePayload;
  }
  else {
    return defaultMessage;
  }
}

void stopMultiListen() {
  radio.stopListening();
  radio.closeReadingPipe(0);
  radio.startListening();
}

void startMultiListen() {
  radio.stopListening();
  radio.openReadingPipe(1, pipes[0]);
  radio.openWritingPipe(pipes[devNum]);
  radio.startListening();
}

int parse_LEDpayload(char payload[33], int rgbp) {
  uint8_t redVal;
  uint8_t greenVal;
  uint8_t blueVal;
  uint8_t patternVal; //0 for solid, 1 for blinking

  sscanf(payload, "%d,%d,%d,%d", &redVal, &greenVal, &blueVal, &patternVal);
  if(rgbp == 0) {
    return redVal;
  }
  else if(rgbp == 1) {
    return greenVal;
  }
  else if(rgbp == 2) {
    return blueVal;
  }
  else {
    return patternVal;
  }
}

void LEDs_ON(int rVal, int gVal, int bVal) {
  analogWrite(ledR, rVal);
  analogWrite(ledG, gVal);
  analogWrite(ledB, bVal);
}

void LEDs_blink(int rVal, int gVal, int bVal, int duration) {
  unsigned long currentMillis = millis();
  unsigned long firstMillis = currentMillis;
  unsigned long previousMillis = 0;
  bool ledState = HIGH;
  int interval = 250;
  LEDs_ON(rVal, gVal, bVal);

  while (millis() - firstMillis < duration) {
    currentMillis = millis();
    
    if(currentMillis - previousMillis > interval) {   
      previousMillis = currentMillis;

      if(ledState == HIGH) {
        ledState = LOW;
        LEDs_ON(0,0,0);
      }
      else {
        ledState = HIGH;
        LEDs_ON(rVal, gVal, bVal);
      }

    }
  }
  LEDs_ON(0,0,0);
}

void LEDs_blinking(int rVal, int gVal, int bVal) {
  if(rVal == 0 && gVal == 0 && bVal == 0) {
    return;
  }
  else {
    unsigned long previousMillis = millis();
    //unsigned long interval = 250;
    int interval = 250;
    bool ledState = HIGH;
    LEDs_ON(rVal, gVal, bVal);

    if(millis() - previousMillis > interval) {
      previousMillis = millis();

      if(ledState == HIGH) {
        ledState = LOW;
        LEDs_ON(0,0,0);
      }
      else {
        ledState = HIGH;
        LEDs_ON(rVal, gVal, bVal);
      }
    }  
  }
}

void speak(char* speakFile, int speakVolume) {
  if(SD.begin(SD_ChipSelectPin)) {
    tmrpcm.setVolume(speakVolume);
    tmrpcm.play(speakFile);
  }
  else {
    analogWrite(speakPin, 127);
    delay(250);
    analogWrite(speakPin, 0);
  }
}

void Set_Impact_Detection(void)
{
  adxl372_Set_Op_mode(&adxl372, STAND_BY);
  
  adxl372_Set_Autosleep(&adxl372, false);
  
  adxl372_Set_BandWidth(&adxl372, BW_3200Hz);
  
  adxl372_Set_ODR(&adxl372, ODR_6400Hz);
  
  adxl372_Set_WakeUp_Rate(&adxl372, WUR_52ms);
  
  adxl372_Set_Act_Proc_Mode(&adxl372, LOOPED);
  
  /* Set Instant On threshold */
  adxl372_Set_InstaOn_Thresh(&adxl372, ADXL_INSTAON_LOW_THRESH); //Low threshold 10-15 G
  
  /*Put fifo in Peak Detect and Stream Mode */
  adxl372_Configure_FIFO(&adxl372, 512, STREAMED, XYZ_PEAK_FIFO);
  
  /* Set activity/inactivity threshold */
  adxl372_Set_Activity_Threshold(&adxl372, ACT_VALUE, true, true);
  adxl372_Set_Inactivity_Threshold(&adxl372, INACT_VALUE, true, true);
  
  /* Set activity/inactivity time settings */
  adxl372_Set_Activity_Time(&adxl372, ACT_TIMER);
  adxl372_Set_Inactivity_Time(&adxl372, INACT_TIMER);
  
  /* Set instant-on interrupts and activity interrupts */
   adxl372_Set_Interrupts(&adxl372);
  
  /* Set filter settle time */
  adxl372_Set_Filter_Settle(&adxl372, FILTER_SETTLE_16);
  
  /* Set operation mode to Instant-On */
  adxl372_Set_Op_mode(&adxl372, INSTANT_ON);
}

float parse_XLpayload(char payload[33], bool threshold) {
  //char payloadVal[33] = payload[33];
  float lowthresh;
  float highthresh;

  sscanf(payload, "%f,%f", &lowthresh, &highthresh);
  if(threshold) {
    return highthresh;
  }
  else {
    return lowthresh;
  }
}
