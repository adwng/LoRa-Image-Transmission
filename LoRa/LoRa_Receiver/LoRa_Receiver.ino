#include <LoRa.h>
#include "boards.h"

char message[50];
char message1[50];


void setup()
{
    initBoard();
    // When the power is turned on, a delay is required.
    delay(1500);

    Serial.println("LoRa Receiver");

    LoRa.setPins(RADIO_CS_PIN, RADIO_RST_PIN, RADIO_DIO0_PIN);
    if (!LoRa.begin(LoRa_frequency)) {
        Serial.println("Starting LoRa failed!");
        while (1);
    }
}

void loop()
{
    // try to parse packet
    int packetSize = LoRa.parsePacket();
    if (packetSize) {

        String recv = "";
        // read packet
        while (LoRa.available()) {
            recv =LoRa.readString();
        }

        Serial.println(recv);

        // int* values = ParseData(recv);
        // sprintf(message, "distance(cm):%d", values[0]);
        // // sprintf(message1, "distance(inch):%d", values[1]);
        
        // if (u8g2) {
        //     u8g2->clearBuffer();
            
        //     u8g2->drawStr(0, 10, "Received OK!");
            
        //     //  u8g2->drawStr(0, 20, message);
        //     // u8g2->drawStr(0, 30, message1);

        //     u8g2->drawStr(0, 20, recv.c_str());

        //     char buf[256];
        //     snprintf(buf, sizeof(buf), "RSSI:%i", LoRa.packetRssi());
        //     u8g2->drawStr(0, 40, buf);
        //     snprintf(buf, sizeof(buf), "SNR:%.1f", LoRa.packetSnr());
        //     u8g2->drawStr(0, 50, buf);
        //     u8g2->sendBuffer();
        // }
    }
}


int* ParseData(String recv)
{
    char *token;
    uint8_t idx = 0;
    char *pszValues[2]; // Adjust the size as per your data
    int finalValues[2] = {0}; // Initialize with 0

    token = strtok(recv.begin(), ",");
    while (token != NULL)
    {
        if (idx < 2)
        {
            pszValues[idx++] = token;
        }
        token = strtok(NULL, ",");
    }

    for (int i = 0; i < idx; i++) // Iterate up to the number of tokens found
    {
        finalValues[i] = atoi(pszValues[i]);
    }

    return finalValues;
}