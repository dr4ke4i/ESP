#ifndef __INCLUDE_MQTTNUMBERS_H
#define __INCLUDE_MQTTNUMBERS_H

#define MQTTNUMBERS_MAXPARAM   4

#include "Arduino.h"
#include "main.h"
#include "mytimer.h"

struct MQTTNUMBERS {
    public:
        MQTTNUMBERS(unsigned long _timeout);  
                
        typedef int16_t    MYDATA;

        enum MQTT {
            cfg = 1,
            get,
            set
        };

        const char* getTopic(MQTT _choice, int _param);
        void getPayload(char (&_msg)[], MQTT _choice, int _param);
        
        MYDATA state(int _param);                                 // get
        bool state(MYDATA _newState, int _param);                       // set

        bool stateChanged(int _param);                          // get        
        bool stateUndefined(int _param);                        // get
        void stateUndefined(bool _newStateUndefined, int _param);     // set
        bool undefinedTimeout(void);

        bool parsePayload(const String &_payload, int _param);
        bool parsePayload(const char (&_payload)[], int _param);

        //do some things each cycle (e.g. redraw or poll sensor state)
        void update(void);
    
    private:
        MYDATA state_[MQTTNUMBERS_MAXPARAM];
        MYDATA lastState_[MQTTNUMBERS_MAXPARAM];
        bool stateUndefined_[MQTTNUMBERS_MAXPARAM];
        MYDATA get_(int _param);
        void set_(MYDATA _state, int _param);
        const char* strMQTTParamGetTopicTemplate      = "homeassistant/number/" ESPnode "/RGB_Parameter_%u/get";
        const char* strMQTTParamSetTopicTemplate      = "homeassistant/number/" ESPnode "/RGB_Parameter_%u/set";
        const char* strMQTTParamConfigTopicTemplate   = "homeassistant/number/" ESPnode "/RGB_Parameter_%u/config";
        const char* strMQTTParamConfigPayloadTemplate = "{ \"name\": \"" ESPnode " Number %u\","
                                                        "  \"min\": 0,"
                                                        "  \"max\": 255,"
                                                        "  \"mode\": \"slider\","
                                                        "  \"state_topic\": \"%s\","
                                                        "  \"command_topic\": \"%s\","
                                                        "  \"availability_topic\": \"%s\""
                                                        "}";
        MYTIMER tmrTimeout;
};

#endif