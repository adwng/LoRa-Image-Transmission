#include <LoRa.h>
#include "boards.h"
#include <WiFi.h>
#include "time.h"
#include <Arduino.h>
#include "EmonLib.h"
 

// The GPIO pin were the CT sensor is connected to (should be an ADC input)
#define ADC_INPUT 34

// The voltage in our apartment. Usually this is 230V for Europe, 110V for US.
// Ours is higher because the building has its own high voltage cabin.
#define HOME_VOLTAGE 220.0

// Force EmonLib to use 10bit ADC resolution
#define ADC_BITS    10
#define ADC_COUNTS  (1<<ADC_BITS)

#define DHT11_PIN 21
#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP          /* Time ESP32 will go to sleep (in seconds) */

#define LED  23

EnergyMonitor emon1;

// const int trigPin = 25;
// const int echoPin = 4;
// const int sensor_pin = 13;    //Soil moisture sensor O/P pin /
// int distance;

unsigned long stopTime = 0;
unsigned long currentTime = 0;
unsigned long startTime = 0;
unsigned long runTime = 0;
unsigned long calRunTime = 0;
unsigned long offTime = 0;
unsigned long remainTime = 0;

unsigned long currentMillis;
unsigned long prevMillis;

// Variable to save current epoch time
unsigned long epochTime; 

double amps;
double watt;
int offset = 0;

// Replace with your network credentials
const char* ssid = "Andrew";
const char* password = "andrewng";

// const char* ssid = "Thomas";
// const char* password = "honhonhon123";

// const char* ssid = "Mehehehhe";
// const char* password = "2510yasirah";

// NTP server to request epoch time
const char* ntpServer = "pool.ntp.org";

// Function that gets current epoch time
unsigned long getTime() {
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    //Serial.println("Failed to obtain time");
    return(0);
  }
  // Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  time(&now);
  return now;
}

// Function to format time into dd-mm-yyyy
String formatDate(const time_t& time) {
  char buffer[20];  // Buffer to hold the formatted date
  struct tm * timeinfo = localtime(&time);
  strftime(buffer, sizeof(buffer), "%H:%M:%S", timeinfo);
  return String(buffer);
}

// Initialize WiFi
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

void setup()
{
  Serial.begin(115200);
  delay(100);
  pinMode(LED,OUTPUT);
  digitalWrite(LED,0);
  analogReadResolution(10);
  emon1.current(ADC_INPUT, 111.1);
  // pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  // pinMode(echoPin, INPUT);  // Sets the echoPin as an Input
  
  initBoard();
  initWiFi();
    // When the power is turned on, a delay is required.
  delay(1500);
  
  configTime(28800, 0, ntpServer);

  Serial.println("LoRa Sender");
  LoRa.setPins(RADIO_CS_PIN, RADIO_RST_PIN, RADIO_DIO0_PIN);
  if (!LoRa.begin(LoRa_frequency)) {
      Serial.println("Starting LoRa failed!");
      while (1);
  }

}

void loop()
{
  Serial.print("Sending packet: ");

  //Timestamp
  epochTime = getTime();

  String time = formatDate(epochTime);

  currentMillis = millis();

  // If it's been longer then 1000ms since we took a measurement, take one now!
  if(currentMillis - prevMillis > 1000){
    amps = emon1.calcIrms(1480); // Calculate Irms only
    watt = amps * HOME_VOLTAGE;

    prevMillis = millis();
    if (amps < 1.0 && offset == 0)
    {
      offset = 1;
    }
    if (amps > 1.5 && offset == 1)
    {
      currentTime = millis();
      runTime = runTime + currentTime - startTime;
      startTime = currentTime; 
    }
    else
    {
      startTime = millis();
    }
  }
  remainTime = 3*60*1000 - runTime;
  char message[100];
  sprintf(message,"Time,%s;AMPS,%.2f;WATTS,%.2f;RunTime,%d;Remain,%d",time, amps, watt, runTime,remainTime);
  Serial.println(message);


  // Send packet
  LoRa.beginPacket();
  LoRa.print(message);
  LoRa.endPacket();

  
  Serial.print(message);
  Serial.println();

  if (u8g2) {
      u8g2->clearBuffer();
      u8g2->drawStr(0, 15, "package sent");
      // u8g2->drawStr(0,30, message1);
      u8g2->sendBuffer();
  }
}

