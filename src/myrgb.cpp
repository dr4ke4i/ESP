#include "Arduino.h"
#include <FastLED.h>
#include <ArduinoJson.h>
#include "myrgb.h"
#include "main.h"
#include "mytimer.h"

#define DEBUGSTATE(STRING,NAME) \
    Serial.printf(STRING " { %s, bt:%u, r:%u, g:%u, b:%u, eff.id:%u, eff.nm:%s, t:%s, tT:%lu }\n", \
    NAME.enabled?"ON":"OFF", NAME.brightness, NAME.red, NAME.green, NAME.blue, NAME.effectIndex, NAME.effectName, \
    NAME.transition?"yes":"no", NAME.transitionTime);

MYRGB::MYRGB(unsigned long _timeout)
    : led_rgb(CRGB::Black),
      led_hsv(0,0,0),
      tmrTimeout(_timeout),
      tmrTransition(_timeout),
      tmrFade(_timeout),
      fromstate_(MYDATA {false,255,255,255, 1,false,2000,EFFINDEX::EFF_PLAIN,effectsList[EFFINDEX::EFF_PLAIN], {0,0,0,0} }),
                        //enbl,  r,  g,  b,bt,trans,  tT,        effectIndex,                      effectName,p[0,1,2,3]
      currentstate_(fromstate_),
      tostate_(fromstate_),
      stateUndefined_(true),
      stateChanged_(false)
{    
  for (int i = 0; i < NUM_LEDS; i++)
  {
    leds_[i]    = CRGB::Black;
    buf_[i]     = CRGB::Black;
    bright_[i]  = 0;
  }
  for (int i = 0; i < MAX_EFFECTS; i++)  
    {
        profile_[i] = currentstate_;  
        profile_[i].effectIndex = i;
        profile_[i].effectName = effectsList[i];
    }
  FastLED.addLeds<NEOPIXEL, WS2812B_DATA_PIN>(leds_, NUM_LEDS);
  tmrTimeout.start(false); tmrFade.stop(); tmrTransition.stop();
}

// Private:
void MYRGB::get_(MYDATA &_state)
{
    _state = tostate_;
}

MYRGB::MYDATA MYRGB::get_(void)
{
    MYDATA value;
    get_(value);
    return value;
}

void MYRGB::set_(const MYDATA &_state)
{
    tostate_ = _state;
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
    result &= (_lhs.transition == _rhs.transition);
    result &= (_lhs.transitionTime == _rhs.transitionTime);
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
        (tostate_.enabled ? "ON" : "OFF"), tostate_.brightness,
        tostate_.red, tostate_.green, tostate_.blue, tostate_.effectName);

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
    MYDATA result = tostate_;
    return result;
}

//SET RGB state
bool MYRGB::state(MYRGB::MYDATA _newState)
{
    if (stateUndefined_)
    {
        stateUndefined(false);
    }
    fromstate_ = currentstate_;
    tostate_ = _newState;

    if (fromstate_ != tostate_)
        update();

    return (fromstate_ != tostate_);
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
    // bool result = (fromstate_ != tostate_);
    // lastState_ = state_;
    bool result = stateChanged_;
    stateChanged_ = false;
    return result;
}

bool MYRGB::undefinedTimeout()
{
    if (tmrTimeout.click())
    {
        // set_(state_);
        // effectState_ = plainState_ = state_;
        return true;
    }
    return false;
}

