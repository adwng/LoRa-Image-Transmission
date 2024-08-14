#include "boards.h"
#include <LoRa.h>

static int chunk_size = 250;  //size of jpeg data in a single packet, max 250
long int bandwidth = 500000;
int spread_factor = 7;  //7-12
unsigned long start_millis = 0;
unsigned long current_millis = 0;
int ack_wait_timeout = 5000;
int SF_find_timeout = 300; //increase this value up to 30000 if transmission is stopped after BW of SF changes. 
byte packet_number_array[2];
int packet_number = 0;
int instance;

void send_chunk(int packet_number, uint8_t* data, size_t size) 
{
  uint8_t chunk[chunk_size];
  memcpy(chunk, data + (chunk_size * (packet_number - 1)), chunk_size);
  Serial.println("Sending:" + String(packet_number) + "packet");

  display_update(packet_number);

  LoRa.beginPacket();
  for (int j = 0; j < chunk_size; j++) 
  {
    LoRa.write(chunk[j]);  //sending the payload
  }
  LoRa.write(packet_number_array[0]);  //write packet number as two-byte number
  LoRa.write(packet_number_array[1]);
  LoRa.endPacket();
}

int findSF() 
{
  start_millis = millis();
  for (int sf = 7; sf <= 10; sf++)
  {
    Serial.printf("\nTrying SF: %d", sf);
    LoRa.setSpreadingFactor(sf);
    while (true) {
      int packet_size = LoRa.parsePacket();
      if (packet_size) {
        Serial.printf("\nSF is %d\n", sf);
        return 1;
      }
      current_millis = millis();
      if (current_millis - start_millis >= SF_find_timeout) {
        break;
      }
    }
    start_millis = millis();
  }
  return 0;
}

void find_BW() 
{
  Serial.println("I have got no acknowledgement from the receiver in "+String(ack_wait_timeout/1000)+" seconds.");
  int BWarray[] = {7800, 10400, 15600, 20800, 31250, 41700, 62500, 125000, 250000, 500000};
  while (true) {
    for (int i = 0; i < sizeof(BWarray); i++)
    {
      LoRa.setSignalBandwidth(BWarray[i]);
      Serial.printf("\nTrying BW: %d",BWarray[i]);
      if (findSF() == 1) {
        return;
      }
    }
  }
}

int wait_for_ack(int packet_number)  //return 1 for OK, return 0 for retransmission
{
  start_millis = millis();
  while (true) 
  {
    current_millis = millis();
    if (current_millis - start_millis >= ack_wait_timeout) 
    {
      find_BW();
      start_millis = current_millis;
    }
    int packet_size = LoRa.parsePacket();
    if (packet_size) 
    {
      byte packet_confirmation_array[2];
      while (LoRa.available()) {
        LoRa.readBytes(packet_confirmation_array, 2);
        int packet_confirmation = packet_confirmation_array[0] + 256 * packet_confirmation_array[1];
        if (packet_confirmation != packet_number) 
        {
          Serial.println(
            "Expected confirmation for packet " + String(packet_number) + ", got confirmation for packet " + String(packet_confirmation) 
            + ". Sending packet " + String(packet_number) + " again..."
          );

          char message[50]; 
          sprintf(message, "Expecting Packet: %d", packet_number);
          u8g2->clearBuffer();
          u8g2->drawStr(0,30, message);

          return 0;
        } 
        else 
        {
          Serial.println(String(packet_confirmation) + " OK!");
          return 1;
        }
      }
    }
  }
}

void display_update(int num) 
{
  char message[50];
  sprintf(message, "Packet Sent: %d",num);
  if (u8g2)
  {
    u8g2->clearBuffer();
    u8g2->drawStr(0,15, message);
    u8g2->sendBuffer();
  }
}


void sensor_data_sending() 
{

  if (wait_for_ack() == 0)
  {
    send_chunk(i);
  }


  Serial.println("Finished transmission");

  u8g2->clearBuffer();
  u8g2->drawStr(0, 30, "Data Sent");
  u8g2->sendBuffer();
}

void setup() 
{
  initBoard();
  delay(1500);

  Serial.begin(115200);
  Serial.println("LoRa Sender");

  LoRa.setPins(RADIO_CS_PIN, RADIO_RST_PIN, RADIO_DIO0_PIN);
  if (!LoRa.begin(LoRa_frequency)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  } 
  // Change sync word (0xF3) to match the receiver
  // The sync word assures you don't get LoRa messages from other LoRa transceivers
  // ranges from 0-0xFF
  LoRa.setSyncWord(0x6C);
  LoRa.enableCrc();
  LoRa.setSignalBandwidth(bandwidth);
  LoRa.setSpreadingFactor(spread_factor);
  if (u8g2)
  {
    u8g2->clearBuffer();
    u8g2->drawStr(0,30, "LoRa Init Done");
  }
  start_millis = millis();
}

void loop() {
  if (instance == 0)  //if movement detected
  {
    sensor_data_sending();  //this picture will be transmitted
  }
}