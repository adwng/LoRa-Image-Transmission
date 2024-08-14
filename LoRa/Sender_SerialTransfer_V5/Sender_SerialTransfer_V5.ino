#define DEBUG_ESP              //comment out to deactivate debug console

#ifdef DEBUG_ESP
  #define pDBGln(x) Serial.println(x)
  #define pDBG(x)   Serial.print(x)
#else 
  #define pDBG(...)
  #define pDBGln(...)
#endif

#include "esp_camera.h"
#include "Arduino.h"
#include "soc/soc.h"           // Disable brownour problems
#include "soc/rtc_cntl_reg.h"  // Disable brownour problems
#include "driver/rtc_io.h"
#include "SerialTransfer.h"

//define LoRa pins
#define ESP32_TX_PIN 15
#define ESP32_RX_PIN 14

// Pin definition for CAMERA_MODEL_AI_THINKER
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

SerialTransfer myTransfer;

HardwareSerial Comm(1);
unsigned long start_millis = 0;
unsigned long current_millis = 0;
const int chunk_size = 100;

struct img_meta_data{
  uint16_t size;
  uint16_t packet_number;
  uint16_t total_packets;
  byte chunk[chunk_size];
} ImgMetaData;



void setup() {
   WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector
 
  Serial.begin(115200);                     // Define and start serial monitor
  // 115200,256000,512000,962100
  Comm.begin(962100, SERIAL_8N1,15,14);     //, Comm_Txd_pin, Comm_Rxd_pin); // Define and start Comm serial port
  myTransfer.begin(Comm);
 
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

  if(psramFound()){
  config.frame_size = FRAMESIZE_VGA; // FRAMESIZE_ + QVGA|CIF|VGA|SVGA|XGA|SXGA|UXGA
  config.jpeg_quality = 34; //10-63 lower number means higher quality
  config.fb_count = 1;
  } else {
    config.frame_size = FRAMESIZE_VGA;
    config.jpeg_quality = 34;
    config.fb_count = 1;
  }
 
  // Init Camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK){
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }
}

void loop() {
   uint16_t sendSize = 0;

  camera_fb_t * fb = NULL;
  //Take Picture with Camera
  fb = esp_camera_fb_get(); 

  ImgMetaData.size = fb->len;
  ImgMetaData.total_packets     = (ImgMetaData.size /chunk_size) + 1;   //(sizeof(myFb)/PIXELS_PER_PACKET) + 1; 

  for (ImgMetaData.packet_number = 1; ImgMetaData.packet_number <= ImgMetaData.total_packets; ImgMetaData.packet_number++)
  {
    memcpy(ImgMetaData.chunk, (fb->buf) + (chunk_size * (ImgMetaData.packet_number - 1)), chunk_size);

    sendSize = myTransfer.txObj(ImgMetaData, sizeof(ImgMetaData));

    myTransfer.sendData(sendSize);

    printBuf();

    delay(100);
  }

  esp_camera_fb_return(fb);
  delay(10000);
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

