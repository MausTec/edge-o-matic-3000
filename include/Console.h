#ifndef __Console_h
#define __Console_h

#include "Arduino.h"

#define SERIAL_BUFFER_LEN 256

namespace Console {
  void loop();
  void handleMessage(char *line, String &out);
  void handleMessage(char *line);

  namespace {
    char buffer[SERIAL_BUFFER_LEN] = {0};
    size_t buffer_i = 0;
    String cwd = "/";
  }
}

#endif