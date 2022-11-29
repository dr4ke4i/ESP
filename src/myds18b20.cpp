#include "Arduino.h"
#include "main.h"
#include "myds18b20.h"
#include <OneWire.h>
#include <DS18B20.h>

MYDS18B20::MYDS18B20(void)
    :
    #ifdef enableDS18B20
        myds18b20_(&onewire),
    #else   // b.c. of preprocessor bug (it includes file that is not included in the main.h or main.cpp)
        myds18b20_(nullptr),
    #endif
      state_(0.0)
{
    myds18b20_.begin();
    myds18b20_.setResolution(12);
    myds18b20_.requestTemperatures();
    // get_(state_);
    laststate_ = state_;
}

// Private:
void MYDS18B20::get_(float &_state)
{
    _state = myds18b20_.getTempC();
    myds18b20_.requestTemperatures();
}

float MYDS18B20::get_(void)
{
    float value;
    get_(value);
    return value;
}
// --------

const char* MYDS18B20::getTopic(MQTT _choice)
{
    switch (_choice)
    {
    case MQTT::cfg:
        return strDS18B20configTopic;
        break;
    case MQTT::get:
        return strDS18B20getTopic;
        break;
    default:
        break;
    }
    return "";
}

void MYDS18B20::getPayload(char (&_msg)[], MQTT _choice)
{
    switch (_choice)
    {
    case MQTT::cfg:
        snprintf(_msg, MSG_BUFFER_SIZE, strDS18B20configPayload, strDS18B20getTopic, strESPstateTopic);
        break;
    case MQTT::get: 
        snprintf (_msg, MSG_BUFFER_SIZE, "%4.1f", state_);
        break;
    default:
        snprintf(_msg, MSG_BUFFER_SIZE, " ");
        break;
    }
}

float MYDS18B20::state(void)
{
    return state_;
}

bool MYDS18B20::stateChanged(void)
{
    bool result = (state_ != laststate_);    
    laststate_ = state_;    
    return result;
}

void MYDS18B20::update(void)
{
    get_(state_);
}