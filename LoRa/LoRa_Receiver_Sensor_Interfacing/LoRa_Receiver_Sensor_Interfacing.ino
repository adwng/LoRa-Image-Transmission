#include <LoRa.h>
#include "boards.h"

char message[50];
char message1[50];

int extractMessageID(String packet) {
  // Example packet format: "ID, 123; Time, 12:34:56; Distance, 50; Temperature, 25; Humidity, 60.00"
  int index = packet.indexOf("ID, ");
  if (index != -1) {
    // Extract the message ID
    int semicolonIndex = packet.indexOf(";", index);
    if (semicolonIndex != -1) {
      String idStr = packet.substring(index + 4, semicolonIndex);
      return idStr.toInt();
    }
  }
  return -1; // Invalid packet format
}

void sendAcknowledgment(int messageID) {
  // Build acknowledgment packet
  String ackPacket = "ACK " + String(messageID);

  // Send acknowledgment packet
  LoRa.beginPacket();
  LoRa.print(ackPacket);
  LoRa.endPacket();
  // Serial.println("Acknowledgment sent: " + ackPacket);
}


void setup()
{
    Serial.begin(115200);
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
      delay(800);

      int messageID = extractMessageID(recv);
      if (messageID != -1) {
        // Serial.println("Message ID: " + String(messageID));

        // Send acknowledgment
        sendAcknowledgment(messageID);
      }
  }
}



