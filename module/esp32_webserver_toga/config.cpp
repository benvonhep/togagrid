#include <AsyncWiFiMulti.h>
#include <Arduino.h>
#include <FS.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include "togaws_config.h"

/** Loads the configuration from a file */
void Config::loadConfiguration(const char *filename) {
  // Open file for reading
  File file = SPIFFS.open(filename, "r");

  // Allocate a temporary JsonDocument
  // Don't forget to change the capacity to match your requirements.
  // Use arduinojson.org/v6/assistant to compute the capacity.
  StaticJsonDocument<512> doc;

  // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, file);
  if (error)
    Serial.println(F("Failed to read file, using default configuration"));

  // Copy values from the JsonDocument to the Config
  leds = doc["leds"] | 10;
  strlcpy(hostname, doc["hostname"] | WiFi.getHostname(), sizeof(hostname));

  // Close the file (Curiously, File's destructor doesn't close the file)
  file.close();
}

/** Loads the configuration from a string */
void Config::loadConfiguration(const String &str) {
  // Allocate a temporary JsonDocument
  // Don't forget to change the capacity to match your requirements.
  // Use arduinojson.org/v6/assistant to compute the capacity.
  StaticJsonDocument<512> doc;

  // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, str);
  if (error)
    Serial.println(F("Failed to deserialize, using default configuration"));

  // Copy values from the JsonDocument to the Config
  leds = doc["leds"] | 10;
  strlcpy(hostname, doc["hostname"] | WiFi.getHostname(), sizeof(hostname));
}

bool Config::writeConfiguration(File file) {
  // Allocate a temporary JsonDocument
  // Don't forget to change the capacity to match your requirements.
  // Use arduinojson.org/assistant to compute the capacity.
  StaticJsonDocument<256> doc;

  doc["hostname"] = hostname;
  doc["leds"] = leds;

  if (serializeJson(doc, file) == 0) {
    Serial.println(F("Failed to write to file"));
    return false;
  }
  return true;
}

bool Config::writeConfiguration(String &str) {
  // Allocate a temporary JsonDocument
  // Don't forget to change the capacity to match your requirements.
  // Use arduinojson.org/assistant to compute the capacity.
  StaticJsonDocument<256> doc;

  doc["hostname"] = hostname;
  doc["leds"] = leds;

  if (serializeJson(doc, str) == 0) {
    Serial.println(F("Failed to write to serialize"));
    return false;
  }
  return true;
}

/** Saves the configuration to a file */
bool Config::saveConfiguration(const char *filename) {
  // Delete existing file, otherwise the configuration is appended to the file
  // SD.remove(filename);

  // Open file for writing
  File file = SPIFFS.open(filename, FILE_WRITE);
  if (!file) {
    Serial.println(F("Failed to create file"));
    return false;
  }

  writeConfiguration(file);

  file.close();
  return true;
}

void Config::printFile(const char *filename) {
  File file = SPIFFS.open(filename, FILE_READ);
  if (!file) {
    Serial.println(F("Failed to read file"));
    return;
  }

  while (file.available()) {
    Serial.print((char)file.read());
  }
  Serial.println();

  file.close();
}
