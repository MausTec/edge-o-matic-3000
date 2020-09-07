#ifndef __OrgasmControl_h
#define __OrgasmControl_h

#include "Arduino.h"
#include "../config.h"

namespace OrgasmControl {
  void tick();

  // Fetch Data


  namespace {
    long last_update_ms = 0;

    // Orgasmo Calculations
    uint16_t last_value = 0;
    uint16_t peak_start = 0;



    void updateCounts();
  }
}

#endif