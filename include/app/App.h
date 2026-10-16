#pragma once

#include "core/AppKernel.h"

// The firmware entry point (main.cpp calls setup()/loop()).
//
// It holds no kernel of its own. The kernel is the process-wide one behind
// globalAppKernel(), so the legacy menu -- which needs the runner to start a
// feature -- and the kernel's own policy gate cannot end up as two instances
// with two registries and two settings stores.
class App {
public:
    void setup();
    void loop();
};
