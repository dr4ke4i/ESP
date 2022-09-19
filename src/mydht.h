#ifndef __INCLUDE_MYDHT_H
#define __INCLUDE_MYDHT_H

#include "Arduino.h"
#include "main.h"
#include <Adafruit_Sensor.h>
#include <DHT.h>

struct MYDHT {
    public:
        MYDHT(void);  

        enum MQTT  {
            cfg = 1,            
            get,
            //set
        };

        enum MQTTsensors {
            temperature = 1,
            humidity
        };

        struct MYDATA
        {
            float   temperature;
            float   humidity;
        };  

        const char* getTopic(MQTT _choice, MQTTsensors _sensor);
        void getPayload(char (&_msg)[], MQTT _choice, MQTTsensors _sensor);
        
        MYDATA state(void);                                  // get
        // bool state(bool _newState);                       // set

        bool stateChanged(MQTTsensors _sensor);
        // bool stateUndefined(void);                        // get
        // void stateUndefined(bool _newStateUndefined);     // set
        // bool undefinedTimeout(void);

        // bool parsePayload(const String &_payload);
        // bool parsePayload(const char (&_payload)[]);

        //do some things each cycle (e.g. redraw or update sensor state)
        void update(void);

    private:
        DHT dht_;

        MYDATA state_, laststate_;
        // bool stateUndefined_ = true;        
        
        MYDATA get_(void);
        void get_ (MYDATA &_state);
        // void set_(MYDATA _state);

        const char* strDHTgetTopicTemperature   = "homeassistant/sensor/" ESPnode "/Temperature/get";
        const char* strDHTcfgTopicTemperature   = "homeassistant/sensor/" ESPnode "/Temperature/config";
        const char* strDHTcfgPayloadTemperature = "{ \"device_class\": \"temperature\","
                                                  "  \"name\": \"" ESPnode " DHT22 Temperature\","
                                                  "  \"state_topic\": \"%s\","
                                                  "  \"availability_topic\": \"%s\","
                                                  "  \"unit_of_measurement\": \"°C\""
                                                  "}";

        const char* strDHTgetTopicHumidity      = "homeassistant/sensor/" ESPnode "/Humidity/get";
        const char* strDHTcfgTopicHumidity      = "homeassistant/sensor/" ESPnode "/Humidity/config";
        const char* strDHTcfgPayloadHumidity    = "{ \"device_class\": \"humidity\","
                                                  "  \"name\": \"" ESPnode " DHT22 Humidity\","
                                                  "  \"state_topic\": \"%s\","
                                                  "  \"availability_topic\": \"%s\","
                                                  "  \"unit_of_measurement\": \"%%\""
                                                  "}";
};

#endif