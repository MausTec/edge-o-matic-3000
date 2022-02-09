#ifndef __Console_h
#define __Console_h

#include <cstring>
#include <string>

#define SERIAL_BUFFER_LEN 256
#define MAX_DIR_LENGTH 120

namespace Console {
  void loop();
  void handleMessage(char *line, char* out, size_t len);
  void handleMessage(char *line);

  namespace {
    char buffer[SERIAL_BUFFER_LEN] = {0};
    size_t buffer_i = 0;
    char cwd[MAX_DIR_LENGTH] = "/";
  }
}

#endif