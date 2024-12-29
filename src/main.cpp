#ifdef FRAMEWORK_ARDUINO
#include <Arduino.h>
#include "receiver.h"
#include "ppm.h"

void setup()
{

    // Initialize pin
    pinMode(GPIO_LED_STATUS, OUTPUT);
    digitalWrite(GPIO_LED_STATUS, LOW); // Led on

    Serial.begin(921600);

    // Initialize ESP-NOW
    // initEspNow();

    PPMinit();

}

void loop()
{
    Serial.println("Updated channel data:");
    digitalWrite(GPIO_LED_STATUS, LOW); // Led on
    delay(500);
    digitalWrite(GPIO_LED_STATUS, HIGH); // Led on
    delay(500);
}

#endif