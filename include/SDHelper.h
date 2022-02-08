#ifndef __SDHelper_h
#define __SDHelper_h

#include <Arduino.h>
#include <ArduinoJson.h>
#include <stdio.h>

namespace SDHelper {
  void printDirectory(FILE *dir, int numTabs, String &out);
  void printDirectoryJson(FILE *dir, JsonVariant files);
  void printFile(FILE *file, String &out);
}

#endif