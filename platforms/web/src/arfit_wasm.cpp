#include "arfit_kit.h"
#include <emscripten/bind.h>

using namespace emscripten;
using namespace arfit;

// Embind wrapper for ARFitKit
EMSCRIPTEN_BINDINGS(arfit_kit_module) {
  class_<SessionConfig>("SessionConfig")
      .constructor<>()
      .property("targetFPS", &SessionConfig::targetFPS)
      .property("enableClothSimulation", &SessionConfig::enableClothSimulation);

  class_<ARFitKit>("ARFitKit")
      .constructor<>()
      .function("initialize", &ARFitKit::initialize)
      .function("startSession", &ARFitKit::startSession)
      .function("stopSession", &ARFitKit::stopSession)
      .function("getCurrentFPS", &ARFitKit::getCurrentFPS);

  // ImageData type mapping needed
  // ...
}
