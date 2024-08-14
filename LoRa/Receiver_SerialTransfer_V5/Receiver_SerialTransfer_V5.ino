#define DEBUG_ESP              // Comment out to deactivate debug console

#ifdef DEBUG_ESP
  #define pDBGln(x) Serial.println(x)
  #define pDBG(x)   Serial.print(x)
#else 
  #define pDBG(...)
  #define pDBGln(...)
#endif

#include "SerialTransfer.h"
#include "soc/soc.h"           // Disable brownour problems
#include "soc/rtc_cntl_reg.h"
#include "boards.h"
#include <LoRa.h>

SerialTransfer myTransfer;

long int bandwidth = 500000;
int spread_factor = 7;  //7-12
unsigned long start_millis = 0;
unsigned long current_millis = 0;
int ack_wait_timeout = 5000;
int SF_find_timeout = 3000;

byte total_packets_array[2];
byte packet_number_array[2];
const int chunk_size = 100;

struct img_meta_data{
  uint16_t size;
  uint16_t packet_number;
  uint16_t total_packets;
  byte chunk[chunk_size];
} ImgMetaData;

uint16_t bufferPointer = 0;
byte tempImageBuffer[60000];
uint16_t packetCounter=1;

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
    for (int i = 0; i < 9; i++)
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

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  Serial.begin(115200); // Define and start serial monitor
  Serial2.begin(962100, SERIAL_8N1, 14, 15); // Receiver_Txd_pin, Receiver_Rxd_pin); // Define and start Receiver serial port
  myTransfer.begin(Serial2);

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
}

void loop() {
  if (myTransfer.available()) {
    myTransfer.rxObj(ImgMetaData, sizeof(ImgMetaData));
    
    if (ImgMetaData.packet_number == 1)
    {
      packetCounter = 1;
      bufferPointer = 0;
      copyToImageBuff(sizeof(ImgMetaData.chunk));
    }else
    {
      if (ImgMetaData.packet_number == packetCounter)
      {
        copyToImageBuff(sizeof(ImgMetaData.chunk));
      }
    }
    if (packetCounter > ImgMetaData.total_packets)
    {
      pDBGln("Transmitting Now");
      sendImagetoLora();
      packetCounter = 1;
      bufferPointer = 0;
    }
  
    printBuf();
  }
}

void copyToImageBuff(uint16_t dataLength){
  pDBGln("Copying");
  for(int y=0;y<dataLength;y++){
    tempImageBuffer[bufferPointer+y] = ImgMetaData.chunk[y];
  } 
  bufferPointer += dataLength;
  packetCounter++;  

  pDBG("dataLenght: ");
  pDBG(dataLength);  
  pDBG(", bufferPointer: ");
  pDBGln(bufferPointer);
}

void send_chunk(int packet_number) 
{
  uint8_t chunk[chunk_size];
  memcpy(chunk, tempImageBuffer + (chunk_size * (packet_number - 1)), chunk_size);
  Serial.println("Sending:" + String(packet_number) + "packet");

  LoRa.beginPacket();
  for (int j = 0; j < chunk_size; j++) 
  {
    LoRa.write(chunk[j]);  //sending the payload
  }
  LoRa.write(packet_number_array[0]);  //write packet number as two-byte number
  LoRa.write(packet_number_array[1]);
  LoRa.write(total_packets_array[0]);  //write total packets count as two-byte number
  LoRa.write(total_packets_array[1]);
  LoRa.endPacket();
}

void sendImagetoLora()
{
  total_packets_array[0] = ImgMetaData.total_packets % 256;
  total_packets_array[1] = ImgMetaData.total_packets / 256;

  for (int i = 1; i <= ImgMetaData.total_packets; i++)
  {
    packet_number_array[0] = i % 256;
    packet_number_array[1] = i / 256;
    send_chunk(i);

    while (wait_for_ack(i) == 0) {
      //retransmit the packet until ack is OK
      send_chunk(i);
    }
  }
}

void printBuf() {
  pDBG(F("Packet Info: "));
  pDBG(F("Packet Number: "));
  pDBG(ImgMetaData.packet_number);
  pDBG(F(", Total Packets: "));
  pDBG(ImgMetaData.total_packets);
  pDBGln();
  pDBG(F("Chunk Data: "));
  for (int i = 0; i < chunk_size; i++) {
    pDBG(ImgMetaData.chunk[i]);
    pDBG(F(", "));
  }
  pDBGln();
}

