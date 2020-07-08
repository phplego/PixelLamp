#pragma once

#include <Arduino.h>
#include <WiFiManager.h>
#include <ESP8266WebServer.h>
#include <FS.h>
#include "utils.h"


class WebService {
    public:
        ESP8266WebServer*   server;
    
    public:
        // Default Constructor 
        WebService(); 
        void init();
        void loop();

    private:
        bool handleFileRead(String path);
};