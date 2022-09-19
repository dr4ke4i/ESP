// CONFIG file
#ifndef __INCLUDE_MAIN_H
#define __INCLUDE_MAIN_H

#define Profile 2

// Спальня  (Profile 1 // 192.168.1.132)
// Гостиная (Profile 2 // 192.168.1.122)
// Unknown  (Profile 3 // 192.168.1.94)

#define SCL_PIN             D1       // GPIO5/SCL
#define SDA_PIN             D2       // GPIO4/SDA

#define UPDATE_TIME         3000

#if Profile == 1
  #define ESPnode "ESP1"
  #define enableDHT
  #define enableLUX
  #define enableRGB  
  #define DHT_DATA_PIN      D5       // GPIO14
  #define WS2812B_DATA_PIN  D6       // GPIO12
  #define NUM_LEDS          150
#elif Profile == 2
  #define ESPnode "ESP2"
  #define enableDHT
  #define enableRGB  
  #define DHT_DATA_PIN      D5       // GPIO14
  #define WS2812B_DATA_PIN  D6       // GPIO12
  #define NUM_LEDS          300
#elif Profile == 3
  #define ESPnode "ESP3"
  #define enableDHT
  #define DHT_DATA_PIN      D5       // GPIO14
  #define enableLUX                  // uses SDA_PIN, SCL_PIN
  #define enableRGB
  #define NUM_LEDS          3
  #define WS2812B_DATA_PIN  D6      // GPIO12  
#endif

#define MSG_BUFFER_SIZE	512
extern char msg[];
extern const char* strESPstateTopic;
extern const char* strESPdebugOTAstate;
extern const char* strHAonlineTopic;

#endif