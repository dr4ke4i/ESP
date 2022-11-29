#ifndef __INCLUDE_MYDS18B20_H
#define __INCLUDE_MYDS18B20_H

#include "Arduino.h"
#include "main.h"
#include <OneWire.h>
#include <DS18B20.h>

struct MYDS18B20 {
    public:
        MYDS18B20(void);  

        enum MQTT  {
            cfg = 1,            
            get,
            //set
        };

        const char* getTopic(MQTT _choice);
        void getPayload(char (&_msg)[], MQTT _choice);
        
        float state(void);                                   // get
        // bool state(bool _newState);                       // set

        bool stateChanged(void);
        // bool stateUndefined(void);                        // get
        // void stateUndefined(bool _newStateUndefined);     // set
        // bool undefinedTimeout(void);

        // bool parsePayload(const String &_payload);
        // bool parsePayload(const char (&_payload)[]);

        //do some things each cycle (e.g. redraw or update sensor state)
        void update(void);

    private:
        DS18B20 myds18b20_;
        // 'onewire'

        float state_, laststate_;
        // bool stateUndefined_ = true;        
        
        float get_(void);
        void get_ (float &_state);
        // void set_(MYDATA _state);

        const char* strDS18B20getTopic      = "homeassistant/sensor/" ESPnode "/Temperature_DS18B20/get";
        const char* strDS18B20configTopic   = "homeassistant/sensor/" ESPnode "/Temperature_DS18B20/config";
        const char* strDS18B20configPayload = "{ \"device_class\": \"temperature\","
                                              "  \"name\": \"" ESPnode " DS18B20\","
                                              "  \"state_topic\": \"%s\","
                                              "  \"availability_topic\": \"%s\","
                                              "  \"unit_of_measurement\": \"°C\""
                                              "}";
};

#endif