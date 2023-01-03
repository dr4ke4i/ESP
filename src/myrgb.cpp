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
      tmrTransition(_timeout),
      tmrFade(_timeout),
      state_(MYDATA {false, 0,0,0, 0, 0, 0, 0,   0,  EFFINDEX::EFF_PLAIN, effectsList[EFFINDEX::EFF_PLAIN]}),
                    //enbl, r,g,b,bt,tT,spd,qnty,hue,effectIndex,         effectName
      stateUndefined_(true)
{    
  for (int i = 0; i < NUM_LEDS; i++)
    leds_[i] = CRGB::Black;
  FastLED.addLeds<NEOPIXEL, WS2812B_DATA_PIN>(leds_, NUM_LEDS);
  startState_ = endState_ = effectState_ = plainState_ = lastState_ = state_;
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
    result &= (_lhs.effectIndex == _rhs.effectIndex);
    // result &= (_lhs.speed == _rhs.speed);
    // result &= (_lhs.quantity == _rhs.quantity);    
    // result &= (_lhs.transitionTime == _rhs.transitionTime);
    // result &= (_lhs.hue == _rhs.hue);
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
        effectState_ = plainState_ = state_;
        return true;
    }
    return false;
}

bool MYRGB::parsePayload(const String &_payload)
{
    bool result = true;
    MYDATA newState = state_;
    MYDATA &prevState = state_;

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
            if ( (!(transition_found || eff_found)) && (prevState.effectIndex == EFFINDEX::EFF_TRANSITION) )
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
                if ((prevState.effectIndex==EFF_PLAIN) || (prevState.effectIndex==EFF_TRANSITION))
                {
                    set_(newState);
                    plainState_ = state_;
                }
                else    // if previous state had effect
                {
                    effectState_ = prevState;
                    newState.red = plainState_.red;
                    newState.green = plainState_.green;
                    newState.blue = plainState_.blue;
                    newState.brightness = plainState_.brightness;
                    set_(newState);
                }                   
                break;

            case EFF_TRANSITION:
                if ((prevState.effectIndex==EFF_PLAIN) || (prevState.effectIndex==EFF_TRANSITION))
                {
                    startState_ = prevState;
                    endState_   = newState;
                    plainState_ = endState_;
                }
                else
                {
                    effectState_ = prevState;
                    startState_ = prevState;
                    startState_.red = plainState_.red;
                    startState_.green = plainState_.green;
                    startState_.blue = plainState_.blue;
                    startState_.brightness = plainState_.brightness;
                    endState_ = newState;
                    plainState_ = newState;
                }
                
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
                    startState_.red = newState.red;
                    startState_.green = newState.green;
                    startState_.blue = newState.blue;
                }
                startState_.effectIndex = EFF_TRANSITION;
                endState_.effectIndex = EFF_PLAIN;
                startState_.effectName = effectsList[EFF_PLAIN];
                endState_.effectName = effectsList[EFF_PLAIN];
                tmrTransition.setPeriod((unsigned long)newState.transitionTime * 1000);
                startState_.millis = millis();
                endState_.millis   = millis() + (unsigned long)newState.transitionTime * 1000;
                tmrTransition.start(false);
                set_(startState_);
                break;
                
            case EFF_SPARKS:
                if ((prevState.effectIndex==EFF_PLAIN) || (prevState.effectIndex==EFF_TRANSITION))
                {
                    plainState_ = prevState;
                    newState.red = effectState_.red;
                    newState.green = effectState_.green;
                    newState.blue = effectState_.blue;
                    newState.brightness = effectState_.brightness;
                }
                else
                {                    
                    effectState_ = state_;
                }                
                set_(newState);
                led_rgb = CRGB(state_.red, state_.green, state_.blue);
                led_hsv = rgb2hsv_approximate(led_rgb);
                tmrTransition.setPeriod(state_.speed);
                tmrFade.setPeriod(state_.transitionTime);
                tmrTransition.start(true);
                tmrFade.start(true);
                break;
            
            default:
                newState.effectIndex = EFF_PLAIN;
                newState.effectName = effectsList[EFF_PLAIN];
                set_(newState);
                plainState_ = state_;
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
                // CRGB value = setColor(state_);
                // for (int i = 0; i < NUM_LEDS; i++)
                //     leds_[i] = value;
                setDitherFill(state_);
            }
            break;

        case EFF_TRANSITION:
            if (tmrTransition.click())
            {
                Serial.printf("tmrTransition.click()\n");
                startState_ = state_ = endState_;
                state_.effectIndex = EFF_PLAIN;
                state_.effectName = effectsList[state_.effectIndex];
                // CRGB value = setColor(state_);
                // for (int i = 0; i < NUM_LEDS; i++)
                //     leds_[i] = value;
                setDitherFill(state_);
            }
            else
            {
                static uint16_t progress = 0;
                uint16_t calc = tmrTransition.progress16();
                if (progress != calc)
                {
                    progress = calc;
                    // Serial.printf("progress16() = %#X\n", calc);
                    setDitherFill(  ((startState_.red << 16) + (endState_.red - startState_.red) * progress) >> 8,
                                    ((startState_.green << 16) + (endState_.green - startState_.green) * progress) >> 8,
                                    ((startState_.blue << 16) + (endState_.blue - startState_.blue) * progress) >> 8,
                        ((startState_.brightness << 16) + (endState_.brightness - startState_.brightness) * progress) >> 8);
                }
            }            
            break;
        
        case EFF_SPARKS:            
            tmrTransition.setPeriod(state_.speed);
            tmrFade.setPeriod(state_.transitionTime);
            // FADE everything
            if (tmrFade.click())
            {                   
                uint8_t     value;
                Serial.printf("< leds_[0]: {%u %u %u}, \t ", leds_[0].r, leds_[0].g, leds_[0].b);
                for (int i = 0; i < NUM_LEDS; i++)
                {
                    for (int j = 0; j < 3; j++)
                    {
                        value = leds_[i].raw[j];
                        value = (value > 0) ? value - (value >> 5) - 1 : 0;
                        leds_[i].raw[j] = value;
                    }
                    // leds_[i]--;
                }
                Serial.printf("leds_[0]: {%u %u %u} >\n", leds_[0].r, leds_[0].g, leds_[0].b);
            }
            // SPAWN new sparks
            if (tmrTransition.click())
            {
                int cycles = state_.quantity / GUARANTEE_THRESHOLD;
                if (state_.quantity % GUARANTEE_THRESHOLD != 0) cycles++;
                for (int i = cycles; i >=0; i--)
                    if ((i!=0) || ((state_.quantity % GUARANTEE_THRESHOLD) >= random(GUARANTEE_THRESHOLD)))
                        {                                        
                            int     new_pos = random(NUM_LEDS);
                            CHSV    new_hsv = led_hsv;
                            CRGB    new_rgb;
                            new_hsv.hue += (uint8_t)random(state_.hue) - state_.hue / 2;
                            new_hsv.saturation = 255;
                            new_hsv.value = random(state_.brightness);                    
                            hsv2rgb_rainbow(new_hsv, new_rgb);
                            leds_[new_pos] += new_rgb;
                            for (int i = 1; i <= 2; i++)
                            {
                                new_rgb >>= 3;
                                if (new_pos + i <= NUM_LEDS)    leds_[new_pos+i] += new_rgb;
                                if (new_pos - i >= 0)           leds_[new_pos-i] += new_rgb;
                            }
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

void MYRGB::setDitherFill(const MYDATA &_state)
{
    uint32_t    quant_error_red = 0, quant_error_green = 0, quant_error_blue = 0;
    CRGB        value;
    uint32_t    calc;
    if (_state.enabled)
    {
        for (int i = 0; i < NUM_LEDS; i++)
        {
            calc = _state.brightness * _state.red + quant_error_red;
            value.red = ((calc >> 8) > 255) ? 255 : (calc >> 8);
            quant_error_red = calc - (value.red << 8);            
            calc = _state.brightness * _state.green + quant_error_green;
            value.green = ((calc >> 8) > 255) ? 255 : (calc >> 8);
            quant_error_green = calc - (value.green << 8);
            calc = _state.brightness * _state.blue + quant_error_blue;
            value.blue = ((calc >> 8) > 255) ? 255 : (calc >> 8);
            quant_error_blue = calc - (value.blue << 8);
            leds_[i] = value;
        }
    }
    else
        FastLED.clearData();
}

void MYRGB::setDitherFill(uint32_t _red, uint32_t _green, uint32_t _blue, uint32_t _brightness)
{
    uint64_t    quant_error_red = 0, quant_error_green = 0, quant_error_blue = 0;
    uint64_t    calc;
    CRGB        value;
    for (int i = 0; i < NUM_LEDS; i++)
    {
        calc = _brightness * _red + quant_error_red;
        value.red = ((calc >> 24) > 255) ? 255 : calc >> 24;
        quant_error_red = calc - (value.red << 24);
        calc = _brightness * _green + quant_error_green;
        value.green = ((calc >> 24) > 255) ? 255 : calc >> 24;
        quant_error_green = calc - (value.green << 24);
        calc = _brightness * _blue + quant_error_blue;
        value.blue = ((calc >> 24) > 255) ? 255 : calc >> 24;
        quant_error_blue = calc - (value.blue << 24);
        leds_[i] = value;
    }
}