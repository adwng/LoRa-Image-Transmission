#include <LoRa.h>
#include "boards.h"
#include <WiFi.h>
#include "time.h"
#include <Arduino.h>
#include "EmonLib.h"

// Define constants
#define ADC_INPUT 34
#define HOME_VOLTAGE 220.0
#define ADC_BITS 10
#define ADC_COUNTS (1<<ADC_BITS)
#define DHT11_PIN 21
#define uS_TO_S_FACTOR 1000000
#define TIME_TO_SLEEP 60  // Adjust as needed
#define LED 23

EnergyMonitor emon1;

// Network credentials
const char* ssid = "Andrew";
const char* password = "andrewng";
const char* ntpServer = "pool.ntp.org";

// Function prototypes
unsigned long getTime();
String formatDate(const time_t& time);
void initWiFi();
int wait_ack();

unsigned long epochTime; 
double amps;
double watt;
int offset = 0;
int packet_number = 1;

unsigned long currentMillis;
unsigned long prevMillis;
unsigned long startTime = 0;
unsigned long runTime = 0;
unsigned long remainTime = 0;

void setup() {
  Serial.begin(115200);
  delay(100);
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);
  analogReadResolution(10);
  emon1.current(ADC_INPUT, 111.1);

  initBoard();
  initWiFi();
  delay(1500);
  configTime(28800, 0, ntpServer);

  Serial.println("LoRa Sender");
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
  Serial.print("Sending packet: ");

  // Timestamp
  epochTime = getTime();
  String time = formatDate(epochTime);

  currentMillis = millis();
  if (currentMillis - prevMillis > 1000) {
    amps = emon1.calcIrms(1480); // Calculate Irms
    watt = amps * HOME_VOLTAGE;
    prevMillis = millis();

    if (amps < 1.0 && offset == 0) {
      offset = 1;
    }
    if (amps > 1.5 && offset == 1) {
      startTime = millis();
    } else {
      runTime += millis() - startTime;
      startTime = millis();
    }
  }

  remainTime = 3 * 60 * 1000 - runTime;
  char message[100];
  sprintf(message, "ID,%d;Time,%s;AMPS,%.2f;WATTS,%.2f;RunTime,%lu;Remain,%lu", packet_number, time.c_str(), amps, watt, runTime, remainTime);

  LoRa.beginPacket();
  LoRa.print(message);
  LoRa.endPacket();

  if (wait_ack() == 1) {
    packet_number++;
  }

  Serial.println(message);
}

unsigned long getTime() {
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return 0;
  }
  time(&now);
  return now;
}

String formatDate(const time_t& time) {
  char buffer[20];
  struct tm * timeinfo = localtime(&time);
  strftime(buffer, sizeof(buffer), "%H:%M:%S", timeinfo);
  return String(buffer);
}

void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
}

int wait_ack() {
  unsigned long startTime = millis();
  while (millis() - startTime < 500) { // 5 seconds timeout
    int packet_size = LoRa.parsePacket();
    if (packet_size) {
      byte packet_confirmation_array[2];
      while (LoRa.available()) {
        LoRa.readBytes(packet_confirmation_array, 2);
        int packet_confirmation = packet_confirmation_array[0] + (256 * packet_confirmation_array[1]);
        if (packet_confirmation == packet_number) {
          Serial.println(String(packet_confirmation) + " OK!");
          return 1;
        } else {
          Serial.println("Expected confirmation for packet " + String(packet_number) + ", got confirmation for packet " + String(packet_confirmation));
        }
      }
    }
  }
  Serial.println("Acknowledgment timeout.");
  return 0;
}
