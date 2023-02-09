/* 15.01.2023 */

#include "Arduino.h"
#include "MQTTswitches.h"
#include "main.h"
#include "mytimer.h"

// CONSTRUCTOR
MQTTSWITCHES::MQTTSWITCHES(unsigned long _timeout)
    : tmrTimeout(_timeout)
{    
    for (int i = 0; i < MQTTSWITCHES_QUANTITY; i++)
    {
        pinMode(config_[i].pin_, config_[i].mode_);
        stateUndefined_[i] = true;
        state_[i] = (digitalRead(config_[i].pin_) == LOW) ? false : true;
        if (config_[i].inverse_)    state_[i] = !state_[i];
        lastState_[i] = state_[i];        
    }    
    tmrTimeout.start(false);
}

// Private:
//do not check (_param) value since we trust caller function
// GETter function that uses hardware access
MQTTSWITCHES::MYDATA MQTTSWITCHES::get_(int _param)
{   
    state_[_param] = (digitalRead(config_[_param].pin_) == LOW) ? false : true;
    if (config_[_param].inverse_)    state_[_param] = !state_[_param];
    return state_[_param];
}
// SETter function that uses hardware access
void MQTTSWITCHES::set_(MYDATA _state, int _param)
{
    MYDATA value = _state;
    if (config_[_param].inverse_)    value = !value;
    digitalWrite(config_[_param].pin_, (value ? HIGH : LOW));
    state_[_param] = _state;
}
// -------

const char* MQTTSWITCHES::getTopic(MQTT _choice, int _param)
{
    if ((_param >= 0) && (_param < MQTTSWITCHES_QUANTITY))
    {
        switch (_choice)
        {
        case MQTT::cfg:
            snprintf(buf, AUX_BUFFER_SIZE, strMQTTswitchConfigTopicTemplate, _param);
            return buf;
            break;
        case MQTT::get:        
            snprintf(buf, AUX_BUFFER_SIZE, strMQTTswitchGetTopicTemplate, _param);
            return buf;
            break;
        case MQTT::set:
            snprintf(buf, AUX_BUFFER_SIZE, strMQTTswitchSetTopicTemplate, _param);    
            return buf;
            break;
        default:
            return (const char *)"";
            break;
        }
    }
    else
        return (const char *)"";
}

void MQTTSWITCHES::getPayload(char (&_msg)[], MQTT _choice, int _param)
{
    if ((_param >= 0) && (_param < MQTTSWITCHES_QUANTITY))
    {
        switch (_choice)
        {
        case MQTT::cfg:
            snprintf(_msg, MSG_BUFFER_SIZE, strMQTTswitchConfigPayloadTemplate, 
                        _param, strMQTTswitchGetTopicTemplate, strMQTTswitchSetTopicTemplate, strESPstateTopic);
            snprintf(_msg, MSG_BUFFER_SIZE, _msg, _param, _param);
            break;
        case MQTT::get: {   // '{' and '}' are needed to avoid 'error: jump to case label' bug                     
            snprintf(_msg, MSG_BUFFER_SIZE, (state_[_param] ? "ON" : "OFF")); }
            break;
        // case MQTT::set:
        //     /// !!! ///
        //     break;
        default:
            snprintf(_msg, MSG_BUFFER_SIZE, " ");
            break;
        }    
    }
    else
        sprintf(_msg," ");
}

//GET state
MQTTSWITCHES::MYDATA MQTTSWITCHES::state(int _param)
{
    if ((_param >= 0) && (_param < MQTTSWITCHES_QUANTITY))
    {
        state_[_param] = get_(_param);
        return state_[_param];
    }
    return -1;
}

//SET state
bool MQTTSWITCHES::state(MYDATA _newState, int _param)
{    
    if ((_param >= 0) && (_param < MQTTSWITCHES_QUANTITY))
    {
        if (stateUndefined_[_param])
        {
            stateUndefined(false,_param);
        }
        lastState_[_param] = state_[_param];
        state_[_param] = _newState;

        if (lastState_[_param] != _newState)
            set_(_newState,_param);

        return (lastState_[_param] != state_[_param]);
    }
    return false;
}

//GET true if state is undefined
bool MQTTSWITCHES::stateUndefined(int _param)
{
    if ((_param >= 0) && (_param < MQTTSWITCHES_QUANTITY))
        {
            return stateUndefined_[_param];
        }
    return false;
}

//SET state to defined/undefined
void MQTTSWITCHES::stateUndefined(bool _newStateUndefined, int _param)
{    
    if ((_param >= 0) && (_param < MQTTSWITCHES_QUANTITY))
    {        
        if (!_newStateUndefined)        // if new state is defined
        {   
            stateUndefined_[_param] = false;                            
            bool total = false;
            for (int i = 0; i < MQTTSWITCHES_QUANTITY; i++)
                {
                    if (stateUndefined_[i] == true)
                        total = true;
                }
            if (total == false)   { tmrTimeout.stop();  }
        }
        else                            // else if new state is undefined
        {
            if (!stateUndefined_[_param])       // if old state was defined
            {
                tmrTimeout.start(false);
                stateUndefined_[_param] = true;
            }
            // else if new state && old state are undefined - do nothing
        } 
    }
}

bool MQTTSWITCHES::stateChanged(int _param)
{
    if ((_param >= 0) && (_param < MQTTSWITCHES_QUANTITY))
    {
        bool result = (state_[_param] != lastState_[_param]);
        lastState_[_param] = state_[_param];
        return result;
    }
    return false;
}

bool MQTTSWITCHES::undefinedTimeout()
{
    if (tmrTimeout.isRunning())
    {
        if (tmrTimeout.click())
        {
            for (int i = 0; i < MQTTSWITCHES_QUANTITY; i++)
            {
                state(state_[i],i);
                get_(i);
            }
            return true;
        }
        return false;
    } // else
    return true;
}

void MQTTSWITCHES::update()
{
    // for (int i = 0; i < MQTTSWITCHES_QUANTITY; i++)     
    //     get_(i);
}

bool MQTTSWITCHES::parsePayload(const String &_payload, int _param)
{
    if ((_param >= 0) && (_param < MQTTSWITCHES_QUANTITY))
    {
        if (_payload.equals("ON"))
            {
                set_(true, _param);
                return true;
            }            
        else if (_payload.equals("OFF"))
            {
                set_(false, _param);
                return true;
            }            
        return false;
    }
    return false;
}

bool MQTTSWITCHES::parsePayload(const char (&_payload)[], int _param)
{
    return parsePayload(String(_payload), _param);
}