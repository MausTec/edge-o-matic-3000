#include "UpdateHelper.h"
#include "UserInterface.h"
#include "WiFiHelper.h"
#include "VERSION.h"

#include <SD.h>
#include <HTTPClient.h>

#define UPDATE_BUFFER_SIZE (1024 * 1)

namespace UpdateHelper {
  bool pendingLocalUpdate = false;
  bool pendingWebUpdate = false;

  size_t waitUntilAvailable(Stream *stream, long timeout_ms = 10000) {
    long start_ms = millis();
    size_t available = 0;
    while(available == 0 && millis() - start_ms < timeout_ms) {
      available = stream->available();
    }
    return available;
  }

  // perform the actual update from a given stream
  bool performUpdate(Stream &updateSource, size_t updateSize) {
    Serial.println("Starting update...");

    if (Update.begin(updateSize)) {
      Serial.println("Update began.");
      size_t written = 0;
      size_t availableBytes = 0;
      byte buffer[UPDATE_BUFFER_SIZE];

      while ((availableBytes = updateSource.available()) > 0) {
        size_t read = updateSource.readBytes(buffer, UPDATE_BUFFER_SIZE);
        written += Update.write(buffer, read);
        log_d("Read %d bytes, Written %d/%d bytes. (Heap Free: %d bytes)", read, written, updateSize, xPortGetFreeHeapSize());
        UI.toastProgress("Updating...", (float)written/updateSize);

        if (written < updateSize) {
          waitUntilAvailable(&updateSource);
        }
      }

      log_w("Stream ended. Last available: %d", availableBytes);

      if (written == updateSize) {
        Serial.println("Written : " + String(written) + " successfully");
        UI.toastNow("Update complete!");
      } else {
        Serial.println("Written only : " + String(written) + "/" + String(updateSize) + ". Retry?");
        UI.toastNow("Update failed:\nCould not write.");
      }

      if (Update.end()) {
        Serial.println("OTA done!");

        if (Update.isFinished()) {
          Serial.println("Update successfully completed. Rebooting.");
          UI.toastNow("Update complete!\nRebooting...");
          return true;

        } else {
          Serial.println("Update not finished? Something went wrong!");
          UI.toastNow("?????");
        }
      } else {
        Serial.println("Error Occurred. Error #: " + String(Update.getError()));
        UI.toastNow("Update error:\n" + String(Update.getError()));
      }

    } else {
      Serial.println("Not enough space to begin OTA");
      UI.toastNow("No space!");
    }

    return false;
  }

  // check given FS for valid update.bin and perform update if available
  void updateFromFS(fs::FS &fs) {
    File updateBin = fs.open("/update.bin");
    bool success = false;

    if (updateBin) {
      if (updateBin.isDirectory()) {
        Serial.println("Error, update.bin is not a file");
        UI.toastNow("Invalid file:\nupdate.bin [DIR]");
        updateBin.close();
        return;
      }

      size_t updateSize = updateBin.size();

      if (updateSize > 0) {
        Serial.println("Try to start update");
        UI.toastNow("Updating...\n[#         ]  10%");
        success = performUpdate(updateBin, updateSize);
      } else {
        UI.toastNow("Update file empty!");
        Serial.println("Error, file is empty");
      }

      updateBin.close();
      UI.toastNow("Updating...\n[##########] 100%");

      if (success) {
        // whe finished remove the binary from sd card to indicate end of the process
        // fs.mv("/update.bin", "/update-" VERSION ".bin");
        delay(2000);
        ESP.restart();
      }
    } else {
      Serial.println("Could not load update.bin from sd root");
    }
  }

  UpdateSource checkForUpdates() {
    UpdateSource source = NoUpdate;
    // Check SD first:
    bool updateBin = SD.exists(UPDATE_FILENAME);
    if (updateBin) {
      Serial.println("Found: update.bin on SD card.");
      pendingLocalUpdate = true;
      source = UpdateFromSD;
    }

    // Check Network:
    if (WiFiHelper::connected()) {
      String web = checkWebLatestVersion();
      Serial.print("Web version was: ");
      Serial.println(web);

      if (compareVersion(web.c_str(), VERSION) > 0) {
        Serial.println("Web Update available!");
        pendingWebUpdate = true;
        source = UpdateFromServer;
      }
    }

    if (pendingLocalUpdate || pendingWebUpdate) {
      UI.drawUpdateIcon(1);
    }

    return source;
  }

  String checkWebLatestVersion() {
    String version;
    HTTPClient http;
    http.begin(REMOTE_UPDATE_URL "/version.txt");
    int httpCode = http.GET();

    if (httpCode > 0) {
      if (httpCode == HTTP_CODE_OK) {
        version = http.getString();
      } else {
        Serial.printf("GET Error: %d\n", httpCode);
      }
    } else {
      Serial.printf("GET Error: %s\n", http.errorToString(httpCode).c_str());
    }

    http.end();
    return version;
  }

  String followRedirects(String url, int &httpCode) {
    String location = url;
    int redirectCount = 0;

    while (redirectCount < 10) {
      HTTPClient http;
      http.begin(location.c_str());

      // We want to keep the "Location" header.
      const char *collectHeaders[] = { "Location" };
      http.collectHeaders(collectHeaders, 1);

      httpCode = http.GET();

      if (httpCode > 0) {
        if (http.hasHeader("Location")) {
          location = http.header("Location");
          Serial.print("Redirect: ");
          Serial.println(location);
        } else if (httpCode == HTTP_CODE_OK) {
          break;
        } else {
          Serial.printf("GET Error from File: %d\n", httpCode);
          break;
        }
      } else {
        Serial.printf("GET Error: %s\n", http.errorToString(httpCode).c_str());
      }

      http.end();
      redirectCount++;
    }

    return location;
  }

  void updateFromWeb() {
    HTTPClient fileHttp;
    int httpCode = 0;
    String location = followRedirects(REMOTE_UPDATE_URL "/update.bin", httpCode);

    if (httpCode == HTTP_CODE_OK) {
      fileHttp.begin(location.c_str());
      httpCode = fileHttp.GET();

      if (httpCode > 0) {
        if (httpCode == HTTP_CODE_OK) {
          int len = fileHttp.getSize();
          Serial.printf("Got size: %d bytes\n", len);
          Stream *stream = fileHttp.getStreamPtr();
          size_t available = waitUntilAvailable(stream);

          if (available > 0) {
            bool success = performUpdate(*stream, len);
            if (success) {
              delay(2000);
              ESP.restart();
            }
          } else {
            Serial.printf("%d bytes sent from server.\n", available);
          }
        } else {
          Serial.printf("GET Error from File: %d\n", httpCode);
        }
      } else {
        Serial.printf("GET Error: %s\n", fileHttp.errorToString(httpCode).c_str());
      }

      fileHttp.end();
    }
  }

  int compareVersion(const char *a, const char *b) {
    int va[3] = { 0, 0, 0 };
    int vb[3] = { 0, 0, 0 };

    sscanf(a[0] == 'v' ? a + 1 : a, "%d.%d.%d", &va[0], &va[1], &va[2]);
    sscanf(b[0] == 'v' ? b + 1 : b, "%d.%d.%d", &vb[0], &vb[1], &vb[2]);

    for (int i = 0; i <= 2; i++) {
      if (va[i] != vb[i]) {
        return va[i] > vb[i] ? 1 : -1;
      }
    }

    return 0;
  }
}
