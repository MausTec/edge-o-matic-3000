#ifndef __SDHelper_h
#define __SDHelper_h

#import <Arduino.h>
#import <SD.h>

namespace SDHelper {
  void printDirectory(File dir, int numTabs, String &out);
  void printFile(File file, String &out);
}

#endif