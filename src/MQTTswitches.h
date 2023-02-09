/* 15.01.2023 */

#ifndef __INCLUDE_MQTTSWITCHES_H
#define __INCLUDE_MQTTSWITCHES_H

#define MQTTSWITCHES_QUANTITY   3

#include "Arduino.h"
#include "main.h"
#include "mytimer.h"

struct MQTTSWITCHES {
    public:
        typedef     bool    MYDATA;
        enum        MQTT    { cfg = 1, get, set };

        MQTTSWITCHES(unsigned long _timeout);

        // MQTT topic & payload
        const char* getTopic(MQTT _choice, int _param);
        void getPayload(char (&_msg)[], MQTT _choice, int _param);
        
        // STATE get & set
        MYDATA state(int _param);
        bool state(MYDATA _newState, int _param);

        // CHANGE of State
        bool stateChanged(int _param);

        // UNDEFINED State value procedures
        bool stateUndefined(int _param);
        void stateUndefined(bool _newStateUndefined, int _param);
        bool undefinedTimeout(void);

        // PARSE payload into change of State
        bool parsePayload(const String &_payload, int _param);
        bool parsePayload(const char (&_payload)[], int _param);

        // UPDATE some things each cycle (e.g. redraw or poll sensor state)
        void update(void);
    
    private:
        struct  MQTTSWITCH_CONFIG       {   uint8_t pin_;
                                            uint8_t mode_;
                                            bool    inverse_;       };
        
        MQTTSWITCH_CONFIG config_[MQTTSWITCHES_QUANTITY] = {                
                {D4, OUTPUT, false},        // transistor-driven relay switch (fan)
                {D7, OUTPUT, true},         // solid-state relay (mirror lamp)
                {D1, OUTPUT, true},         // solid-state relay (bathtub)
            };                                                

        const char* strMQTTswitchGetTopicTemplate      = "homeassistant/switch/" ESPnode "/switch_%u/get";
        const char* strMQTTswitchSetTopicTemplate      = "homeassistant/switch/" ESPnode "/switch_%u/set";
        const char* strMQTTswitchConfigTopicTemplate   = "homeassistant/switch/" ESPnode "/switch_%u/config";
        const char* strMQTTswitchConfigPayloadTemplate = "{ \"name\": \"" ESPnode " Switch %u\","
                                                         "  \"state_topic\": \"%s\","
                                                         "  \"command_topic\": \"%s\","
                                                         "  \"availability_topic\": \"%s\""
                                                         "}";
        
        MYDATA state_[MQTTSWITCHES_QUANTITY];
        MYDATA lastState_[MQTTSWITCHES_QUANTITY];
        bool stateUndefined_[MQTTSWITCHES_QUANTITY];
        MYTIMER tmrTimeout;

        // Getter & Setter functions using hardware access
        MYDATA get_(int _param);
        void set_(MYDATA _state, int _param);
};

#endif