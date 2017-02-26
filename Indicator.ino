#include <Arduino.h>
#include "avr/sleep.h"
#include <SPI.h>
#include <MFRC522.h>

#define INDICATOR_RIGHT 8    //output pin for right indicator
#define INDICATOR_LEFT A0    //output pin for left indicator
#define POWER_RFID_READER 0  //power out for RFID reader
#define RFID_LED 1           //LED to indicate the
#define BTN_WARNING 2        //btn pin for hazard switch
#define IGNITION_ON 3        //key switched power (12V)
#define BTN_LEFT 4           //left indicator button
#define BTN_RIGHT 5          //right indicator button
#define BTN_WARN_LIGHT 6     //warning light from hazard button
#define ENABLE_START 7       //enables the start switch if RFID is correct
#define RFID_RESET 9         //RFID reset pin
#define RFID_SDA 10          //RFID SDA pin

#define DELAY 100             //delay loop
#define MODULO 5              //modulo: corresponds to delay and loop. Flash every 500 ms (100 * 5)
#define SLEEP_TIME 60000      //delay before Atmega goes to sleep 1 * 60 * 1000 = 1 min
#define RFID_READ_TIME 15000  //delay before Atmega goes to sleep 150 * 1000 = 15 sec
#define RFID_BLINK_RATE 50    //blink every n/10 seconds e.g. 20 = 2 secs.

//global variables
int indicatorState = 0;      //current state of indicators 0 = all off
unsigned int count = 0;      //counter to allow blinking
long time = 0;               //holds the current time in millis
long powerOffTime = 0;       //power off time in millis
int ENABLE_SLEEP = false;    //flag to enable sleep mode
int START_IS_BLOCKED = true; //block start initially TRUE
int blockingState = 0;       //the current state of the blocking
bool READING_DONE = false;   //global flag to indicate the reading happend once

MFRC522 rfidReader(RFID_SDA, RFID_RESET); //initialise the RFID reader
/*byte allowedRfids[2][] = { //holds an array of allowed RFIDs
    {0x30, 0x8D, 0x1E, 0x83}, //ID 1 of a chip
    {0x88, 0xD2, 0xF7, 0x78}  //ID 2 of a chip
};*/
byte sesam[] = {0x30, 0x8D, 0x1E, 0x83};

/**
 * handle the different states for inidcator
 */
void handleIndicatorState(int state) {
    switch (state) {
        //indicator right on
        case 1:
            digitalWrite(INDICATOR_RIGHT, HIGH);
            break;

        //indicator left on
        case 2:
            digitalWrite(INDICATOR_LEFT, HIGH);
            break;

        //warning lights on
        case 3:
            digitalWrite(INDICATOR_LEFT, HIGH);
            digitalWrite(INDICATOR_RIGHT, HIGH);
            digitalWrite(BTN_WARN_LIGHT, HIGH);
            break;

        //all indicators off
        default:
            digitalWrite(INDICATOR_RIGHT, LOW);
            digitalWrite(INDICATOR_LEFT, LOW);
            digitalWrite(BTN_WARN_LIGHT, LOW);
            break;
    }
}

/**
 * read from RFID Scanner
 */
void readRfidTag() {
    digitalWrite(RFID_LED, HIGH);
    digitalWrite(POWER_RFID_READER, HIGH);
    delay(500);
    
    READING_DONE = true;
    bool CARD_SHOWED = false;
    long current = millis();
    long start = millis();
    
    while(current < (start + RFID_READ_TIME) && !CARD_SHOWED) { //read for RFID_READ_TIME
        current = millis();
        if ( rfidReader.PICC_IsNewCardPresent() && rfidReader.PICC_ReadCardSerial() ) {
            if ( isRfidAllowed() ) {
                digitalWrite(ENABLE_START, HIGH); //enable start switch
                START_IS_BLOCKED = false; //ingnition no longer blocked
            } else {
                signalAccessDenied();
                digitalWrite(RFID_LED, HIGH);
            }
            CARD_SHOWED = true;
        }
    }
    
    digitalWrite(POWER_RFID_READER, LOW);
    digitalWrite(RFID_LED, LOW);
}

/**
 * test if the RF card is valid
 */
bool isRfidAllowed() {
    return
      (rfidReader.uid.uidByte[0] == sesam[0]) &&
      (rfidReader.uid.uidByte[1] == sesam[1]) &&
      (rfidReader.uid.uidByte[2] == sesam[2]) &&
      (rfidReader.uid.uidByte[3] == sesam[3]);
}

/**
 * indicates that access was refused
 */
void signalAccessDenied() {
    for (int i = 0; i <= 3; i++) {
        digitalWrite(RFID_LED, HIGH);
        delay(250);
        digitalWrite(RFID_LED, LOW);
        delay(250);
    }
}

