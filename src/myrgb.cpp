#include "Arduino.h"
#include <FastLED.h>
#include <ArduinoJson.h>
#include "myrgb.h"
#include "main.h"
#include "mytimer.h"

#define DEBUGSTATE(STRING,NAME) \
    Serial.printf(STRING " { %s, bt:%u, r:%u, g:%u, b:%u, eff.id:%u, eff.nm:%s }\n", \
    NAME.enabled?"ON":"OFF", NAME.brightness, NAME.red, NAME.green, NAME.blue, NAME.effectIndex, NAME.effectName);

MYRGB::MYRGB(unsigned long _timeout)
    : tmrTimeout(_timeout),
      tmrState(_timeout),
      state_(MYDATA {false, 0,0,0, 0, 0, 0, 0, EFFINDEX::EFF_PLAIN, effectsList[EFFINDEX::EFF_PLAIN]}),
                    //enbl, r,g,b,bt,tT,spd,qnty,      effectIndex, effectName
      stateUndefined_(true)
{    
  for (int i = 0; i < NUM_LEDS; i++)
    leds_[i] = CRGB::Black;
  FastLED.addLeds<NEOPIXEL, WS2812B_DATA_PIN>(leds_, NUM_LEDS);
  lastState_ = state_;
  tmrTimeout.start(false);
}

// Private:
void MYRGB::get_(MYDATA &_state)
{
    _state = state_;
}

MYRGB::MYDATA MYRGB::get_(void)
{
    MYDATA value;
    get_(value);
    return value;
}

void MYRGB::set_(const MYDATA &_state)
{
    state_ = _state;
}
// -------

bool operator==(const MYRGB::MYDATA &_lhs, const MYRGB::MYDATA &_rhs)
{
    bool result = (_lhs.enabled == _rhs.enabled);
    result &= (_lhs.red == _rhs.red);
    result &= (_lhs.green == _rhs.green);
    result &= (_lhs.blue == _rhs.blue);
    result &= (_lhs.brightness == _rhs.brightness);
    result &= (_lhs.speed == _rhs.speed);
    result &= (_lhs.quantity == _rhs.quantity);
    result &= (_lhs.effectIndex == _rhs.effectIndex);
    result &= (_lhs.transitionTime == _rhs.transitionTime);
    return result;
}

bool operator!=(const MYRGB::MYDATA &_lhs, const MYRGB::MYDATA &_rhs)
{
    return !(_lhs==_rhs);
}

const char* MYRGB::getTopic(MQTT _choice)
{
    switch (_choice)
    {
    case MQTT::cfg:
        return strRGBcfgTopic;
        break;
    case MQTT::get:
        return strRGBgetTopic;
        break;
    case MQTT::set:
        return strRGBsetTopic;
        break;
    default:
        return (const char *)"";
        break;
    }
}

void MYRGB::getPayload(char (&_msg)[], MQTT _choice)
{
    switch (_choice)
    {
    case MQTT::cfg:
        snprintf(_msg, MSG_BUFFER_SIZE, strRGBcfgPayload, strRGBgetTopic, strRGBsetTopic, strESPstateTopic);
        break;
    case MQTT::get:
        snprintf (_msg, MSG_BUFFER_SIZE, "{ \"state\": \"%s\","
                                        "  \"brightness\": %u,"
                                        "  \"color_mode\": \"rgb\","
                                        "  \"color\":"
                                            " {\"r\": %u,"
                                            "  \"g\": %u,"
                                            "  \"b\": %u}, "
                                        "  \"effect\": \"%s\""
                                        "}",
        (state_.enabled ? "ON" : "OFF"), state_.brightness,
        state_.red, state_.green, state_.blue, state_.effectName);

            // snprintf (_msg, MSG_BUFFER_SIZE, "{ \"state\": \"%s\" }", (state_.enabled ? "ON" : "OFF"));
        break;
    case MQTT::set:
        /// !!! ///
        break;
    default:
        snprintf(_msg, MSG_BUFFER_SIZE, " ");
        break;
    }    
}

