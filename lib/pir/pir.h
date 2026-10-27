#pragma once
#include <Arduino.h>
#include "../../include/config.h"

class PIR
{
public:
    void begin();
    void update();
    bool detectaMovimento() const { return _movimento; }

private:
    bool _movimento = false;
    unsigned long _tempoInicio = 0;
};