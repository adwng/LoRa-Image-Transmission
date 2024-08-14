#include "esp_camera.h"
#include "HardwareSerial.h"

//define LoRa pins
#define ESP32_TX_PIN 1
#define ESP32_RX_PIN 3

#define SS 15
#define RST -1
#define DIO0 2
#define SCK 14
#define MISO 12
#define MOSI 13
//define camera pins
#define PWDN_GPIO_NUM 32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 0
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27
#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22

int instance;
static int chunk_size = 250;  //size of jpeg data in a single packet, max 250
int total_packets = 0;
unsigned long start_millis = 0;
unsigned long current_millis = 0;
int ack_wait_timeout = 5000;
byte total_packets_array[2];
byte packet_number_array[2];

HardwareSerial SerialPort(0); // use UART0


void send_chunk(int packet_number, camera_fb_t* fb) 
{
  uint8_t chunk[chunk_size];
  memcpy(chunk, (fb->buf) + (chunk_size * (packet_number - 1)), chunk_size);
  
  for (int j = 0; j < chunk_size; j++) 
  {
    SerialPort.println("Sending: " +  String(packet_number) + "|" + chunk[j]);  //sending the payload
  }
}

int wait_for_ack()  //return 1 for OK, return 0 for retransmission
{
  start_millis = millis();
  while (true) {
    current_millis = millis();
    if (current_millis - start_millis >= ack_wait_timeout) {
      start_millis = current_millis;
    }
    String data = SerialPort.readStringUntil('\n');

    int index = data.indexOf("Status: ");
    if (index != -1)
    {
      index += strlen("Status: ");
      String ack = data.substring(index);
      if (ack == "Pending"){
        return 0;
      }
      else if (ack = "OK!")
      {
        return 1;
      }
    }
    
  }
}

void take_pic(bool transmit) {
  if (transmit == false) {  //takes a picture without transmission
    Serial.println("Taking pic ");
    camera_fb_t* fb = NULL;
    fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
      Serial.println("Restarting");
      ESP.restart();
      return;
    }
    int size = fb->len;
    SerialPort.println("Size of picture is: " + String(size) + " bytes");
    esp_camera_fb_return(fb);
    delay(1000);
  }
  if (transmit == true) {  //takes and transmits a picture
    Serial.println("Transmission is True");
    camera_fb_t* fb = NULL;
    fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
      Serial.println("Restarting");
      ESP.restart();
      return;
    }
    int size = fb->len;  //gets size of jpeg
    total_packets = (size / chunk_size) + 1;
    SerialPort.println("Size: " + String(size) + ";" + "Total Packets: " + String(total_packets));
    while (wait_for_ack() == 0) {
      //retransmit the packet until ack is OK
      SerialPort.println("Size: " + String(size) + ";" + "Total Packets: " + String(total_packets));
    }
    total_packets_array[0] = total_packets % 256;
    total_packets_array[1] = total_packets / 256;
    for (int i = 1; i <= total_packets; i++) {  //sends all the chunks
      packet_number_array[0] = i % 256;
      packet_number_array[1] = i / 256;
      send_chunk(i, fb);

      while (wait_for_ack() == 0) {
        //retransmit the packet until ack is OK
        send_chunk(i, fb);
      }
    }
    Serial.println("Finished transimssion");
    esp_camera_fb_return(fb);
  }
}

void setup() {
  Serial.begin(9600, SERIAL_8N1, ESP32_RX_PIN, ESP32_TX_PIN);
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  if (psramFound()) {
    config.frame_size = FRAMESIZE_HD;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }
  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  sensor_t* s = esp_camera_sensor_get();
  start_millis = millis();
}

void loop() {
  if (instance == 0)  //if movement detected
  {
    take_pic(false); //taking dummy pictures helps calibrate the camera for light conditions
    delay(1000);
    take_pic(false);
    delay(1000);
    take_pic(true);  //this picture will be transmitted
    instance++;
  }
}
