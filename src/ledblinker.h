#include <Arduino.h>

void ledBlinkTask()
{
    static const int ledPin = 13;
    static int ledState = LOW;             // ledState used to set the LED
    static unsigned long previousMillis = 0;        // will store last time LED was updated
    static unsigned long currentMillis = 0;
    static unsigned long currentInterval = 0;
    static unsigned long ledBlinkOnInterval = 100;
    static unsigned long ledBlinkOffInterval = 2000;

    currentMillis = millis();
    currentInterval = currentMillis - previousMillis;
    
    if (ledState == LOW)
    {
        if (currentInterval > ledBlinkOffInterval)
        {
            previousMillis = currentMillis;
            ledState = HIGH;
            digitalWrite(ledPin, HIGH);
        }
    }
    else
    {
        if (currentInterval > ledBlinkOnInterval)
        {
            previousMillis = currentMillis;
            ledState = LOW;
            digitalWrite(ledPin, LOW);
        }
    }
}