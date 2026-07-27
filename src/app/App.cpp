#include "app/App.h"
#include "app/LegacyApp.h"

void App::setup() {
    legacySetup();
}

void App::loop() {
    legacyLoop();
}