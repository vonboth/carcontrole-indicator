#include <Arduino.h>
#include "avr/sleep.h"

#define INDICATOR_RIGHT 13
#define INDICATOR_LEFT 12
#define BTN_RIGHT 4
#define BTN_LEFT 5
#define BTN_WARNING 2
#define POWER_WAKE_UP 3
#define DELAY 100
#define MODULO 5

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
            break;

        //all indicators off
        default:
            digitalWrite(INDICATOR_RIGHT, LOW);
            digitalWrite(INDICATOR_LEFT, LOW);
            break;
    }
}

int indicatorState = 0;
unsigned int count = 0;
long time = 0;
long turnOffTime = 0;
int enableSleep = 0;

void wakeUp() {}

void gotoSleep() {
    handleIndicatorState(0);
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sleep_enable();
    attachInterrupt(0, wakeUp, HIGH);
    attachInterrupt(1, wakeUp, HIGH);

    sleep_mode();
    sleep_disable();
    detachInterrupt(0);
    detachInterrupt(1);
}

void setup() {
    Serial.begin(9600);
    pinMode(INDICATOR_RIGHT, OUTPUT);
    pinMode(INDICATOR_LEFT, OUTPUT);
    pinMode(BTN_LEFT, INPUT);
    pinMode(BTN_RIGHT, INPUT);
    pinMode(BTN_WARNING, INPUT);
    pinMode(POWER_WAKE_UP, INPUT); //vermutlich egal, weil INTERRUPT funktioniert, bei INPUT & OUTPUT
    digitalWrite(POWER_WAKE_UP, HIGH); //überprüfen, ob interner Pullup hier erforderlich

    handleIndicatorState(0);

    attachInterrupt(0, wakeUp, HIGH);
    attachInterrupt(1, wakeUp, HIGH);
}

void loop() {
    time = millis();

    int readLeft = digitalRead(BTN_LEFT);
    int readRight = digitalRead(BTN_RIGHT);
    int readWarning = digitalRead(BTN_WARNING);
    int readPowerOn = digitalRead(POWER_WAKE_UP);

    if (enableSleep == 1) {
        if (readPowerOn == LOW && readWarning == LOW && time > )
    }

    count++;

    if (readWarning == HIGH && count % MODULO == 0) {
        indicatorState = (indicatorState == 0) ? 3 : 0;
    } else if (readRight == HIGH && count % MODULO == 0) {
        indicatorState = (indicatorState == 0) ? 1 : 0;
    } else if (readLeft == HIGH && count % MODULO == 0) {
        indicatorState = (indicatorState == 0) ? 2 : 0;
    } else if (count % MODULO == 0) {
        indicatorState = 0;
    }

    handleIndicatorState(indicatorState);

    if (readPowerOn == LOW && readWarning == LOW) {
      gotoSleep();
    }
    
    Serial.println(count);
    delay(DELAY);
}
