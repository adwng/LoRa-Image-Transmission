
#pragma once

#define LILYGO_T3_V1_6


/*
* if you need to change it,
* please open this note and change to the frequency you need to test
* Option: 433E6,470E6,868E6,915E6
* */

#define LoRa_frequency      433E6


#define UNUSE_PIN                   (0)


#if defined(LILYGO_T3_V1_6)
#define I2C_SDA                     21
#define I2C_SCL                     22
#define OLED_RST                    UNUSE_PIN

#define RADIO_SCLK_PIN              5
#define RADIO_MISO_PIN              19
#define RADIO_MOSI_PIN              27
#define RADIO_CS_PIN                18
#define RADIO_DIO0_PIN               26
#define RADIO_RST_PIN               23
#define RADIO_DIO1_PIN              33
#define RADIO_BUSY_PIN              32

#define SDCARD_MOSI                 15
#define SDCARD_MISO                 2
#define SDCARD_SCLK                 14
#define SDCARD_CS                   13

#define BOARD_LED                   25
#define LED_ON                      HIGH

#define ADC_PIN                     35

#define HAS_SDCARD
#define HAS_DISPLAY


#else
#error "For the first use, please define the board version and model in <utilities. h>"
#endif









