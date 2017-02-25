#include <Arduino.h>
#include "avr/sleep.h"
#include <SPI.h>
#include <MFRC522.h>

#define INDICATOR_RIGHT 8 //output pin for right indicator
#define INDICATOR_LEFT A0  //output pin for left indicator
#define RFID_LED 0         //LED to indicate the
#define BTN_WARNING 2      //btn pin for hazard switch
#define IGNITION_ON 3        //key switched power (12V)
#define BTN_LEFT 4         //left indicator button
#define BTN_RIGHT 5        //right indicator button
#define BTN_WARN_LIGHT 6   //warning light from hazard button
#define ENABLE_START 7     //enables the start switch if RFID is correct
#define RFID_RESET 9       //RFID reset pin
#define RFID_SDA 10        //RFID SDA pin

#define DELAY 100          //delay loop
#define MODULO 5           //modulo: corresponds to delay and loop. Flash every 500 ms (100 * 5)
#define SLEEP_TIME 60000  //delay before Atmega goes to sleep 1 * 60 * 1000 = 1 min
#define RFID_READ_TIME 15000  //delay before Atmega goes to sleep 150 * 1000 = 15 sec

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
 * test if the read uid is in the array of allowed id
 */
bool isAllowed(byte uid[4]) {
    bool allowed = false;
    for (int i = 0; i < sizeof(allowedRfids); i++) {
        allowed = (rfidReader.uid.uidByte[0] == allowedRfids[i][0]) &&
            (rfidReader.uid.uidByte[1] == allowedRfids[i][1]) &&
            (rfidReader.uid.uidByte[2] == allowedRfids[i][2]) &&
            (rfidReader.uid.uidByte[3] == allowedRfids[i][3])
    }
    return allowed;
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
        case 0: //ignition off && blocking active/true
            if (count % 20 == 0) {
                digitalWrite(RFID_LED, HIGH);
            } else if (count % 2 == 0) {
                digitalWrite(RFID_LED, LOW)
            }
            break;

        case 1: //ignition on && blocking active/true
            digitalWrite(RFID_LED, HIGH);
            long current = millis();
            long start = millis();
            while(current < (start + RFID_READ_TIME) && BLOCK_START == 1) { //read for RFID_READ_TIME
                current = millis();
                if (rfidReader.PICC_IsNewCardPresent()) {
                    if (allowedRfids(rfidReader.uid.uidByte)) {
                        digitalWrite(ENABLE_START, HIGH); //enable start switch
                        BLOCK_START = 0;
                    } else {
                        signalAccessDenied();
                    }
                }
            }
            digitalWrite(RFID_LED, LOW);
            break;

        case 2: //ignition off && blocking inactive/false
            digitalWrite(RFID_LED, HIGH);
            digitalWrite(RFID_LED, LOW);
            break;
    }
}

int indicatorState = 0; //current state of indicators 0 = all off
unsigned int count = 0; //counter to allow blinking
long time = 0;          //time
long powerOffTime = 0;  //power off time
int enableSleep = 0;    //flag to enable sleep mode
int BLOCK_START = 1;    //block start initially TRUE
int flagRfidLooped = 0;
int blockingState = 0;  //the current state of the blocking
MFRC522 rfidReader(RFID_SDA, RFID_RESET); //initialise the RFID reader
byte allowedRfids[2][4] = { //holds an array of allowed RFIDs
    {0x, 0x, 0x, 0x}, //ID 1 of a chip
    {0x, 0x, 0x, 0x}  //ID 2 of a chip
};

/**
 * wake up routine
 */
void wakeUp() {
  enableSleep = 0;
  BLOCK_START = 1;
  handleIndicatorState(0);
  handleBlockingState(0);
}

/**
 * send atmega to power off mode
 */
void gotoSleep() {
    indicatorState = 0;
    blockingState = 0;
    BLOCK_START = 1;
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
    SPI.begin();
    rfidReader.PCD_Init();
    //setup output pins
    pinMode(INDICATOR_RIGHT, OUTPUT);
    pinMode(INDICATOR_LEFT, OUTPUT);
    pinMode(BTN_WARN_LIGHT, OUTPUT);

    //immobiliser
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
    if (enableSleep == 1 && ignitionState == LOW) {
        if (time > (powerOffTime + SLEEP_TIME)) {
            gotoSleep();
        }
    } else if (enableSleep == 1 && ignitionState == HIGH) {
        enableSleep = 0;
    }

    //handle blocking of start switch
    if (ignitionState == LOW && BLOCK_START == 1) {  //case if ignition off, initial start
        blockingState = 0;
    } else if (ignitionState == HIGH && BLOCK_START = 1) { //ingnition on, but start blocked, ready for RFID
        blockingState = 1;
    } else if (ignitionState == LOW && BLOCK_START == 0) { //engine ran and is switched off, blocking off
        blockingState = 2;
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
    if (enableSleep == 0 && ignitionState == LOW && readBtnWarning == HIGH) {
        powerOffTime = time;
        enableSleep = 1;
    }

    //delay
    delay(DELAY);
}
