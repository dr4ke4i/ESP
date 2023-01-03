#include "Arduino.h"
#include "MQTTnumbers.h"
#include "main.h"
#include "mytimer.h"

MQTTNUMBERS::MQTTNUMBERS(unsigned long _timeout)
    : tmrTimeout(_timeout)
{    
    for (int i = 0; i < MQTTNUMBERS_MAXPARAM; i++)
    {
        state_[i] = -1;
        lastState_[i] = -1;
        stateUndefined_[i] = true;
    }    
    tmrTimeout.start(false);
}

// private:
MQTTNUMBERS::MYDATA MQTTNUMBERS::get_(int _param)
{
    if ((_param >= 0) && (_param < MQTTNUMBERS_MAXPARAM))
        return state_[_param];
    else
        return -1;
}

void MQTTNUMBERS::set_(MYDATA _state, int _param)
{
    if ((_param >= 0) && (_param < MQTTNUMBERS_MAXPARAM))
        state_[_param] = _state;
}
// -------

const char* MQTTNUMBERS::getTopic(MQTT _choice, int _param)
{
    switch (_choice)
    {
    case MQTT::cfg:
        snprintf(buf, AUX_BUFFER_SIZE, strMQTTParamConfigTopicTemplate, _param);
        return buf;
        break;
    case MQTT::get:        
        snprintf(buf, AUX_BUFFER_SIZE, strMQTTParamGetTopicTemplate, _param);
        return buf;
        break;
    case MQTT::set:
        snprintf(buf, AUX_BUFFER_SIZE, strMQTTParamSetTopicTemplate, _param);    
        return buf;
        break;
    default:
        return (const char *)"";
        break;
    }
}

void MQTTNUMBERS::getPayload(char (&_msg)[], MQTT _choice, int _param)
{
    switch (_choice)
    {
    case MQTT::cfg:
        snprintf(_msg, MSG_BUFFER_SIZE, strMQTTParamConfigPayloadTemplate, 
                    _param, strMQTTParamGetTopicTemplate, strMQTTParamSetTopicTemplate, strESPstateTopic);
        snprintf(_msg, MSG_BUFFER_SIZE, _msg, _param, _param);
        break;
    case MQTT::get:
        snprintf(_msg, MSG_BUFFER_SIZE, "%u", state_[_param]);
        break;
    case MQTT::set:
        /// !!! ///
        break;
    default:
        snprintf(_msg, MSG_BUFFER_SIZE, " ");
        break;
    }    
}

//GET state
MQTTNUMBERS::MYDATA MQTTNUMBERS::state(int _param)
{
    if ((_param >= 0) && (_param < MQTTNUMBERS_MAXPARAM))
        return state_[_param];
    return -1;
}

//SET state
bool MQTTNUMBERS::state(MYDATA _newState, int _param)
{    
    if ((_param >= 0) && (_param < MQTTNUMBERS_MAXPARAM))
    {
        if (stateUndefined_[_param])
        {
            stateUndefined(false,_param);
        }
        lastState_[_param] = state_[_param];
        state_[_param] = _newState;

        // if (lastState_ != state_)
        //     set_(state_);

        return (lastState_[_param] != state_[_param]);
    }
    return false;
}

//GET true if state is undefined
bool MQTTNUMBERS::stateUndefined(int _param)
{
    if ((_param >= 0) && (_param < MQTTNUMBERS_MAXPARAM))
        {
            return stateUndefined_[_param];
        }
    return false;
}

//SET state to defined/undefined
void MQTTNUMBERS::stateUndefined(bool _newStateUndefined, int _param)
{    
    if ((_param >= 0) && (_param < MQTTNUMBERS_MAXPARAM))
    {        
        if (!_newStateUndefined)        // if new state is defined
        {   
            stateUndefined_[_param] = false;                            
            bool total = false;
            for (int i = 0; i < MQTTNUMBERS_MAXPARAM; i++)
                {   if (stateUndefined_[i] == true)      total = true;   }
            if (total == false)   tmrTimeout.stop();            
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

bool MQTTNUMBERS::stateChanged(int _param)
{
    if ((_param >= 0) && (_param < MQTTNUMBERS_MAXPARAM))
    {
        bool result = (state_[_param] != lastState_[_param]);
        lastState_[_param] = state_[_param];
        return result;
    }
    return false;
}

bool MQTTNUMBERS::undefinedTimeout()
{
    if (tmrTimeout.click())
    {
        for (int i = 0; i < MQTTNUMBERS_MAXPARAM; i++)
        {
            state(state_[i],i);
        }
        return true;
    }
    return false;
}

void MQTTNUMBERS::update()
{
    return;
}

bool MQTTNUMBERS::parsePayload(const String &_payload, int _param)
{
    if ((_param >= 0) && (_param < MQTTNUMBERS_MAXPARAM))
    {
        long conversion_result = _payload.toInt();
        if (conversion_result < -1)      return false;
        if (conversion_result > 1000)   return false;
        state(conversion_result, _param);
        return true;
    }
    return false;
}

bool MQTTNUMBERS::parsePayload(const char (&_payload)[], int _param)
{
    return parsePayload(String(_payload), _param);
}