#include <Arduino.h>
#include "avr/sleep.h"

#define INDICATOR_RIGHT 12 //output pin for right indicator
#define INDICATOR_LEFT 13  //output pin for left indicator
#define BTN_WARNING 2      //btn pin for hazard switch
#define ENGINE_ON 3        //key switched power (12V)
#define BTN_LEFT 4         //left indicator button
#define BTN_RIGHT 5        //right indicator button
#define BTN_WARN_LIGHT 7   //warning light from hazard button
#define DELAY 100          //delay loop
#define MODULO 5           //modulo: corresponds to delay and loop. Flash every 500 ms (100 * 5)
#define SLEEP_TIME 60000  //delay before Atmega goes to sleep 2 * 60 * 1000 = 2 min

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

int indicatorState = 0; //current state of indicators 0 = all off
unsigned int count = 0; //counter to allow blinking
long time = 0;          //time
long powerOffTime = 0;  //power off time
int enableSleep = 0;    //flag to enable sleep mode

/**
 * wake up routine
 */
void wakeUp() {
  enableSleep = 0;
  handleIndicatorState(0);
}

/**
 * send atmega to power off mode
 */
void gotoSleep() {
    handleIndicatorState(0);
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sleep_enable();
    attachInterrupt(0, wakeUp, LOW);
    attachInterrupt(1, wakeUp, HIGH);

    sleep_mode();
    sleep_disable();
    detachInterrupt(0);
    detachInterrupt(1);
}

void setup() {
    //setup output pins
    pinMode(INDICATOR_RIGHT, OUTPUT);
    pinMode(INDICATOR_LEFT, OUTPUT);
    pinMode(BTN_WARN_LIGHT, OUTPUT);

    //setup button pins
    pinMode(BTN_LEFT, INPUT);
    digitalWrite(BTN_LEFT, HIGH);    //enable internal pullup
    pinMode(BTN_RIGHT, INPUT);
    digitalWrite(BTN_RIGHT, HIGH);   //enable internal pullup
    pinMode(BTN_WARNING, INPUT);
    digitalWrite(BTN_WARNING, HIGH); //enable internal pullup

    //setup power wakeup pin
    pinMode(ENGINE_ON, INPUT);

    handleIndicatorState(0);
}

void loop() {
    time = millis();

    int readBtnLeft = digitalRead(BTN_LEFT);
    int readBtnRight = digitalRead(BTN_RIGHT);
    int readBtnWarning = digitalRead(BTN_WARNING);
    int readPowerOn = digitalRead(ENGINE_ON);

    //when power down go to sleep
    if (enableSleep == 1 && readPowerOn == LOW) {
         if (time > (powerOffTime + SLEEP_TIME)) {
            gotoSleep();
        }
    } else if (enableSleep == 1 && readPowerOn == HIGH) {
        enableSleep = 0;
    }

    count++;

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
    if (enableSleep == 0 && readPowerOn == LOW && readBtnWarning == HIGH) {
        powerOffTime = time;
        enableSleep = 1;
    }

    //delay
    delay(DELAY);
}
