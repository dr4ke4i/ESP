#include "Arduino.h"
#include "main.h"
#include "mydht.h"
#include <Adafruit_Sensor.h>
#include <DHT.h>

MYDHT::MYDHT(void)
    : dht_(DHT_DATA_PIN, DHT22),
      state_({0.0, 0.0}),
      laststate_(state_)
{    
    dht_.begin();    
    // get_(state_);
    laststate_ = state_;    // https://docs.microsoft.com/en-us/cpp/cpp/copy-constructors-and-copy-assignment-operators-cpp
}

// Private:
void MYDHT::get_(MYDATA &_state)
{
    dht_.read();
    _state.temperature = dht_.readTemperature();
    _state.humidity = dht_.readHumidity();
}

MYDHT::MYDATA MYDHT::get_(void)
{
    MYDATA value;
    get_(value);
    return value;
}
// --------

const char* MYDHT::getTopic(MQTT _choice, MQTTsensors _sensor)
{
    switch (_choice)
    {
    case MQTT::cfg:
        if (_sensor == MQTTsensors::temperature)
            return strDHTcfgTopicTemperature;
        else if (_sensor == MQTTsensors::humidity)
            return strDHTcfgTopicHumidity;
        break;
    case MQTT::get:
        if (_sensor == MQTTsensors::temperature)
            return strDHTgetTopicTemperature;
        else if (_sensor == MQTTsensors::humidity)
            return strDHTgetTopicHumidity;
        break;
    default:
        break;
    }
    return "";
}

void MYDHT::getPayload(char (&_msg)[], MQTT _choice, MQTTsensors _sensor)
{
    switch (_choice)
    {
    case MQTT::cfg:
        if (_sensor == MQTTsensors::temperature)
            snprintf(_msg, MSG_BUFFER_SIZE, strDHTcfgPayloadTemperature, strDHTgetTopicTemperature, strESPstateTopic);
        else if (_sensor == MQTTsensors::humidity)
            snprintf(_msg, MSG_BUFFER_SIZE, strDHTcfgPayloadHumidity, strDHTgetTopicHumidity, strESPstateTopic);
        break;
    case MQTT::get: 
        if (_sensor == MQTTsensors::temperature)
            snprintf(_msg, MSG_BUFFER_SIZE, "%3.1f", state_.temperature);   
        else if (_sensor == MQTTsensors::humidity)
            snprintf(_msg, MSG_BUFFER_SIZE, "%3.1f", state_.humidity);
        break;
    default:
        snprintf(_msg, MSG_BUFFER_SIZE, " ");
        break;
    }
}

MYDHT::MYDATA MYDHT::state(void)
{
    return state_;
}

bool MYDHT::stateChanged(MQTTsensors _sensor)
{
    bool result;
    switch (_sensor)
    {
    case MQTTsensors::temperature:
        result = (state_.temperature != laststate_.temperature);
        laststate_.temperature = state_.temperature;
        break;
    case MQTTsensors::humidity:
        result = (state_.humidity != laststate_.humidity);
        laststate_.humidity = state_.humidity;
        break;
    default:
        result = ((state_.temperature != laststate_.temperature) || (state_.humidity != laststate_.humidity));
        laststate_ = state_;
        break;
    }
    return result;
}

void MYDHT::update(void)
{
    get_(state_);
}