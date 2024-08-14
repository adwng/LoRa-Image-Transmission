#include <LoRa.h>
#include "boards.h"


const int trigPin = 2;
const int echoPin = 4;

#define CM_TO_INCH 0.393701

void setup()
{
  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin, INPUT);  // Sets the echoPin as an Input
  
  initBoard();
    // When the power is turned on, a delay is required.
  delay(1500);

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
      String distances = ultrasonicSensor();
      // Send packet
      LoRa.beginPacket();
      LoRa.print(distances);
      LoRa.endPacket();

      
      Serial.print(distances);
      Serial.println();

      const char *ptr = distances.c_str();
      int values[2];
      for (int i = 0; i < 2; i++) {
        ptr = strchr(ptr, ':'); // Find the ':' character
        if (ptr != NULL) {
          values[i] = atoi(ptr + 1); // Convert the number after ':' to integer
          ptr++; // Move pointer to the next character
        }
      }

      char message[50]; char message1[50];

      sprintf(message, "Distance: %d cm", values[0]);
      sprintf(message1, "Distance: %d in", values[1]);


      if (u8g2) {
          u8g2->clearBuffer();
          u8g2->drawStr(0, 15, message);
          u8g2->drawStr(0,30, message1);
          u8g2->sendBuffer();
      }
      delay(1000);
}

String ultrasonicSensor() {
      // Clears the trigPin
      digitalWrite(trigPin, HIGH);
      delayMicroseconds(10);
      // Sets the trigPin on HIGH state for 10 microseconds
      digitalWrite(trigPin, LOW);
    
      // Reads the echoPin, returns the sound wave travel time in microseconds
      long duration = pulseIn(echoPin, HIGH);
    
      // Calculate the distance in centimeters
      float distanceCm = (duration/2)/29;
    
      // Convert to inches
      float distanceInch = distanceCm * CM_TO_INCH;
    
      // Round the distances
      String roundedCm = String(round(distanceCm));
      String roundedInch = String(round(distanceInch));
      
      String distances = "cm:" + roundedCm + ", inch:" + roundedInch;


      return distances;
      
  }