//GET RGB state
MYRGB::MYDATA MYRGB::state(void)
{
    MYDATA result = state_;
    return result;
}

//SET RGB state
bool MYRGB::state(MYRGB::MYDATA _newState)
{
    if (stateUndefined_)
    {
        stateUndefined(false);
    }
    lastState_ = state_;
    state_ = _newState;

    if (lastState_ != state_)
        update();

    return (lastState_ != state_);
}

//GET true if state is undefined
bool MYRGB::stateUndefined(void)
{
    return stateUndefined_;
}

//SET state to defined/undefined
void MYRGB::stateUndefined(bool _newStateUndefined)
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

bool MYRGB::stateChanged()
{
    bool result = (state_ != lastState_);
    lastState_ = state_;
    return result;
}

bool MYRGB::undefinedTimeout()
{
    if (tmrTimeout.click())
    {
        set_(state_);
        return true;
    }
    return false;
}

bool MYRGB::parsePayload(const String &_payload)
{
    bool result = true;
    MYDATA newState = state_;

    DeserializationError error = deserializeJson(jsonDoc_, _payload);
      if (error)
        {
            result = false;
            Serial.print("Json deserialization error: ");
            Serial.println(error.f_str());
        }
      else
        {
            Serial.print("Deserialization [ ");

            /// EFFECT AND COLOR TRANSITION
            bool transition_found = false;
            bool eff_found = false;
            if (jsonDoc_.containsKey("transition"))
            {
                transition_found = true;
                newState.transitionTime = (uint16_t)jsonDoc_["transition"];
                Serial.printf("{ transition: %u } ", newState.transitionTime);                
                newState.effectIndex = EFFINDEX::EFF_TRANSITION;
                newState.effectName = effectsList[EFFINDEX::EFF_PLAIN];
            }
            if ((!transition_found) && jsonDoc_.containsKey("effect"))
            {
                String value = String((const char *)jsonDoc_["effect"]);     
                Serial.printf("{ effect: \"%s\" } ", value.c_str());     
                
                for (int index = EFFINDEX::EFF_PLAIN; index <= EFFINDEX::EFF_SENSOR; index++)
                    if (value.equals(effectsList[index]))
                    {
                        newState.effectIndex = index;
                        newState.effectName = effectsList[index];
                        eff_found = true;
                        break;
                    }
                if (!eff_found)
                {
                    result = false;                    
                    Serial.print("/* <- unknown value*/ ");
                }
            }
            if ( (!(transition_found || eff_found)) && (state_.effectIndex == EFFINDEX::EFF_TRANSITION) )
            {
                newState.effectIndex = EFFINDEX::EFF_PLAIN;
                newState.effectName = effectsList[EFFINDEX::EFF_PLAIN];             
            }

            /// STATE ON / OFF
            if (jsonDoc_.containsKey("state"))
            {
                String value = String((const char *)jsonDoc_["state"]);
                Serial.printf("{ state: \"%s\" } ", value.c_str());
                if (value.equals("ON"))
                    newState.enabled = true;
                else if (value.equals("OFF"))
                    newState.enabled = false;
                else
                    {
                        result = false;
                        Serial.print("/* <- unknown value*/ ");
                    }            
            }

            /// COLOR
            if (jsonDoc_.containsKey("color"))
            {
                newState.red   = (uint8_t)jsonDoc_["color"]["r"];
                newState.green = (uint8_t)jsonDoc_["color"]["g"];
                newState.blue  = (uint8_t)jsonDoc_["color"]["b"];
                Serial.printf("{ color: { R:%u, G:%u, B:%u } } ", newState.red, newState.green, newState.blue);
            }

            /// BRIGHTNESS
            if (jsonDoc_.containsKey("brightness"))
            {            
                newState.brightness = (uint8_t)jsonDoc_["brightness"];
                Serial.printf("{ brightness: %u } ", newState.brightness);
            }

            /// initial states of Effects
            switch (newState.effectIndex)
            {
            case EFF_PLAIN:                    
                set_(newState);
                break;

            case EFF_TRANSITION:
                startState_ = state_;
                endState_   = newState;
                if (endState_.enabled==false)           
                {
                    endState_.brightness = 0;
                    // endState_.red = 0;
                    // endState_.green = 0;
                    // endState_.blue = 0;
                }
                else if (startState_.enabled==false)    // && endState_.enabled == true
                {
                    startState_.enabled = true;
                    startState_.brightness = 0;
                    startState_.red = 0;
                    startState_.green = 0;
                    startState_.blue = 0;
                }
                startState_.millis = millis();
                endState_.millis   = millis() + (unsigned long)newState.transitionTime * 1000;
                startState_.effectIndex = EFF_TRANSITION;
                endState_.effectIndex = EFF_PLAIN;
                startState_.effectName = effectsList[EFF_PLAIN];
                endState_.effectName = effectsList[EFF_PLAIN];
                tmrState.setPeriod((unsigned long)newState.transitionTime * 1000);
                tmrState.start(false);
                set_(startState_);
                break;
            
            default:
                newState.effectIndex = EFF_PLAIN;
                newState.effectName = effectsList[EFF_PLAIN];
                set_(newState);
                break;
            }            
            Serial.println("]");
        }
        // DEBUGSTATE("parsePayload() state_", state_)
        // DEBUGSTATE("parsePayload() lastState_", lastState_)
    return result;
}

