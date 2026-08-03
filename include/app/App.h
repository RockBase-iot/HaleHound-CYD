#pragma once

#include "core/AppKernel.h"

class App {
public:
    void setup();
    void loop();

private:
    AppKernel kernel_;
};
