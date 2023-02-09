#ifndef __INCLUDE_MYRGB_H
#define __INCLUDE_MYRGB_H

#define GUARANTEE_THRESHOLD     64
#define DEFAULT_TRANSITION_TIME 1500
#define ENABLE_EFF_SPARKS       1
#define ENABLE_EFF_RAINBOW      0
#define ENABLE_EFF_FIRE         0
#define MAX_EFFECTS             (ENABLE_EFF_RAINBOW + ENABLE_EFF_SPARKS + ENABLE_EFF_FIRE + 1)

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
        
        typedef uint16_t PARAM_T;

        struct MYDATA {
            bool    enabled;
            union   {   uint8_t  red;
                        uint8_t  r;      };
            union   {   uint8_t  green;
                        uint8_t  g;      };
            union   {   uint8_t  blue;
                        uint8_t  b;      };
            union   {   uint8_t  brightness;
                        uint8_t  br;     };
            bool    transition;
            union   {   unsigned long  transitionTime;
                        unsigned long  tt;      };
            int         effectIndex;
            const char* effectName;                                   
            PARAM_T     param[4];      // SPARKS:  b_rate[0], b_period[1], hue_width[2], fade_period[3]
        };

        const char* effectsList[MAX_EFFECTS] = { "Plain"
            #if ENABLE_EFF_SPARKS==1
                , "Sparks"            
            #endif
            #if ENABLE_EFF_RAINBOW==1        
                , "Rainbow"
            #endif
            #if ENABLE_EFF_FIRE==1
                , "Fire"
            #endif
            };

        enum EFFINDEX { EFF_PLAIN = 0
            #if ENABLE_EFF_SPARKS==1
                , EFF_SPARKS            
            #endif
            #if ENABLE_EFF_RAINBOW==1        
                , EFF_RAINBOW
            #endif
            #if ENABLE_EFF_FIRE==1
                , EFF_FIRE
            #endif
            };
        
        PARAM_T     getParam(int _i);
        void        setParam(const PARAM_T& _value, int _i);

        friend bool operator== (const MYDATA &_lhs, const MYDATA &_rhs);
        friend bool operator!= (const MYDATA &_lhs, const MYDATA &_rhs);

        MYRGB::MYDATA state(void);                                 // get
        bool state(MYRGB::MYDATA _newState);                       // set

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
        CRGB buf_[NUM_LEDS];
        uint8_t bright_[NUM_LEDS];
        CRGB led_rgb;   CHSV led_hsv;
        StaticJsonDocument<512> jsonDoc_;
        MYTIMER tmrTimeout, tmrTransition, tmrFade;

        MYDATA profile_[MAX_EFFECTS];
        MYDATA fromstate_, currentstate_, tostate_;
        // MYDATA state_, lastState_, startState_, endState_, effectState_, plainState_;
        bool stateUndefined_;
        bool stateChanged_;
        MYDATA get_(void);
        void get_(MYDATA &_state);
        void set_(const MYDATA &_state);
        CRGB setColor(uint8_t _red, uint8_t _green, uint8_t _blue, uint8_t _brightness);
        CRGB setColor(const MYDATA &_state);
        CRGB setColor(const CRGB &_rgb, uint8_t _brightness);
        void setDitherFill(uint16_t _red, uint16_t _green, uint16_t _blue, uint16_t _brightness, CRGB (&_output)[]);
        void setDitherFill(const MYDATA &_state, CRGB (&_output)[]);
        void setDitherFill(const CRGB &_rgb, uint8_t _brightness, CRGB (&_output)[]);

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
                                        #if ENABLE_EFF_SPARKS==1
                                           ",\"Sparks\""            
                                        #endif
                                        #if ENABLE_EFF_RAINBOW==1        
                                           ",\"Rainbow\""
                                        #endif
                                        #if ENABLE_EFF_FIRE==1
                                           ",\"Fire\""
                                        #endif
                                            "]"
                                          "}";
};

#endif