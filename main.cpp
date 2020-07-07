#include <ArduinoOTA.h>
#include <FastLED.h>
#include <ESP8266WiFi.h>
#include "WebService.h"


#define APP_VERSION     "0.1"
#define DEVICE_ID       "PixelLamp"


CRGB leds[256] = {0};
#include "ledeffects.h"



WiFiManager         wifiManager;                 // WiFi Manager
WebService          webService(&wifiManager);    // Web Server


void setup() 
{

    Serial.begin(115200, SERIAL_8N1, SERIAL_TX_ONLY);

    // ЛЕНТА
    FastLED.addLeds<WS2812B, 2, GRB>(leds, 256);//.setCorrection( TypicalLEDStrip );
    FastLED.setBrightness(40);
    FastLED.setMaxPowerInVoltsAndMilliamps(5, 200);
    FastLED.clear();

    leds[0] = CRGB::Blue;
    leds[1] = CRGB::Magenta;
    leds[2] = CRGB::Red;
    leds[3] = CRGB::Green;
    leds[4] = CRGB::Blue;

    leds[16+0] = CRGB::Blue;
    leds[16+1] = CRGB::Magenta;
    leds[16+2] = CRGB::Red;
    leds[16+3] = CRGB::Green;
    leds[16+4] = CRGB::Blue;

    leds[32+0] = CRGB::Blue;
    leds[32+1] = CRGB::Magenta;
    leds[32+2] = CRGB::Red;
    leds[32+3] = CRGB::Green;
    leds[32+4] = CRGB::Blue;

    FastLED.show();
    delay(50);
    FastLED.show();


    // =====================  Setup WiFi ==================
    String apName = String("esp-") + DEVICE_ID + "-v" + APP_VERSION + "-" + ESP.getChipId();
    apName.replace('.', '_');
    WiFi.hostname(apName);

    wifiManager.setAPStaticIPConfig(IPAddress(10, 0, 1, 1), IPAddress(10, 0, 1, 1), IPAddress(255, 255, 255, 0));
    wifiManager.autoConnect(apName.c_str(), "12341234"); // IMPORTANT! Blocks execution. Waits until connected

    // Wait for WIFI connection

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(10);
        Serial.print(".");
    }

    Serial.print("\nConnected to ");
    Serial.println(WiFi.SSID());
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    // =====================  End Setup  ==================
 
    // socketService.init();
    webService.init();

    webService.server->on("/set-mode", [](){
        currentMode = atoi(webService.server->arg(0).c_str());
        //webService.server->send(200, "text/html", "OK");
        webService.server->sendHeader("Location", "/",true);   //Redirect to our html web page  
        webService.server->send(302, "text/plane","");
    });


    ArduinoOTA.begin();

    Serial.println("*** end setup ***");
}




void loop() 
{
    effectsTick();
    webService.loop();
    ArduinoOTA.handle();
    delay(55);

}