bool MYRGB::parsePayload(const String &_payload)
{
    bool result = true;
    MYDATA newstate = tostate_;
    MYDATA &prevstate = currentstate_;

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

            /// EFFECT & TRANSITION
            bool eff_found = false;
            if (jsonDoc_.containsKey("effect"))
            {
                String value = String((const char *)jsonDoc_["effect"]);     
                Serial.printf("{ effect: \"%s\" } ", value.c_str());     
                
                for (int index = EFFINDEX::EFF_PLAIN; index <= MAX_EFFECTS; index++)
                    if (value.equals(effectsList[index]))
                    {
                        newstate = profile_[index];
                        newstate.effectIndex = index;
                        newstate.effectName = effectsList[index];
                        eff_found = true;
                        break;
                    }
                if (!eff_found)
                {
                    result = false;                    
                    Serial.print("/* <- unknown value*/ ");
                }
            }
            newstate.transition = false;
            if (jsonDoc_.containsKey("transition"))
            {
                newstate.transition = true;
                newstate.transitionTime = ((unsigned long)jsonDoc_["transition"]) * 1000;
                Serial.printf("{ transition: %lu } ", newstate.transitionTime);    
            }

            /// STATE ON / OFF
            if (jsonDoc_.containsKey("state"))
            {
                String value = String((const char *)jsonDoc_["state"]);
                Serial.printf("{ state: \"%s\" } ", value.c_str());
                if (value.equals("ON"))
                    newstate.enabled = true;
                else if (value.equals("OFF"))
                    newstate.enabled = false;
                else
                    {
                        result = false;
                        Serial.print("/* <- unknown value*/ ");
                    }            
            }

            /// COLOR & BRIGHTNESS
            if (jsonDoc_.containsKey("color"))
            {
                newstate.red   = (uint8_t)jsonDoc_["color"]["r"];
                newstate.green = (uint8_t)jsonDoc_["color"]["g"];
                newstate.blue  = (uint8_t)jsonDoc_["color"]["b"];
                Serial.printf("{ color: { R:%u, G:%u, B:%u } } ", newstate.red, newstate.green, newstate.blue);
            }
            
            if (jsonDoc_.containsKey("brightness"))
            {            
                newstate.brightness = (uint8_t)jsonDoc_["brightness"];
                Serial.printf("{ brightness: %u } ", newstate.brightness);
            }

            /// trigger update MQTT of get message
            if (prevstate != newstate)      stateChanged_ = true;

            /// initial states of Effects
            if (prevstate.effectIndex == newstate.effectIndex)
            {
                fromstate_ = currentstate_;
                tostate_ = newstate;
                profile_[newstate.effectIndex] = newstate;                
                if (newstate.transition)
                    {   tostate_.transitionTime = newstate.transitionTime;
                        tostate_.transition = true;     }
                else
                    {   tostate_.transitionTime = DEFAULT_TRANSITION_TIME;
                        tostate_.transition = true;     }
                switch(newstate.effectIndex)
                {
                    case EFF_PLAIN:
                        tmrTransition.setPeriod(tostate_.transitionTime);
                        tmrTransition.start(false);
                        break;

                    #if ENABLE_EFF_SPARKS==1
                    case EFF_SPARKS:
                        led_rgb = CRGB(tostate_.r, tostate_.g, tostate_.b);
                        led_hsv = rgb2hsv_approximate(led_rgb);
                        tmrFade.setPeriod(tostate_.param[3]);
                        tmrTransition.setPeriod(tostate_.param[1]);
                        tmrFade.start(true);
                        tmrTransition.start(true);
                        break;
                    #endif

                    #if ENABLE_EFF_RAINBOW==1
                    case EFF_RAINBOW:
                        break;
                    #endif

                    #if ENABLE_EFF_FIRE==1
                    case EFF_FIRE:
                        break;
                    #endif

                    default:
                        tostate_ = profile_[EFF_PLAIN];
                        currentstate_ = fromstate_ = tostate_;
                        tostate_.transition = false;
                        stateChanged_ = true;
                        break;
                }
            }
            else    // prevstate.effectIndex != newstate.effectIndex
            {
                fromstate_ = currentstate_;
                tostate_ = newstate;
                profile_[newstate.effectIndex] = newstate;
                // tostate_ = profile_[newstate.effectIndex];
                if (newstate.transition)
                    {   tostate_.transitionTime = newstate.transitionTime;
                        tostate_.transition = true;     }
                else
                    {   tostate_.transitionTime = DEFAULT_TRANSITION_TIME;
                        tostate_.transition = true;     }
                switch (newstate.effectIndex)
                {
                    case EFF_PLAIN:
                        for (int i = 0; i < NUM_LEDS; i++)
                            buf_[i] = leds_[i];
                        tmrFade.stop();
                        tmrTransition.setPeriod(tostate_.transitionTime);
                        tmrTransition.start(false);                      
                        break;  

                    #if ENABLE_EFF_SPARKS==1                    
                    case EFF_SPARKS:
                    {   CRGB    rgb = CRGB(currentstate_.r, currentstate_.g, currentstate_.b);
                        uint8_t brightness;
                        if (currentstate_.enabled)      brightness = currentstate_.br;
                            else    brightness = 0;
                        for (int i = 0; i < NUM_LEDS; i++)
                        {
                            buf_[i] = rgb;
                            bright_[i] = brightness;
                        }
                        led_rgb = CRGB(tostate_.r, tostate_.g, tostate_.b);
                        led_hsv = rgb2hsv_approximate(led_rgb);
                        tmrFade.setPeriod(tostate_.param[3]);
                        tmrTransition.setPeriod(tostate_.param[1]);
                        tmrFade.start(true);
                        tmrTransition.start(true);
                    }
                        break;
                    #endif

                    #if ENABLE_EFF_RAINBOW==1
                    case EFF_RAINBOW:
                        break;
                    #endif

                    #if ENABLE_EFF_FIRE==1
                    case EFF_FIRE:
                        break;
                    #endif
                
                    default:
                        tostate_ = profile_[EFF_PLAIN];
                        currentstate_ = fromstate_ = tostate_;
                        stateChanged_ = true;
                        break;
                }            
            }
            Serial.println("]");
        }
        // DEBUGSTATE("parsePayload() tostate_", tostate_)
        // DEBUGSTATE("parsePayload() currentstate_", currentstate_)
        // DEBUGSTATE("parsePayload() fromstate_", fromstate_)
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
        switch (tostate_.effectIndex)
        {
        case EFF_PLAIN:
            if (fromstate_.effectIndex == EFF_PLAIN)
            {
                if (tmrTransition.click())
                {
                    tostate_.transition = false;
                    tostate_.transitionTime = DEFAULT_TRANSITION_TIME;
                    fromstate_ = tostate_;
                    currentstate_ = tostate_;
                    setDitherFill(tostate_, leds_);                    
                    tmrTransition.stop();
                    stateChanged_ = true;
                    // DEBUGSTATE("update(): tostate_", tostate_)
                    // DEBUGSTATE("update(): currentstate_", currentstate_)
                    // DEBUGSTATE("update(): fromstate_", fromstate_)
                    // Serial.printf("leds_[%u] {%u,%u,%u} \n", NUM_LEDS-1, leds_[NUM_LEDS-1].r, leds_[NUM_LEDS-1].g, leds_[NUM_LEDS-1].b);
                }             
                else if (tmrTransition.isRunning())
                {
                    static uint16_t progress = 0;
                    uint16_t        calc     = tmrTransition.progress16();
                    if (progress != calc)
                    {
                        progress = calc;
                        int64_t p_r16 = ((((int64_t)fromstate_.r) << 16) + ((int64_t)tostate_.r - (int64_t)fromstate_.r) * ((int64_t)progress)) >> 8;
                        int64_t p_g16 = ((((int64_t)fromstate_.g) << 16) + ((int64_t)tostate_.g - (int64_t)fromstate_.g) * ((int64_t)progress)) >> 8;
                        int64_t p_b16 = ((((int64_t)fromstate_.b) << 16) + ((int64_t)tostate_.b - (int64_t)fromstate_.b) * ((int64_t)progress)) >> 8;             
                        int64_t start_br, end_br;
                        if (tostate_.enabled)       end_br = (int64_t)tostate_.brightness;
                            else                    end_br = 0;
                        if (fromstate_.enabled)     start_br = (int64_t)fromstate_.brightness;
                            else                    start_br = 0;
                        int64_t p_br16 = (((start_br) << 16) + (end_br - start_br) * ((int64_t)progress)) >> 8;   
                        setDitherFill(p_r16, p_g16, p_b16, p_br16, leds_);
                        currentstate_.r = p_r16 >> 8;
                        currentstate_.g = p_g16 >> 8;
                        currentstate_.b = p_b16 >> 8;
                        currentstate_.br = p_br16 >> 8;
                        currentstate_.enabled = fromstate_.enabled || tostate_.enabled;
                        // Serial.printf("%u: \tr16 %lli \tg16 %lli \tb16 %lli \tbr16 %lli \tleds_[%u] {%u,%u,%u} \n", 
                        //                 progress, p_r16, p_g16, p_b16, p_br16,
                        //                 NUM_LEDS-1, leds_[NUM_LEDS-1].r, leds_[NUM_LEDS-1].g, leds_[NUM_LEDS-1].b);
                    }
                }   
                else
                {
                    // NOP?
                }
            }
            else    // prev effect wasn't EFF_PLAIN
            {
                if (tmrTransition.click())
                {
                    tostate_.transition = false;
                    tostate_.transitionTime = DEFAULT_TRANSITION_TIME;
                    fromstate_ = tostate_;
                    currentstate_ = tostate_;
                    setDitherFill(tostate_, leds_);                    
                    stateChanged_ = true;                  
                    tmrTransition.stop();
                }            
                else if (tmrTransition.isRunning())
                {
                    static int32_t progress = 0;                    
                    int32_t        calc     = tmrTransition.progress8();
                    if (progress != calc)
                    {                        
                        progress = calc;
                        CRGB      curr_value;
                        int32_t  p_r32 = 0, p_g32 = 0, p_b32 = 0;
                        int32_t  start_red, start_green, start_blue;
                        int32_t  end_red = ((int32_t)tostate_.r * (int32_t)tostate_.br) >> 8;
                        int32_t  end_green = ((int32_t)tostate_.g * (int32_t)tostate_.br) >> 8;
                        int32_t  end_blue = ((int32_t)tostate_.b * (int32_t)tostate_.br) >> 8;
                        for (int i = 0; i < NUM_LEDS; i++)
                        {
                            start_red = buf_[i].r;  start_green = buf_[i].g;    start_blue = buf_[i].b;
                            calc = ((start_red   << 8) + (end_red   - start_red)   * progress) >> 8;    curr_value.r = calc;
                            calc = ((start_green << 8) + (end_green - start_green) * progress) >> 8;    curr_value.g = calc;
                            calc = ((start_blue  << 8) + (end_blue  - start_blue)  * progress) >> 8;    curr_value.b = calc;
                            leds_[i] = curr_value;
                            p_r32 += curr_value.r;      p_g32 += curr_value.g;      p_b32 += curr_value.b;
                        }                          
                        currentstate_.r = p_r32 / NUM_LEDS; currentstate_.g = p_g32 / NUM_LEDS; currentstate_.b = p_b32 / NUM_LEDS;
                        currentstate_.enabled = fromstate_.enabled || tostate_.enabled;
                    }
                }    
                else
                {
                    // NOP?
                }
            }
            break;
        
        #if ENABLE_EFF_SPARKS==1
        case EFF_SPARKS:
            if (fromstate_.effectIndex == EFF_SPARKS)
            {
                tmrTransition.setPeriod(tostate_.param[1]);
                tmrFade.setPeriod(tostate_.param[3]);
                if (tmrFade.click())
                {
                    uint8_t brightness;
                    uint64_t p_br = 0;
                    for (int i = 0; i < NUM_LEDS; i++)
                    {
                        brightness = bright_[i];
                        if (brightness > 0)
                           bright_[i] = --brightness;    
                        leds_[i] = setColor(buf_[i], brightness);
                        p_br += brightness;
                    }
                    currentstate_.br = p_br / NUM_LEDS;
                }
                if (tmrTransition.click())
                {                    
                    if (tostate_.enabled) 
                    {
                        int spawn_cycles = (tostate_.param[0] / GUARANTEE_THRESHOLD);
                        for (int i = spawn_cycles; i >= 0; i--)                    
                            if ((i != 0) || ((tostate_.param[0] % GUARANTEE_THRESHOLD) >= random (GUARANTEE_THRESHOLD)))
                            {
                                int new_pos = random(NUM_LEDS);
                                for (int i = 0; i < 100; i++)
                                    if (bright_[new_pos]==0)    break;
                                        else    new_pos = random(NUM_LEDS);
                                CHSV new_hsv = led_hsv;
                                CRGB new_rgb;                            
                                new_hsv.hue += (uint8_t)random(tostate_.param[2]) - tostate_.param[2] / 2;
                                new_hsv.sat = 255; new_hsv.val = 255;
                                hsv2rgb_rainbow(new_hsv, new_rgb);                      
                                bright_[new_pos] = (uint8_t)random(tostate_.brightness);
                                buf_[new_pos] = new_rgb;
                                leds_[new_pos] = setColor(buf_[new_pos], bright_[new_pos]);
                            }                    
                    }
                    
                }
            }
            else    // if prev effect was not EFF_SPARKS
            {
                fromstate_ = tostate_;
                currentstate_ = tostate_;                    
                stateChanged_ = true;
                // DEBUGSTATE("update() fromstate_.EFF != EFF_SPARKS: tostate_ ", tostate_)
                // DEBUGSTATE("update() fromstate_.EFF != EFF_SPARKS: currentstate_ ", currentstate_)
                // DEBUGSTATE("update() fromstate_.EFF != EFF_SPARKS: fromstate_", fromstate_)
                // Serial.printf("update() fromstate_.EFF != EFF_SPARKS: led_[%u] {%u,%u,%u} \n",
                //               NUM_LEDS-1, leds_[NUM_LEDS-1].r, leds_[NUM_LEDS-1].g, leds_[NUM_LEDS-1].b);
            }
            break;
        #endif

        #if ENABLE_EFF_RAINBOW==1
        case EFF_RAINBOW:
            break;
        #endif

        #if ENABLE_EFF_FIRE==1
        case EFF_FIRE:
            break;
        #endif

        default:        
            tostate_ = profile_[EFF_PLAIN];
            currentstate_ = fromstate_ = tostate_;
            stateChanged_ = true;
            break;
        }
    }
    FastLED.show();
}

