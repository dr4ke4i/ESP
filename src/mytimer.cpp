#include "Arduino.h"
#include "mytimer.h"

// MYTIMER (_period (ms))
MYTIMER::MYTIMER(unsigned long _period)
    :period_(_period), isrunning_(false), repeat_(false)
{    }

// (re)start the timer
void MYTIMER::start(bool _repeat)
{
    repeat_ = _repeat;
    isrunning_ = true;
    startMillis_ = millis();
}

// check if the timer will ever return click() as true
bool MYTIMER::isRunning()
{
    return isrunning_;
}

// stop the timer
void MYTIMER::stop()
{
    isrunning_ = false;
}

// check if the timer has fired
bool MYTIMER::click()
{
    if (!isrunning_)
        { return false; }
    else if (millis() - startMillis_ >= period_)
        {
            if (repeat_)
                { start(); }
            else
                { stop(); }
            return true;
        }
    else
        { return false; }
}

// set the timer period (ms)
void MYTIMER::setPeriod(unsigned long _period)
{
    period_ = _period;
}

// get the timer period (ms)
unsigned long MYTIMER::getPeriod()
{
    return period_;
}

uint32_t MYTIMER::progress_()
{
    if (isrunning_)
    {
        uint64_t result = millis() - startMillis_;
        result = (result << 32) / period_;
        result = result > ((((uint64_t)1) << 32) - 1) ? ((((uint64_t)1) << 32) - 1) : result;            
        return (uint32_t)result;    
    }
    else
        return 0;
}

uint32_t MYTIMER::progress32()
{
    return progress_();
}

uint16_t MYTIMER::progress16()
{    
    return (uint16_t)((progress_()) >> 16);
}

uint8_t MYTIMER::progress8()
{
    return (uint8_t)((progress_()) >> 24);
}