#ifndef __INCLUDE_MYBMP180_H
#define __INCLUDE_MYBMP180_H

#include "Arduino.h"
#include "main.h"
#include <Adafruit_BMP085.h>

struct MYBMP180 {
    public:
        MYBMP180(void);  

        enum MQTT  {
            cfg = 1,            
            get,
            //set
        };        
        
        enum MQTTsensors {
            pressure = 1,
            temperature
        };

        struct MYDATA
        {
            float   pressure;
            float   temperature;
        }; 

        const char* getTopic(MQTT _choice, MQTTsensors _sensor);
        void getPayload(char (&_msg)[], MQTT _choice, MQTTsensors _sensor);
        
        MYDATA state(void);                                  // get
        // MYDATA state(bool _newState);                       // set

        bool stateChanged(MQTTsensors _sensor);
        // bool stateUndefined(void);                        // get
        // void stateUndefined(bool _newStateUndefined);     // set
        // bool undefinedTimeout(void);

        // bool parsePayload(const String &_payload);
        // bool parsePayload(const char (&_payload)[]);

        //do some things each cycle (e.g. redraw or update sensor state)
        void update(void);

    private:
        Adafruit_BMP085 mybmp180_;

        MYDATA state_, laststate_;
        // bool stateUndefined_ = true;        
        
        MYDATA get_(void);
        void get_ (MYDATA &_state);
        // void set_(MYDATA _state);

        const char* strBMP180getTopicPressure      = "homeassistant/sensor/" ESPnode "/Pressure_BMP180/get";
        const char* strBMP180configTopicPressure   = "homeassistant/sensor/" ESPnode "/Pressure_BMP180/config";
        const char* strBMP180configPayloadPressure = "{ \"device_class\": \"pressure\","
                                                     "  \"name\": \"" ESPnode " BMP180\","
                                                     "  \"state_topic\": \"%s\","
                                                     "  \"availability_topic\": \"%s\","
                                                     "  \"unit_of_measurement\": \"Pa\""
                                                     "}";

        const char* strBMP180getTopicTemperature      = "homeassistant/sensor/" ESPnode "/Temperature_BMP180/get";
        const char* strBMP180configTopicTemperature   = "homeassistant/sensor/" ESPnode "/Temperature_BMP180/config";
        const char* strBMP180configPayloadTemperature = "{ \"device_class\": \"temperature\","
                                                        "  \"name\": \"" ESPnode " BMP180\","
                                                        "  \"state_topic\": \"%s\","
                                                        "  \"availability_topic\": \"%s\","
                                                        "  \"unit_of_measurement\": \"°C\""
                                                        "}";
};

#endif