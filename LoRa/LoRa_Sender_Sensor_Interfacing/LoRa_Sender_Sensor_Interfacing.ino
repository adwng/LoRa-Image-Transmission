#include <LoRa.h>
#include "boards.h"
#include <dht.h>
#include <WiFi.h>
#include "time.h"

#define DHT11_PIN 21
#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  60        /* Time ESP32 will go to sleep (in seconds) */

#define LED  23

const int trigPin = 25;
const int echoPin = 4;
const int sensor_pin = 13;    //Soil moisture sensor O/P pin /
int distance;

dht DHT;

// Replace with your network credentials
const char* ssid = "Andrew";
const char* password = "andrewng";

// NTP server to request epoch time
const char* ntpServer = "pool.ntp.org";

// Variable to save current epoch time
unsigned long epochTime; 

// Variable to save start of epoch time
RTC_DATA_ATTR unsigned long start_epochTime;
RTC_DATA_ATTR int bootCount = 0;

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

float ultrasonicSensor() {

  digitalWrite(trigPin, HIGH); 

  delayMicroseconds(10);                  // must keep the trig pin high for at least 10us 

  digitalWrite(trigPin, LOW); 

  int ultrasonic2Duration = 0;
  int ultrasonic2Distance = 0;


  ultrasonic2Duration = pulseIn(echoPin, HIGH); 

  ultrasonic2Distance = (ultrasonic2Duration/2)/29; 

  return ultrasonic2Distance;
}

void print_wakeup_reason(){
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;
  }
}

void setup()
{
  Serial.begin(115200);
  delay(100);
  pinMode(LED,OUTPUT);
  digitalWrite(LED,0);

  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  Serial.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) + " Seconds");

  //Print the wakeup reason for ESP32
  print_wakeup_reason();

  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin, INPUT);  // Sets the echoPin as an Input

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

  sendMessage();

  ++bootCount;

  esp_deep_sleep_start();
}

void sendMessage()
{
  Serial.print("Sending packet: ");
  float moisture_percentage;
  int sensor_analog;
  char message[100];

  if (bootCount == 0)
  {
    start_epochTime = getTime();
  }


  //Read distance
  distance = ultrasonicSensor();

  //Read moisture
  sensor_analog = analogRead(sensor_pin);
  moisture_percentage = ( 100 - ( (sensor_analog/1023.00) * 100 ) );

  //Get Timestamp
  epochTime = getTime();

  //Format Time
  String start_Time = formatDate(start_epochTime);
  String time = formatDate(epochTime);

  //Get DHT
  int chk = DHT.read11(DHT11_PIN);
  int temp = DHT.temperature;
  float hum = DHT.humidity;

  

  sprintf(message, "Timestamp, %s;ID, %d;Difference, %s;Distance, %d;Temperature, %d;Humidity, %.2f",
          time, bootCount, time_diff.c_str(), distance, temp, hum);

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

  digitalWrite(LED,1);
  delay(1000);
  digitalWrite(LED,0);
}

void loop()
{

}

// if current > threshold{ add 30 minutes to RTC buffer, } else { no add}

// 