/* 14.01.2023 */

#include "Arduino.h"
#include "mybinarysensors.h"
#include "main.h"
#include "mytimer.h"

// CONSTRUCTOR
MYBINARYSENSORS::MYBINARYSENSORS(/*unsigned long _timeout*/)
    // : tmrTimeout(_timeout)
{    
    for (int i = 0; i < BINSENSORS_QUANTITY; i++)
    {
        pinMode(config_[i].pin_, config_[i].mode_);
        stateUndefined_[i] = true;
        lastState_[i] = state_[i] = digitalRead(config_[i].pin_);
    }    
    // tmrTimeout.start(false);
}

// Private:
// GETter function that uses hardware access
MYBINARYSENSORS::MYDATA MYBINARYSENSORS::get_(int _param)
{
    // if ((_param >= 0) && (_param < BINSENSORS_QUANTITY))
    //     {
            state_[_param] = digitalRead(config_[_param].pin_);
            return state_[_param];
    //     }
    // else
    //     return -1;
}
// SETter function that uses hardware access
// void MYBINARYSENSORS::set_(MYDATA _state, int _param)
// {
//     if ((_param >= 0) && (_param < BINSENSORS_QUANTITY))
//         state_[_param] = _state;
// }
// -------

const char* MYBINARYSENSORS::getTopic(MQTT _choice, int _param)
{
    if ((_param >= 0) && (_param < BINSENSORS_QUANTITY))
    {
        switch (_choice)
        {
        case MQTT::cfg:
            snprintf(buf, AUX_BUFFER_SIZE, strMQTTBinarySensorConfigTopicTemplate, _param);
            return buf;
            break;
        case MQTT::get:        
            snprintf(buf, AUX_BUFFER_SIZE, strMQTTBinarySensorGetTopicTemplate, _param);
            return buf;
            break;
        // case MQTT::set:
        //     snprintf(buf, AUX_BUFFER_SIZE, strMQTTBinarySensorSetTopicTemplate, _param);    
        //     return buf;
        //     break;
        default:
            return (const char *)"";
            break;
        }
    }
    else
        return (const char *)"";
}

void MYBINARYSENSORS::getPayload(char (&_msg)[], MQTT _choice, int _param)
{
    if ((_param >= 0) && (_param < BINSENSORS_QUANTITY))
    {
        switch (_choice)
        {
        case MQTT::cfg:
            snprintf(_msg, MSG_BUFFER_SIZE, strMQTTBinarySensorConfigPayloadTemplate, 
                        _param, strMQTTBinarySensorGetTopicTemplate, strESPstateTopic);
            snprintf(_msg, MSG_BUFFER_SIZE, _msg, _param);
            break;
        case MQTT::get: {   // '{' and '}' are needed to avoid 'error: jump to case label' bug
            bool value = (state_[_param] == LOW) ? false : true;
            if (config_[_param].inverse_)   value = !value;            
            snprintf(_msg, MSG_BUFFER_SIZE, (value ? "ON" : "OFF")); }
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
MYBINARYSENSORS::MYDATA MYBINARYSENSORS::state(int _param)
{
    if ((_param >= 0) && (_param < BINSENSORS_QUANTITY))
        return state_[_param];
    return -1;
}

//SET state
// bool MYBINARYSENSORS::state(MYDATA _newState, int _param)
// {    
//     if ((_param >= 0) && (_param < BINSENSORS_QUANTITY))
//     {
//         if (stateUndefined_[_param])
//         {
//             stateUndefined(false,_param);
//         }
//         lastState_[_param] = state_[_param];
//         state_[_param] = _newState;

//         // if (lastState_ != state_)
//         //     set_(state_);

//         return (lastState_[_param] != state_[_param]);
//     }
//     return false;
// }

//GET true if state is undefined
bool MYBINARYSENSORS::stateUndefined(int _param)
{
    if ((_param >= 0) && (_param < BINSENSORS_QUANTITY))
        {
            return stateUndefined_[_param];
        }
    return false;
}

//SET state to defined/undefined
void MYBINARYSENSORS::stateUndefined(bool _newStateUndefined, int _param)
{    
    if ((_param >= 0) && (_param < BINSENSORS_QUANTITY))
    {        
        if (!_newStateUndefined)        // if new state is defined
        {   
            stateUndefined_[_param] = false;                            
            bool total = false;
            for (int i = 0; i < BINSENSORS_QUANTITY; i++)
                {
                    if (stateUndefined_[i] == true)
                        total = true;
                }
            if (total == false)   { /*tmrTimeout.stop();*/  }
        }
        else                            // else if new state is undefined
        {
            if (!stateUndefined_[_param])       // if old state was defined
            {
                // tmrTimeout.start(false);
                stateUndefined_[_param] = true;
            }
            // else if new state && old state are undefined - do nothing
        } 
    }
}

bool MYBINARYSENSORS::stateChanged(int _param)
{
    if ((_param >= 0) && (_param < BINSENSORS_QUANTITY))
    {
        bool result = (state_[_param] != lastState_[_param]);
        lastState_[_param] = state_[_param];
        return result;
    }
    return false;
}

// bool MYBINARYSENSORS::undefinedTimeout()
// {
//     if (tmrTimeout.isRunning())
//     {
//         if (tmrTimeout.click())
//         {
//             for (int i = 0; i < BINSENSORS_QUANTITY; i++)
//             {
//                 // state(state_[i],i);
//                 get_(i);
//             }
//             return true;
//         }
//         return false;
//     } // else
//     return true;
// }

void MYBINARYSENSORS::update()
{
    for (int i = 0; i < BINSENSORS_QUANTITY; i++)     
        get_(i);
}

// bool MYBINARYSENSORS::parsePayload(const String &_payload, int _param)
// {
//     if ((_param >= 0) && (_param < BINSENSORS_QUANTITY))
//     {
//         long conversion_result = _payload.toInt();
//         if (conversion_result < -1)      return false;
//         if (conversion_result > 1000)   return false;
//         state(conversion_result, _param);
//         return true;
//     }
//     return false;
// }

// bool MYBINARYSENSORS::parsePayload(const char (&_payload)[], int _param)
// {
//     return parsePayload(String(_payload), _param);
// }