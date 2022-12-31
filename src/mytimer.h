#ifndef __INCLUDE_MYTIMER
#define __INCLUDE_MYTIMER

#include "Arduino.h"

struct MYTIMER {
    
public:    
    MYTIMER(unsigned long _period);
    void start(bool _repeat = true);
    bool isRunning(void);
    void stop(void);

    bool click(void);

    void setPeriod(unsigned long _period);
    unsigned long getPeriod(void);

    uint8_t progress8(void);
    uint16_t progress16(void);
    uint32_t progress32(void);

private:
    unsigned long period_;
    unsigned long startMillis_;
    bool isrunning_;
    bool repeat_;
    uint32_t progress_();
};

#endif