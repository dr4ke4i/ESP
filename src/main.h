// CONFIG file
#ifndef __INCLUDE_MAIN_H
#define __INCLUDE_MAIN_H

#define Profile 4

// Спальня  (Profile 1 // 192.168.1.132)
// Гостиная (Profile 2 // 192.168.1.122)
// Ванная  (Profile 3 // 192.168.1.94)
// Testing  (Profile 4 // 192.168.1.94)

#define SCL_PIN             D1       // GPIO5/SCL
#define SDA_PIN             D2       // GPIO4/SDA
#define ONEWIRE_PIN         D3       // GPIO0/ONE-Wire (DS18b20,...)

#define UPDATE_TIME         3000

#if Profile == 1
  #define ESPnode "ESP1"
  #define enableLEDbuiltin              // uses GPIO02 (D4)
  #define enableMQTTNumbers
  #define enableDHT
  // #define enableBMP180
  #define enableRGB  
  #define enableLUX
  // #define enableDS18B20              // uses ONEWIRE_PIN 
  // #define enableBinarySensors
  // #define enableMQTTswitches
  #define DHT_DATA_PIN      D5       // GPIO14
  #define WS2812B_DATA_PIN  D6       // GPIO12
  #define NUM_LEDS          150
#elif Profile == 2
  #define ESPnode "ESP2"
  #define enableLEDbuiltin              // uses GPIO02 (D4)
  #define enableMQTTNumbers
  #define enableDHT
  #define enableBMP180
  #define enableRGB
  #define enableLUX
  #define enableDS18B20              // uses ONEWIRE_PIN   
  // #define enableBinarySensors
  // #define enableMQTTswitches
  #define DHT_DATA_PIN      D5       // GPIO14
  #define WS2812B_DATA_PIN  D6       // GPIO12
  #define NUM_LEDS          300
#elif Profile == 3
  #define ESPnode "ESP3"
  // #define enableLEDbuiltin              // uses GPIO02 (D4)
  // #define enableMQTTNumbers
  #define enableDHT
  // #define enableBMP180
  // #define enableRGB
  // #define enableLUX                  // uses SDA_PIN, SCL_PIN
  // #define enableDS18B20              // uses ONEWIRE_PIN
  #define enableBinarySensors
  #define enableMQTTswitches
  #define DHT_DATA_PIN      D5       // GPIO14
  #define NUM_LEDS          3
  #define WS2812B_DATA_PIN  D6      // GPIO12   
#elif Profile == 4
  #define ESPnode "ESP4"
  #define enableLEDbuiltin              // uses GPIO02 (D4)
  #define enableMQTTNumbers
  // #define enableDHT
  // #define enableBMP180
  #define enableRGB
  // #define enableLUX                  // uses SDA_PIN, SCL_PIN
  // #define enableDS18B20              // uses ONEWIRE_PIN
  // #define enableBinarySensors
  // #define enableMQTTswitches
  #define DHT_DATA_PIN      D5       // GPIO14
  #define NUM_LEDS          30
  #define WS2812B_DATA_PIN  D6      // GPIO12  
#endif

#define MSG_BUFFER_SIZE	512
#define AUX_BUFFER_SIZE 100
extern char msg[];
extern char buf[];
extern const char* strESPstateTopic;
extern const char* strESPdebugOTAstate;
extern const char* strHAonlineTopic;

#ifdef enableDS18B20                // or if defined any other sensor that uses OneWire
  #include <OneWire.h>
  extern OneWire onewire;
#endif

#endif