#include "boards.h"
#include <SPIFFS.h>
#include <LoRa.h>
#include <EEPROM.h>
#include "arduino_base64.hpp"

#define FILE_PHOTO "/photo.jpg"  //path to the image in SPIFFS
#define EEPROM_SIZE 1

int EEPROM_position = 0;
int total_packets;
const int chunk_size = 200;
byte chunk[chunk_size];
int packet_number;
byte current_packet_array[2];
String string_num;
File file;
int EEPROM_count = 0;
int bandwidth = 500000;
int spread_factor = 7;
String rssi;
String snr;
String path;
int ack_retransmission_period = random(200, 400);
unsigned long start_millis;
unsigned long current_millis;
bool transmissionInProgress = false;


// when a packet indicating a new photo arrives
void new_photo() {
  if (EEPROM_count == 1000) {
    EEPROM_count = 0;
    EEPROM.begin(EEPROM_SIZE);
    EEPROM_position = EEPROM.read(0) + 1;
    EEPROM.write(0, EEPROM_position);
    EEPROM.commit();
  }
  path = "/picture" + String(EEPROM_position) + "_" + String(EEPROM_count) + ".jpg";  //path where image will be stored on SD card
  EEPROM_count++;
  Serial.printf("Picture file name: %s\n", path.c_str());
  file = SPIFFS.open(FILE_PHOTO, FILE_WRITE);  //save current photo to SPIFFS
  if (!file) {
    Serial.println("Failed to open file in writing mode");
  }
}

void send_ack(const byte* packet_number_array) { 
  Serial.println("\nSending ack for packet " + String(packet_number_array[0] + 256 * packet_number_array[1]));
  LoRa.setSignalBandwidth(bandwidth);
  LoRa.setSpreadingFactor(spread_factor);
  LoRa.beginPacket();
  LoRa.write(packet_number_array[0]);  //write packet number as two byte number
  LoRa.write(packet_number_array[1]);
  LoRa.endPacket();
}

void printPictureData()
{
  File pictureFile = SPIFFS.open(FILE_PHOTO, "r"); // Open the picture file in read mode
  if (!pictureFile) {
    Serial.println("Failed to open picture file for reading");
    return;
  }
  Serial.print("\nPicture Data: ");

  while (pictureFile.available()) {
    char data = pictureFile.read();
    Serial.printf("%02X", (uint8_t)data);// Print each byte of the picture's data in hexadecimal format
  }
  
  Serial.println("\nDone");
  delay(1000);
  pictureFile.close(); // Close the picture file
}


void display_update(String string_num, String total_packets, int rssi, float snr) {
  char message[50]; char message2[50]; char message3[50]; char message4[50];
  sprintf(message, "Packet: %s",string_num);
  sprintf(message2, "Total Packets: %s", total_packets);
  sprintf(message3, "With RSSI: %d", rssi);
  sprintf(message4, "SNR: %f", snr);
  if (u8g2)
  {
    u8g2->clearBuffer();
    u8g2->drawStr(0,15, message);
    u8g2->drawStr(0,30, message2);
    u8g2->drawStr(0,45, message3);
    u8g2->drawStr(0,60, message4);
    u8g2->sendBuffer();
  }
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
  // Change sync word (0xF3) to match the receiver
  // The sync word assures you don't get LoRa messages from other LoRa transceivers
  // ranges from 0-0xFF
  LoRa.setSyncWord(0x6C);
  //enable error check, highly recommended
  LoRa.enableCrc();
  LoRa.setSignalBandwidth(bandwidth);
  LoRa.setSpreadingFactor(spread_factor);
  u8g2->clearBuffer();
  u8g2->drawStr(0,30, "LoRa Init Ok!");
  u8g2->sendBuffer();
  delay(500);

  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    ESP.restart();
  }

  EEPROM.begin(EEPROM_SIZE);
  EEPROM_position = EEPROM.read(0) + 1;
  EEPROM.write(0, EEPROM_position);
  EEPROM.commit();

  //clear SPIFFS
  SPIFFS.remove("/photo.jpg");
  Serial.println("Waiting for a new image");
  start_millis = millis();

}

void loop() {
  // try to parse packet
    int packet_size = LoRa.parsePacket();
    if (packet_size) 
    {
      while (LoRa.available()) {
        for (int i = 0; i < chunk_size; i++) {
          chunk[i] = LoRa.read();  //read packet byte by byte
        }
        byte packet_info[4];
        LoRa.readBytes(packet_info, 4);
        packet_number = (packet_info[0] + 256 * packet_info[1]); //read packet number
        total_packets = (packet_info[2] + 256 * packet_info[3]); //read total packets info
      }
      if (packet_number == 1 &&  !transmissionInProgress)  //this packet indicates start of new photo
      {
        new_photo();
        transmissionInProgress = true;
      }
      string_num = (String(packet_number)); 
      Serial.print("Got packet " + string_num + "/" + String(total_packets));
      rssi = String(LoRa.packetRssi());
      snr = String(LoRa.packetSnr());
      Serial.println(" with RSSI: " + rssi + ", SNR: " + snr + " ,packet length= " + String(packet_size));
      if (!file) {
        Serial.println("Failed to open file in writing mode");
      } 
      else 
      {
        for (int i = 0; i < chunk_size; i++) {
          file.write(chunk[i]);   //write to SPIFFS
          Serial.print(String(chunk[i],HEX)); //for debugging purposes
        }
      }
      display_update(string_num, String(total_packets), LoRa.packetRssi(), LoRa.packetSnr());
      current_packet_array[0] = packet_number % 256;
      current_packet_array[1] = packet_number / 256;
      send_ack(current_packet_array);
      if (packet_number == total_packets) {  //if received a final packet
        current_packet_array[0] = 0;
        current_packet_array[1] = 0;
        u8g2->clearBuffer();
        u8g2->drawStr(0,30, "Finished Transmission");
        u8g2->sendBuffer();
        printPictureData();
        transmissionInProgress = false; 
      }
    }
    if (transmissionInProgress)
    {
       current_millis = millis();
      if (current_millis - start_millis >= ack_retransmission_period)  //If no packet is received after a specified period, request retransmission
      {
        ack_retransmission_period = random(200, 400);
        Serial.println("Sending acknowledgment again for packet "+String(current_packet_array[0] + 256 * current_packet_array[1]));
        send_ack(current_packet_array);
        start_millis = current_millis;  //IMPORTANT to save the start time
      }
    }
}