/* 14.01.2023 */

#ifndef __INCLUDE_MYBINARYSENSORS_H
#define __INCLUDE_MYBINARYSENSORS_H

#define BINSENSORS_QUANTITY   2

#include "Arduino.h"
#include "main.h"
#include "mytimer.h"

struct MYBINARYSENSORS {
    public:
        typedef     int     MYDATA;
        enum        MQTT    { cfg = 1, get, /* set */  };

        MYBINARYSENSORS(/*unsigned long _timeout*/);

        // MQTT topic & payload
        const char* getTopic(MQTT _choice, int _param);
        void getPayload(char (&_msg)[], MQTT _choice, int _param);
        
        // STATE get & set
        MYDATA state(int _param);
        //bool state(MYDATA _newState, int _param);

        // CHANGE of State
        bool stateChanged(int _param);

        // UNDEFINED State value procedures
        bool stateUndefined(int _param);
        void stateUndefined(bool _newStateUndefined, int _param);
        //bool undefinedTimeout(void);

        // PARSE payload into change of State
        //bool parsePayload(const String &_payload, int _param);
        //bool parsePayload(const char (&_payload)[], int _param);

        // UPDATE some things each cycle (e.g. redraw or poll sensor state)
        void update(void);
    
    private:
        struct  MYBINSENSOR_CONFIG      {   uint8_t pin_;
                                            uint8_t mode_;
                                            bool    inverse_;       };
        
        MYBINSENSOR_CONFIG config_[BINSENSORS_QUANTITY] = {
                // {D1, INPUT, false},         // TTP223 touch button
                {D2, INPUT, false},         // RCWL-9196 motion sensor
                {D6, INPUT, false}          // GND-connected button (input pulled up)
            };                                                

        const char* strMQTTBinarySensorGetTopicTemplate      = "homeassistant/binary_sensor/" ESPnode "/binary_sensor_%u/get";
        // const char* strMQTTBinarySensorSetTopicTemplate      = "homeassistant/binary_sensor/" ESPnode "/binary_sensor_%u/set";
        const char* strMQTTBinarySensorConfigTopicTemplate   = "homeassistant/binary_sensor/" ESPnode "/binary_sensor_%u/config";
        const char* strMQTTBinarySensorConfigPayloadTemplate = "{ \"name\": \"" ESPnode " Button %u\","
                                                               "  \"state_topic\": \"%s\","
                                                               // "  \"command_topic\": \"%s\","
                                                               "  \"availability_topic\": \"%s\""
                                                               "}";
        
        MYDATA state_[BINSENSORS_QUANTITY];
        MYDATA lastState_[BINSENSORS_QUANTITY];
        bool stateUndefined_[BINSENSORS_QUANTITY];
        //MYTIMER tmrTimeout;

        // Getter & Setter functions using hardware access
        MYDATA get_(int _param);
        //void set_(MYDATA _state, int _param);
};

#endif