CRGB MYRGB::setColor(uint8_t _red, uint8_t _green, uint8_t _blue, uint8_t _brightness)
{
    CRGB        value;
    uint32_t    calc;
    calc    = (_red + 1) * (_brightness + 1) - 1;       value.r = calc >> 8;
    calc    = (_green + 1) * (_brightness + 1) - 1;     value.g = calc >> 8;
    calc    = (_blue + 1) * (_brightness + 1) - 1;      value.b = calc >> 8;
    return value;
}

CRGB MYRGB::setColor(const MYDATA &_state)
{
    if (_state.enabled)
        return setColor(_state.red, _state.green, _state.blue, _state.brightness);
    else
        return CRGB::Black;
}

CRGB MYRGB::setColor(const CRGB &_rgb, uint8_t _brightness)
{
    return setColor(_rgb.red, _rgb.green, _rgb.blue, _brightness);
}

void MYRGB::setDitherFill(uint16_t _red, uint16_t _green, uint16_t _blue, uint16_t _brightness, CRGB (&_output)[])
{
    uint64_t    r = ((uint64_t)_red) * ((uint64_t)_brightness);
    uint64_t    g = ((uint64_t)_green) * ((uint64_t)_brightness);
    uint64_t    b = ((uint64_t)_blue) * ((uint64_t)_brightness);
    uint64_t    quant_error_red = 0, quant_error_green = 0, quant_error_blue = 0;    
    uint64_t    calc, red, green, blue;  
    CRGB        value;  
    for (int i = 0; i < NUM_LEDS; i++)
    {
        calc = r + quant_error_red;
        red = ((calc >> 24) > 255) ? 255 : (calc >> 24);
        quant_error_red = calc - (red << 24);
        calc = g + quant_error_green;
        green = ((calc >> 24) > 255) ? 255 : (calc >> 24);
        quant_error_green = calc - (green << 24);
        calc = b + quant_error_blue;
        blue = ((calc >> 24) > 255) ? 255 : (calc >> 24);
        quant_error_blue = calc - (blue << 24);
        value.red = red;        value.green = green;        value.blue = blue;
        _output[i] = value;
    }
}

void MYRGB::setDitherFill(const MYDATA &_state, CRGB (&_output)[])
{
    if (_state.enabled)
    {
        setDitherFill((uint16_t)_state.red << 8, (uint16_t)_state.green << 8, (uint16_t)_state.blue << 8,
                      (uint16_t)_state.brightness << 8, _output);
    }
    else
        for (int i = 0; i < NUM_LEDS; i++)
        {
            _output[i] = CRGB::Black;
        }
}

void MYRGB::setDitherFill(const CRGB &_rgb, uint8_t _brightness, CRGB (&_output)[])
{
    setDitherFill((uint16_t)_rgb.red << 8, (uint16_t)_rgb.green << 8, (uint16_t)_rgb.blue << 8, (uint16_t)_brightness << 8, _output);
}

MYRGB::PARAM_T MYRGB::getParam(int _i)
{
    PARAM_T value;
    if ((_i >= 0) && (_i <= 4))
        value = tostate_.param[_i];
    else
        value = -1;
    return value;
}
void MYRGB::setParam(const PARAM_T& _value, int _i)
{
    if ((_i >= 0) && (_i <= 4))
    {
        tostate_.param[_i] = _value;
        profile_[tostate_.effectIndex] = tostate_;
    }
}