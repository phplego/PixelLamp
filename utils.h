#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <Adafruit_MQTT_Client.h>


String getContentType(String filename);



// Forcibly mount the SPIFFS. Formatting the SPIFFS if needed.
//
// Returns:
//   A boolean indicating success or failure.
bool mountSpiffs(void);

/**
 * Returns contents of the file or empty string on fail.
 */
String fileGetContents(const char * filename);


bool loadConfig(const char * filename, std::function<void(DynamicJsonDocument)> onLoadCallback);
bool saveConfig(const char * filename, DynamicJsonDocument json) ;
bool saveConfig(const char * filename, String jsonStr);

// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect(Adafruit_MQTT_Client * mqtt);