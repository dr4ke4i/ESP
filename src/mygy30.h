#ifndef __INCLUDE_MYLUX_H
#define __INCLUDE_MYLUX_H

#include "Arduino.h"
#include "main.h"
#include "BH1750FVI.h"

struct MYLUX {
    public:
        MYLUX(void);  

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
        BH1750FVI myLux_;

        float state_, laststate_;
        // bool stateUndefined_ = true;        
        
        float get_(void);
        void get_ (float &_state);
        // void set_(MYDATA _state);

        const char* strLUXgetTopic      = "homeassistant/sensor/" ESPnode "/Illuminance/get";
        const char* strLUXconfigTopic   = "homeassistant/sensor/" ESPnode "/Illuminance/config";
        const char* strLUXconfigPayload = "{ \"device_class\": \"illuminance\","
                                            "  \"name\": \"" ESPnode " BH1750FVI\","
                                            "  \"state_topic\": \"%s\","
                                            "  \"availability_topic\": \"%s\","
                                            "  \"unit_of_measurement\": \"lx\""
                                            "}";
};

#endif