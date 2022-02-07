#ifndef __SDHelper_h
#define __SDHelper_h

#include <Arduino.h>
#include <SD.h>
#include <ArduinoJson.h>

namespace SDHelper {
  void printDirectory(File dir, int numTabs, String &out);
  void printDirectoryJson(File dir, JsonVariant files);
  void printFile(File file, String &out);
}

#endif