#include "../include/Console.h"

namespace Console {
  namespace {
    /**
     * Handles the message currently in the buffer.
     */
    void handleMessage() {
      char *cmd = strtok(buffer, " ");

      Serial.println("> " + String(cmd) + "(" + String(buffer + strlen(cmd) + 1) + ")");

      // Reset buffer
      buffer_i = 0;
      buffer[buffer_i] = 0;
    }
  }

  /**
   * Reads and stores incoming bytes onto the buffer until
   * we have a newline.
   */
  void loop() {
    while (Serial.available() > 0) {
      // read the incoming byte:
      char incoming = Serial.read();

      if (incoming == '\n') {
        handleMessage();
      } else {
        buffer[buffer_i] = incoming;
        buffer_i++;
        buffer[buffer_i] = 0;
      }
    }
  }
}