bool MYRGB::parsePayload(const char (&_payload)[])
{
    return parsePayload(String(_payload));
}

void MYRGB::update()
{    
    // DEBUGSTATE("update() state_", state_)
    // DEBUGSTATE("update() lastState_", lastState_)
    if (!stateUndefined_)
    {
        switch (state_.effectIndex)
        {
        case EFF_PLAIN:
            if (state_ != lastState_)
            {
                CRGB value = setColor(state_);
                for (int i = 0; i < NUM_LEDS; i++)
                    leds_[i] = value;
            }
            break;

        case EFF_TRANSITION:
            if (tmrState.click())
            {
                Serial.printf("tmrState.click()\n");
                startState_ = state_ = endState_;
                state_.effectIndex = EFF_PLAIN;
                state_.effectName = effectsList[state_.effectIndex];
                CRGB value = setColor(state_);
                for (int i = 0; i < NUM_LEDS; i++)
                    leds_[i] = value;
            }
            else
            {
                static unsigned long progress = 0;
                unsigned long calc = ((millis() - startState_.millis) << 8) / tmrState.getPeriod();     // x/256-th
                if (progress != calc)
                {
                    progress = calc;
                    Serial.printf("transitioning %u/256-th\n", (byte)progress);
                    calc = (startState_.brightness << 8) + (endState_.brightness - startState_.brightness) * progress;
                    state_.brightness = calc >> 8;
                    calc = (startState_.red << 8)   + (endState_.red   - startState_.red) * progress;
                    state_.red   = calc >> 8;
                    calc = (startState_.green << 8) + (endState_.green - startState_.green) * progress;
                    state_.green = calc >> 8;
                    calc = (startState_.blue << 8)  + (endState_.blue  - startState_.blue) * progress;
                    state_.blue  = calc >> 8;
                    CRGB value = setColor(state_);
                    for (int i = 0; i < NUM_LEDS; i++)
                        leds_[i] = value;
                }                
            }
            break;
        
        default:
            break;
        }
    }
    FastLED.show();
}

CRGB MYRGB::setColor(const MYDATA &_state)
{
    CRGB        value;
    uint16_t    calc;
    if (_state.enabled)
    {
        calc = _state.brightness * _state.red;
        value.red = calc >> 8;
        calc = _state.brightness * _state.green;
        value.green = calc >> 8;
        calc = _state.brightness * _state.blue;
        value.blue = calc >> 8;
    }
    else
    {
        value = CRGB::Black;
    }
    return value;
}