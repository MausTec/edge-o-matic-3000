#include "UpdateHelper.h"

#include <SD.h>

namespace UpdateHelper {
  // perform the actual update from a given stream
  void performUpdate(Stream &updateSource, size_t updateSize) {
    if (Update.begin(updateSize)) {
      size_t written = Update.writeStream(updateSource);
      if (written == updateSize) {
        Serial.println("Written : " + String(written) + " successfully");
      } else {
        Serial.println("Written only : " + String(written) + "/" + String(updateSize) + ". Retry?");
      }
      if (Update.end()) {
        Serial.println("OTA done!");
        if (Update.isFinished()) {
          Serial.println("Update successfully completed. Rebooting.");
        } else {
          Serial.println("Update not finished? Something went wrong!");
        }
      } else {
        Serial.println("Error Occurred. Error #: " + String(Update.getError()));
      }

    } else {
      Serial.println("Not enough space to begin OTA");
    }
  }

  // check given FS for valid update.bin and perform update if available
  void updateFromFS(fs::FS &fs) {
    File updateBin = fs.open("/update.bin");
    if (updateBin) {
      if (updateBin.isDirectory()) {
        Serial.println("Error, update.bin is not a file");
        updateBin.close();
        return;
      }

      size_t updateSize = updateBin.size();

      if (updateSize > 0) {
        Serial.println("Try to start update");
        performUpdate(updateBin, updateSize);
      } else {
        Serial.println("Error, file is empty");
      }

      updateBin.close();

      // whe finished remove the binary from sd card to indicate end of the process
      fs.remove("/update.bin");
    } else {
      Serial.println("Could not load update.bin from sd root");
    }
  }

  UpdateSource checkForUpdates() {
    // Check SD first:
    bool updateBin = SD.exists("/update.bin");
    if (updateBin) {
      Serial.println("Found: update.bin on SD card.");
      return UpdateFromSD;
    }

    // Check Network:
    // noop

    Serial.println("No updates available.");
    return NoUpdate;
  }
}