#include "app/App.h"

void App::setup() {
    kernel_.setup();
}

void App::loop() {
    kernel_.tick();
}
