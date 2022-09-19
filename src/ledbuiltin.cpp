#include "Arduino.h"
#include "ledbuiltin.h"
#include "main.h"
#include "mytimer.h"

MYLEDBUILTIN::MYLEDBUILTIN(unsigned long _timeout)
    : tmrTimeout(_timeout)
{    
  pinMode(LED_BUILTIN, OUTPUT);  
  state_ = get_();
  lastState_ = state_;
  stateUndefined_ = true;
  tmrTimeout.start(false);
}

// private:
bool MYLEDBUILTIN::get_(void)
{
    return (digitalRead(LED_BUILTIN) == HIGH) ? false : true;
}

void MYLEDBUILTIN::set_(bool _state)
{
    digitalWrite(LED_BUILTIN, _state ? LOW : HIGH);
}
// -------

const char* MYLEDBUILTIN::getTopic(MQTT _choice)
{
    switch (_choice)
    {
    case MQTT::cfg:
        return strLEDconfigTopic;
        break;
    case MQTT::get:
        return strLEDgetTopic;
        break;
    case MQTT::set:
        return strLEDsetTopic;
        break;
    default:
        return (const char *)"";
        break;
    }
}

void MYLEDBUILTIN::getPayload(char (&_msg)[], MQTT _choice)
{
    switch (_choice)
    {
    case MQTT::cfg:
        snprintf(_msg, MSG_BUFFER_SIZE,strLEDconfigPayload, strLEDgetTopic, strLEDsetTopic, strESPstateTopic);
        break;
    case MQTT::get:
        snprintf(_msg, MSG_BUFFER_SIZE, state_ ? "ON" : "OFF");
        break;
    case MQTT::set:
        /// !!! ///
        break;
    default:
        snprintf(_msg, MSG_BUFFER_SIZE, " ");
        break;
    }    
}

//GET LED state (true for ON)
bool MYLEDBUILTIN::state(void)
{
    return state_;
}

//SET LED state
bool MYLEDBUILTIN::state(bool _newState)
{
    if (stateUndefined_)
    {
        stateUndefined(false);
    }
    lastState_ = state_;
    state_ = _newState;

    if (lastState_ != state_)
        set_(state_);

    return (lastState_ != state_);
}

//GET true if state is undefined
bool MYLEDBUILTIN::stateUndefined(void)
{
    return stateUndefined_;
}

//SET state to defined/undefined
void MYLEDBUILTIN::stateUndefined(bool _newStateUndefined)
{
    if (!_newStateUndefined)        // if new state is defined
    {                               
        tmrTimeout.stop();
        stateUndefined_ = false;
    }
    else                            // else if new state is undefined
    {
        if (!stateUndefined_)       // if old state was defined
        {
            tmrTimeout.start(false);
            stateUndefined_ = true;
        }
        // else if new state && old state are undefined - do nothing
    } 
}

bool MYLEDBUILTIN::stateChanged()
{
    bool result = (state_ != lastState_);
    lastState_ = state_;
    return result;
}

bool MYLEDBUILTIN::undefinedTimeout()
{
    if (tmrTimeout.click())
    {
        state(state_);
        return true;
    }
    return false;
}

void MYLEDBUILTIN::update()
{
    return;
}

bool MYLEDBUILTIN::parsePayload(const String &_payload)
{
    if (_payload.equals("ON"))
    {
        state(true);
        return true;
    }
    else if (_payload.equals("OFF"))
    {
        state(false);
        return true;
    }
    else
        return false;
}

bool MYLEDBUILTIN::parsePayload(const char (&_payload)[])
{
    return parsePayload(String(_payload));
}