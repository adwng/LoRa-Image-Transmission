#include <LoRa.h>
#include "boards.h"

// int packet_number = 1;

int parseID(const String& message) {
  int idStartIndex = message.indexOf("ID,");
  if (idStartIndex == -1) {
    return -1; // ID not found
  }
  int idEndIndex = message.indexOf(';', idStartIndex);
  if (idEndIndex == -1) {
    return -1; // Semicolon not found after ID
  }
  String idString = message.substring(idStartIndex + 3, idEndIndex); // +3 to skip "ID,"
  return idString.toInt();
}

void send_ack(int packet_number) {
  byte packet_number_array[2];
  packet_number_array[0] = packet_number % 256;
  packet_number_array[1] = packet_number / 256;
  LoRa.beginPacket();
  LoRa.write(packet_number_array, 2);
  LoRa.endPacket();
}

void setup() {
  Serial.begin(115200);
  initBoard();
  delay(1500);

  Serial.println("LoRa Receiver");
  LoRa.setPins(RADIO_CS_PIN, RADIO_RST_PIN, RADIO_DIO0_PIN);
  if (!LoRa.begin(LoRa_frequency)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }

  LoRa.setSyncWord(0x6C);
  LoRa.enableCrc();
  LoRa.setSpreadingFactor(7);
  LoRa.setSignalBandwidth(500000);
}

void loop() {
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    String recv = "";
    while (LoRa.available()) {
      recv = LoRa.readString();
    }
    
    int received_id = parseID(recv);

    send_ack(received_id);
    //   packet_number++;
    // } else {
    //   Serial.println("Packet ID mismatch. Expected: " + String(packet_number) + ", Received: " + String(received_id));
    // }

    Serial.println(recv);
    char buf[256];
    snprintf(buf, sizeof(buf), "RSSI,%i", LoRa.packetRssi());
    Serial.println(buf);
    delay(800);
  }
}
