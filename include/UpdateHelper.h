#ifndef __UPDATEHELPER_H
#define __UPDATEHELPER_H

#include <Update.h>
#include <FS.h>
#include <SD_MMC.h>

#define REMOTE_UPDATE_URL "http://us-central1-maustec-io.cloudfunctions.net/gh-release-embedded-bridge"

enum UpdateSource {
  NoUpdate,
  UpdateFromSD,
  UpdateFromServer
};

namespace UpdateHelper {
  // perform the actual update from a given stream
  void performUpdate(Stream &updateSource, size_t updateSize);

  // check given FS for valid update.bin and perform update if available
  void updateFromFS(fs::FS &fs);

  /**
   * Compares two versions, returning a value based on their SemVer equivalence:
   * -1 : a < b
   *  0 : a == b
   *  1 : a > b
   *
   * Any prefix 'v' will be ignored.
   * @param a SemVer string
   * @param b SemVer string
   * @return -1, 0, 1
   */
  int compareVersion(const char *a, const char *b);

  String checkWebLatestVersion();

  // Update Available
  UpdateSource checkForUpdates();
}

#endif
