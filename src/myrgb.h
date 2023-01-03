#ifndef __INCLUDE_MYRGB_H
#define __INCLUDE_MYRGB_H

#define GUARANTEE_THRESHOLD 64

#include "Arduino.h"
#include "main.h"
#include "mytimer.h"
#include <FastLED.h>
#include <ArduinoJson.h>

struct MYRGB {
    public:
        MYRGB(unsigned long _timeout);  

        enum MQTT {
            cfg = 1,
            get,
            set
        };

        const char* getTopic(MQTT _choice);
        void getPayload(char (&_msg)[], MQTT _choice);
        
        struct MYDATA {
            bool     enabled;
            uint8_t  red;
            uint8_t  green;
            uint8_t  blue;
            uint8_t  brightness;
            uint16_t transitionTime;
            uint16_t speed;
            uint16_t quantity;
            uint16_t hue;
            int      effectIndex;
            const char* effectName;            
            unsigned long   millis;
        };
        const char* effectsList[5] = { "Plain", "Fire", "Sparks", "Rainbow", "Sensor" };
        enum EFFINDEX { EFF_PLAIN = 0, EFF_FIRE, EFF_SPARKS, EFF_RAINBOW, EFF_SENSOR, EFF_TRANSITION };

        friend bool operator== (const MYDATA &_lhs, const MYDATA &_rhs);
        friend bool operator!= (const MYDATA &_lhs, const MYDATA &_rhs);

        MYRGB::MYDATA state(void);                                 // get
        bool state(MYRGB::MYDATA _newState);                      // set

        bool stateChanged(void);                            // get        
        bool stateUndefined(void);                          // get
        void stateUndefined(bool _newStateUndefined);       // set
        bool undefinedTimeout(void);

        bool parsePayload(const String &_payload);
        bool parsePayload(const char (&_payload)[]);

        //do some things each cycle (e.g. redraw or update sensor state)
        void update(void);
    
    private:
        CRGB leds_[NUM_LEDS];
        CRGB led_rgb;
        CHSV led_hsv;
        StaticJsonDocument<512> jsonDoc_;
        MYTIMER tmrTimeout, tmrTransition, tmrFade;

        MYDATA state_, lastState_, startState_, endState_, effectState_, plainState_;
        bool stateUndefined_;
        MYDATA get_(void);
        void get_(MYDATA &_state);
        void set_(const MYDATA &_state);
        CRGB setColor(const MYDATA &_state);
        void setDitherFill(const MYDATA &_state);   // uses leds_[]
        void setDitherFill(uint32_t _red, uint32_t _green, uint32_t _blue, uint32_t _brightness);   // uses leds_[]

        const char* strRGBgetTopic      = "homeassistant/light/" ESPnode "/RGB/get";
        const char* strRGBsetTopic      = "homeassistant/light/" ESPnode "/RGB/set";
        const char* strRGBcfgTopic      = "homeassistant/light/" ESPnode "/RGB/config";
        const char* strRGBcfgPayload    = "{ \"platform\": \"mqtt\","
                                          "  \"schema\": \"json\","
                                          "  \"name\": \"" ESPnode " WS2812b LEDstrip\","
                                          "  \"state_topic\": \"%s\","
                                          "  \"command_topic\": \"%s\","
                                          "  \"availability_topic\": \"%s\","
                                          "  \"brightness\": \"true\","
                                          "  \"color_mode\": \"true\","
                                          "  \"supported_color_modes\": [\"rgb\"],"
                                          "  \"effect\": \"true\","
                                          "  \"effect_list\":"
                                            "["
                                            "\"Plain\""
                                            ","
                                            // "\"Fire\""
                                            // ","
                                            "\"Sparks\""
                                            // ","
                                            // "\"Rainbow\""
                                            // ","
                                            // "\"Sensor\""
                                            "]"
                                          "}";
};

#endif