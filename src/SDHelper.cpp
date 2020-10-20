#include "../include/SDHelper.h"

namespace SDHelper {
  void printDirectory(File dir, int numTabs, String &out) {
    while (true) {
      File entry =  dir.openNextFile();
      if (! entry) {
        // no more files
        break;
      }
      for (uint8_t i = 0; i < numTabs; i++) {
        out += "\t";
      }
      out += String(entry.name());
      if (entry.isDirectory()) {
        out += "/\n";
        printDirectory(entry, numTabs + 1, out);
      } else {
        // files have sizes, directories do not
        out += "\t\t";
        out += String(entry.size()) + "\n";
      }
      entry.close();
    }
  }

  void printFile(File file, String &out) {
    while (file.available()) {
      out += String((char)file.read());
    }
    file.close();
  }
}