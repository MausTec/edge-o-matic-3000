#ifndef __SDHelper_h
#define __SDHelper_h

#include <stddef.h>
#include <stdio.h>
#include "cJSON.h"

namespace SDHelper {
  void printDirectory(FILE *dir, int numTabs, char *out, size_t len);
  void printDirectoryJson(FILE *dir, cJSON *files);
  void printFile(FILE *file, char *out, size_t len);
}

#endif