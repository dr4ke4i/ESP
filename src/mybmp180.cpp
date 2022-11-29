#include "Arduino.h"
#include "main.h"
#include "mybmp180.h"
#include <Adafruit_BMP085.h>

MYBMP180::MYBMP180(void)
    : mybmp180_(),
      state_({0.0,0.0})
{
    mybmp180_.begin();
    // get_(state_);
    laststate_ = state_;
}

// Private:
void MYBMP180::get_(MYDATA &_state)
{
    _state.pressure = mybmp180_.readPressure();
    _state.temperature = mybmp180_.readTemperature();
}

MYBMP180::MYDATA MYBMP180::get_(void)
{
    MYDATA value;
    get_(value);
    return value;
}
// --------

const char* MYBMP180::getTopic(MQTT _choice, MQTTsensors _sensor)
{
    switch (_choice)
    {
    case MQTT::cfg:
        if (_sensor == MQTTsensors::pressure)
            return strBMP180configTopicPressure;
        else if (_sensor == MQTTsensors::temperature)
            return strBMP180configTopicTemperature;
        break;
    case MQTT::get:
        if (_sensor == MQTTsensors::pressure)
            return strBMP180getTopicPressure;
        else if (_sensor == MQTTsensors::temperature)
            return strBMP180getTopicTemperature;
        break;
    default:
        break;
    }
    return "";
}

void MYBMP180::getPayload(char (&_msg)[], MQTT _choice, MQTTsensors _sensor)
{
    switch (_choice)
    {
    case MQTT::cfg:    
        if (_sensor == MQTTsensors::pressure)
            snprintf(_msg, MSG_BUFFER_SIZE, strBMP180configPayloadPressure, strBMP180getTopicPressure, strESPstateTopic);
        else if (_sensor == MQTTsensors::temperature)
            snprintf(_msg, MSG_BUFFER_SIZE, strBMP180configPayloadTemperature, strBMP180getTopicTemperature, strESPstateTopic);
        break;
    case MQTT::get: 
        if (_sensor == MQTTsensors::pressure)
            snprintf (_msg, MSG_BUFFER_SIZE, "%6.0f", state_.pressure);
        else if (_sensor == MQTTsensors::temperature)
            snprintf (_msg, MSG_BUFFER_SIZE, "%4.1f", state_.temperature);
        break;
    default:
        snprintf(_msg, MSG_BUFFER_SIZE, " ");
        break;
    }
}

MYBMP180::MYDATA MYBMP180::state(void)
{
    return state_;
}

bool MYBMP180::stateChanged(MQTTsensors _sensor)
{
    bool result;
    switch (_sensor)
    {
    case MQTTsensors::pressure:
        result = (state_.pressure != laststate_.pressure);
        laststate_.pressure = state_.pressure;
        break;
    case MQTTsensors::temperature:
        result = (state_.temperature != laststate_.temperature);
        laststate_.temperature = state_.temperature;
        break;
    default:
        result = ((state_.pressure != laststate_.pressure) || (state_.temperature != laststate_.temperature));
        laststate_ = state_;
        break;
    }
    return result;
}

void MYBMP180::update(void)
{
    get_(state_);
}