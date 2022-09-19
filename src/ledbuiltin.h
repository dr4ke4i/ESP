#ifndef __INCLUDE_LEDBUILTIN_H
#define __INCLUDE_LEDBUILTIN_H

#include "Arduino.h"
#include "main.h"
#include "mytimer.h"

struct MYLEDBUILTIN {
    public:
        MYLEDBUILTIN(unsigned long _timeout);  

        enum MQTT {
            cfg = 1,
            get,
            set
        };
        const char* getTopic(MQTT _choice);
        void getPayload(char (&_msg)[], MQTT _choice);
        
        bool state(void);                                 // get
        bool state(bool _newState);                       // set

        bool stateChanged(void);                          // get        
        bool stateUndefined(void);                        // get
        void stateUndefined(bool _newStateUndefined);     // set
        bool undefinedTimeout(void);

        bool parsePayload(const String &_payload);
        bool parsePayload(const char (&_payload)[]);

        //do some things each cycle (e.g. redraw or update sensor state)
        void update(void);
    
    private:    
        bool state_ = true;
        bool lastState_ = true;
        bool stateUndefined_ = true;
        bool get_(void);
        void set_(bool _state);
        const char* strLEDgetTopic      = "homeassistant/switch/" ESPnode "/LED/get";
        const char* strLEDsetTopic      = "homeassistant/switch/" ESPnode "/LED/set";
        const char* strLEDconfigTopic   = "homeassistant/switch/" ESPnode "/LED/config";
        const char* strLEDconfigPayload = "{ \"name\": \"" ESPnode "LED_BUILTIN\","
                                          "  \"state_topic\": \"%s\","
                                          "  \"command_topic\": \"%s\","
                                          "  \"availability_topic\": \"%s\""
                                          "}";
        MYTIMER tmrTimeout;
};

#endif