/**
 * handles enable/disable of the start switch
 * 
 * by reading RFID tag
 */
void handleBlockingState(int state) {
    
    switch(state) {
        case 1: //ignition off && blocking is active/true
            if (count % RFID_BLINK_RATE == 0) {
                digitalWrite(RFID_LED, HIGH);
            } else if (count % 2 == 0) {
                digitalWrite(RFID_LED, LOW);
            }
            break;

        case 2: //igntion on && blocking is inactive/false
            digitalWrite(ENABLE_START, HIGH);
            digitalWrite(RFID_LED, LOW);
            break;

        case 3:
            digitalWrite(RFID_LED, HIGH);
            break;

        default: //ignition off && blocking active/true
            digitalWrite(ENABLE_START, LOW);
            digitalWrite(RFID_LED, LOW);
    }
}


/**
 * wake up routine
 */
void wakeUp() {
  ENABLE_SLEEP = false;
  START_IS_BLOCKED = true;
  handleIndicatorState(0);
  handleBlockingState(0);
}

/**
 * send atmega to power off mode
 */
void gotoSleep() {
    indicatorState = 0;
    blockingState = 0;
    START_IS_BLOCKED = true;
    handleIndicatorState(0);
    handleBlockingState(0);

    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sleep_enable();
    attachInterrupt(0, wakeUp, LOW);
    attachInterrupt(1, wakeUp, HIGH);

    sleep_mode();
    sleep_disable();
    detachInterrupt(0);
    detachInterrupt(1);
}

/**
 * setup the Board and the PINs
 */
void setup() {
    Serial.begin(9600);
    SPI.begin();
    rfidReader.PCD_Init();
    //setup output pins
    pinMode(INDICATOR_RIGHT, OUTPUT);
    pinMode(INDICATOR_LEFT, OUTPUT);
    pinMode(BTN_WARN_LIGHT, OUTPUT);

    //immobiliser
    pinMode(POWER_RFID_READER, OUTPUT);
    pinMode(ENABLE_START, OUTPUT);
    pinMode(RFID_LED, OUTPUT);
    digitalWrite(ENABLE_START, LOW); //disable START Pin, block start switch

    //setup button pins
    pinMode(BTN_LEFT, INPUT);
    digitalWrite(BTN_LEFT, HIGH);    //enable internal pullup
    pinMode(BTN_RIGHT, INPUT);
    digitalWrite(BTN_RIGHT, HIGH);   //enable internal pullup
    pinMode(BTN_WARNING, INPUT);
    digitalWrite(BTN_WARNING, HIGH); //enable internal pullup

    //setup power wakeup pin
    pinMode(IGNITION_ON, INPUT);

    handleIndicatorState(0);
    handleBlockingState(0);
}

void loop() {
    time = millis();
    count++;

    int readBtnLeft = digitalRead(BTN_LEFT);
    int readBtnRight = digitalRead(BTN_RIGHT);
    int readBtnWarning = digitalRead(BTN_WARNING);
    int ignitionState = digitalRead(IGNITION_ON);

    //when power down go to sleep
    if (ENABLE_SLEEP && ignitionState == LOW) {
        if (time > (powerOffTime + SLEEP_TIME)) {
            gotoSleep();
        }
    } else if (ENABLE_SLEEP && ignitionState == HIGH) {
        ENABLE_SLEEP = false;
    }

    //handle blocking of start switch
    blockingState = 1;
    if (ignitionState == LOW && READING_DONE) { //engine off = reset flags
        READING_DONE = false;
    }

    if (ignitionState == HIGH && START_IS_BLOCKED) { //ingnition on, but start blocked, ready for RFID
        if (!READING_DONE) {
            readRfidTag();
        }
    } else if (ignitionState == HIGH && !START_IS_BLOCKED) { //ignition is on && start is not blocked == engine running mode
        blockingState = 2;
    } else if (ignitionState == LOW && !START_IS_BLOCKED) {
        blockingState = 3;
    }

    handleBlockingState(blockingState);

    //handle the btn readings
    if (readBtnWarning == LOW && count % MODULO == 0) {
        indicatorState = (indicatorState == 0) ? 3 : 0;
    } else if (readBtnRight == LOW && count % MODULO == 0) {
        indicatorState = (indicatorState == 0) ? 1 : 0;
    } else if (readBtnLeft == LOW && count % MODULO == 0) {
        indicatorState = (indicatorState == 0) ? 2 : 0;
    } else if (count % MODULO == 0) {  //no buttons pressed = turn off indicator gently
        indicatorState = 0;
    }

    handleIndicatorState(indicatorState);

    //check, if power down and start sleep mode in next cycle
    if (!ENABLE_SLEEP && ignitionState == LOW && readBtnWarning == HIGH) {
        powerOffTime = time;
        ENABLE_SLEEP = true;
    }

    //delay
    delay(DELAY);
}
