#include "Arduino.h"
#include "mygy30.h"
#include "main.h"
#include "BH1750FVI.h"

MYLUX::MYLUX(void)
    : myLux_(0x23,SDA_PIN,SCL_PIN),
      state_(0.0)
{
    Wire.begin();
    myLux_.powerOn();
    myLux_.setContLowRes();    
    // get_(state_);
    laststate_ = state_;
}

// Private:
void MYLUX::get_(float &_state)
{
    _state = myLux_.getRaw();
}

float MYLUX::get_(void)
{
    float value;
    get_(value);
    return value;
}
// --------

const char* MYLUX::getTopic(MQTT _choice)
{
    switch (_choice)
    {
    case MQTT::cfg:
        return strLUXconfigTopic;
        break;
    case MQTT::get:
        return strLUXgetTopic;
        break;
    default:
        break;
    }
    return "";
}

void MYLUX::getPayload(char (&_msg)[], MQTT _choice)
{
    switch (_choice)
    {
    case MQTT::cfg:
        snprintf(_msg, MSG_BUFFER_SIZE, strLUXconfigPayload, strLUXgetTopic, strESPstateTopic);
        break;
    case MQTT::get: 
        snprintf (_msg, MSG_BUFFER_SIZE, "%4.1f", state_);
        break;
    default:
        snprintf(_msg, MSG_BUFFER_SIZE, " ");
        break;
    }
}

float MYLUX::state(void)
{
    return state_;
}

bool MYLUX::stateChanged(void)
{
    bool result = (state_ != laststate_);    
    laststate_ = state_;    
    return result;
}

void MYLUX::update(void)
{
    get_(state_);
}