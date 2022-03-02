#include <Arduino.h>
#include "SSD1306Wire.h"
#include "LoRa.h"

#define LED 25
#define SENDER 1

SSD1306Wire display(0x3c, SDA, SCL);
unsigned long prevMillisBlink = 0;
unsigned long prevMillisDisp = 0;
unsigned long prevMillisRestart = 0;
unsigned long prevMillisSend = 0;
unsigned long counter = 0;
String packet = "";
int rssi = 0;
bool isLedHigh = false;
hw_timer_t *timer = NULL;

void IRAM_ATTR restartEsp()
{
    esp_restart();
}

void setup()
{
    timer = timerBegin(0, 80, true);
    timerAttachInterrupt(timer, &restartEsp, true);
    timerAlarmWrite(timer, 3e6, false);
    timerAlarmEnable(timer);

    pinMode(LED, OUTPUT);

    Serial.begin(115200);
    while (!Serial)
        ;

    display.init();
    display.flipScreenVertically();

    SPI.begin(5, 19, 27, 18);
    LoRa.setPins(18, 23, 26);
    if (!LoRa.begin(433e6))
    {
        Serial.println("LoRa.begin() failed");
        while (true)
            ;
    }
    LoRa.setTxPower(20, PA_OUTPUT_PA_BOOST_PIN);
    LoRa.setSpreadingFactor(10);
    LoRa.setSignalBandwidth(125e3);
    LoRa.setCodingRate4(5);
    LoRa.setPreambleLength(8);
    LoRa.setSyncWord(0x47);
    LoRa.enableCrc();
}

void loop()
{
    timerWrite(timer, 0);

    unsigned long currMillis = millis();

    if (currMillis - prevMillisRestart >= 60000)
    {
        prevMillisRestart = currMillis;

        esp_restart();
    }

    if (currMillis - prevMillisBlink >= 500)
    {
        prevMillisBlink = currMillis;

        isLedHigh = !isLedHigh;
        if (isLedHigh)
        {
            digitalWrite(LED, HIGH);
        }
        else
        {
            digitalWrite(LED, LOW);
        }
    }

    if (LoRa.parsePacket())
    {
        packet = "";
        while (LoRa.available())
        {
            packet += (char)LoRa.read();
        }
        if (packet.substring(0, 14) == "rrtechloratest")
        {
            prevMillisRestart = currMillis;

            packet = packet.substring(14);
            rssi = LoRa.packetRssi();

#if !SENDER
            LoRa.beginPacket();
            LoRa.print("rrtechloratest");
            LoRa.print(packet);
            LoRa.endPacket();
#endif
        }
        else
        {
            packet = "";
            rssi = 0;
        }
    }

    if (currMillis - prevMillisDisp >= 100)
    {
        prevMillisDisp = currMillis;

        display.clear();
        display.setFont(ArialMT_Plain_10);
        display.drawString(0, 0, "RR Tech LoRa Tester");
        display.setFont(ArialMT_Plain_16);
#if SENDER
        display.drawString(0, 10, "TX: " + String(counter));
#else
        display.drawString(0, 10, "Receiver");
#endif
        display.drawString(0, 26, "RX: " + packet);
        display.drawString(0, 42, "SS: " + String(rssi));
        display.display();
    }

#if SENDER
    if (currMillis - prevMillisSend >= 3000)
    {
        prevMillisSend = currMillis;

        LoRa.beginPacket();
        LoRa.print("rrtechloratest");
        LoRa.print(counter);
        LoRa.endPacket();

        counter++;
    }
#endif
}
