#include "app/App.h"

#include "TftFeatureCanvas.h"

// The panel instance the legacy screens drive, defined in LegacyApp.inc. Sharing
// the one instance is the point: a feature painting through the canvas and a
// legacy screen painting through tft must be the same piece of glass.
extern TFT_eSPI tft;

void App::setup() {
    // Constructed on first use rather than at static-init time, so it cannot run
    // before the panel it refers to has been constructed.
    static TftFeatureCanvas canvas(tft);
    globalAppKernel().setCanvas(&canvas);
    globalAppKernel().setup();
}

void App::loop() {
    globalAppKernel().tick();
}
