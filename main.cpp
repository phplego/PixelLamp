#include <ArduinoOTA.h>
#include <ArduinoJson.h>
#include <FastLED.h>
#include <ESP8266WiFi.h>
#include <Adafruit_MQTT.h>
#include <Adafruit_MQTT_Client.h>
#include <EEPROM.h>
#include "WebService.h"
#include "utils.h"



#define APP_VERSION     "0.2"
#define DEVICE_ID       "PixelLamp"
#define LED_PIN         2

#define MQTT_HOST "192.168.1.157"   // MQTT host (e.g. m21.cloudmqtt.com)
#define MQTT_PORT 11883             // MQTT port (e.g. 18076)


CRGB leds[256] = {0};
#include "ledeffects.h"


const char* gConfigFile = "/config.json";
byte gBrightness = 40;
bool gRestart = false;

WiFiClient          client;                      // WiFi Client
WiFiManager         wifiManager;                 // WiFi Manager
WebService          webService;    // Web Server

Adafruit_MQTT_Client mqtt(&client, MQTT_HOST, MQTT_PORT);   // MQTT client
Adafruit_MQTT_Subscribe mqtt_sub_set = Adafruit_MQTT_Subscribe (&mqtt, "wifi2mqtt/pixellamp/set", MQTT_QOS_1);


void saveTheConfig()
{
    int addr = 0;
    for (int i = 0; i < MODE_COUNT; i++)
    {
        EEPROM.put(addr++, gModeConfigs[i].speed);
        EEPROM.put(addr++, gModeConfigs[i].scale);
    }

    EEPROM.put(addr++, gCurrentMode);

    EEPROM.commit();
}

void loadTheConfig()
{
    int addr = 0;
    for (int i = 0; i < MODE_COUNT; i++)
    {
        EEPROM.get(addr++, gModeConfigs[i].speed);
        EEPROM.get(addr++, gModeConfigs[i].scale);
    }
    
    EEPROM.get(addr++, gCurrentMode);
}

void setup() 
{

    Serial.begin(115200, SERIAL_8N1, SERIAL_TX_ONLY);

    EEPROM.begin(512);

    // config loading
    loadTheConfig();


    // ЛЕНТА
    FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, 256);//.setCorrection( TypicalLEDStrip );
    FastLED.setBrightness(gBrightness);
    FastLED.setMaxPowerInVoltsAndMilliamps(5, 500);
    FastLED.clear();

    leds[0] = CRGB::Blue;
    leds[1] = CRGB::Magenta;
    leds[2] = CRGB::Red;
    leds[3] = CRGB::Green;
    leds[4] = CRGB::Blue;


    FastLED.show();
    delay(50);
    FastLED.show();


    // =====================  Setup WiFi ==================
    String apName = String("esp-") + DEVICE_ID + "-v" + APP_VERSION + "-" + ESP.getChipId();
    apName.replace('.', '_');
    WiFi.hostname(apName);

    wifiManager.setAPStaticIPConfig(IPAddress(10, 0, 1, 1), IPAddress(10, 0, 1, 1), IPAddress(255, 255, 255, 0));
    wifiManager.autoConnect(apName.c_str(), "12341234"); // IMPORTANT! Blocks execution. Waits until connected

    //WiFi.begin("OpenWrt_2GHz", "111");


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

    String menu;
        menu += "<div>";
        menu += "<a href='/'>index</a> ";
        menu += "<a href='/logout'>logout</a> ";
        menu += "<a href='/restart'>restart</a> ";
        menu += "</div><hr>";


    webService.server->on("/", [menu](){
        String str = ""; 
        str += menu;
        str += "<pre>";
        str += String() + "           Uptime: " + (millis() / 1000) + " \n";
        str += String() + "      FullVersion: " + ESP.getFullVersion() + " \n";
        str += String() + "      ESP Chip ID: " + ESP.getChipId() + " \n";
        str += String() + "       CpuFreqMHz: " + ESP.getCpuFreqMHz() + " \n";
        str += String() + "              VCC: " + ESP.getVcc() + " \n";
        str += String() + "         FreeHeap: " + ESP.getFreeHeap() + " \n";
        str += String() + "       SketchSize: " + ESP.getSketchSize() + " \n";
        str += String() + "  FreeSketchSpace: " + ESP.getFreeSketchSpace() + " \n";
        str += String() + "    FlashChipSize: " + ESP.getFlashChipSize() + " \n";
        str += String() + "FlashChipRealSize: " + ESP.getFlashChipRealSize() + " \n";
        str += "</pre>";

        for(int i = 0; i < MODE_COUNT; i++){
            str += "<button style='font-size:25px' onclick='document.location=\"/set-mode?mode="+String(i)+"\"'> ";
            str += String() + (gCurrentMode == i ? "* " : "") + gModeNames[i];
            str += "</button> "; 
        }
        str += "<br>"; 
        str += "<br>"; 

        str += "speed: " + String(gSpeed) + "<br>"; 
        for(int i = 5; i < 61; i+=5){
            str += " <button style='font-size:25px; width:70px' onclick='document.location=\"/set-speed?speed="+String(i)+"\"'> ";
            str += String() + (gSpeed == i ? "* " : "") + i;
            str += "</button>\n"; 
        }
        str += "<br><br>"; 

        str += "scale: " + String(gScale) + "<br>"; 
        for(int i = 5; i < 61; i+=5){
            str += " <button style='font-size:25px; width:70px' onclick='document.location=\"/set-scale?scale="+String(i)+"\"'> ";
            str += String() + (gScale == i ? "* " : "") + i;
            str += "</button>\n"; 
        }

        str += "<br><br>"; 

        str += "brightness: " + String(gBrightness) + "<br>"; 
        for(int i = 0; i < 255; i+=20){
            str += " <button style='font-size:25px' onclick='document.location=\"/set-brightness?brightness="+String(i)+"\"'> ";
            str += String() + (gBrightness == i ? "* " : "") + i;
            str += "</button>\n"; 
        }
        webService.server->send(200, "text/html; charset=utf-8", str);     
    });



    webService.server->on("/set-mode", [](){
        gCurrentMode = atoi(webService.server->arg(0).c_str());
        saveTheConfig();
        webService.server->sendHeader("Location", "/",true);   //Redirect to index
        webService.server->send(302, "text/plane","");
    });

    webService.server->on("/set-speed", [](){
        gModeConfigs[gCurrentMode].speed = atoi(webService.server->arg(0).c_str());
        saveTheConfig();
        webService.server->sendHeader("Location", "/",true);   //Redirect to index  
        webService.server->send(302, "text/plane","");
    });

    webService.server->on("/set-scale", [](){
        gModeConfigs[gCurrentMode].scale = atoi(webService.server->arg(0).c_str());
        saveTheConfig();
        webService.server->sendHeader("Location", "/",true);   //Redirect to index  
        webService.server->send(302, "text/plane","");
    });

    webService.server->on("/set-brightness", [](){
        gBrightness = atoi(webService.server->arg(0).c_str());          
        FastLED.setBrightness(gBrightness);
        saveTheConfig();
        webService.server->sendHeader("Location", "/",true);   //Redirect to index  
        webService.server->send(302, "text/plane","");
    });

    webService.server->on("/restart", [menu](){
        webService.server->sendHeader("Location", "/",true);   //Redirect to index  
        webService.server->send(200, "text/html", "<script> setTimeout(()=> document.location = '/', 1000) </script> restarting ESP ...");
        gRestart = true;
    });

    // Logout (reset wifi settings)
    webService.server->on("/logout", [menu](){
        if(webService.server->method() == HTTP_POST){
            webService.server->send(200, "text/html", "OK");
            ESP.reset();
        }
        else{
            String output = "";
            output += menu;
            output += String() + "<pre>";
            output += String() + "Wifi network: " + WiFi.SSID() + " \n";
            output += String() + "        RSSI: " + WiFi.RSSI() + " \n";
            output += String() + "    hostname: " + WiFi.hostname() + " \n";
            output += String() + "</pre>";
            output += "<form method='post'><button>Forget</button></form>";
            webService.server->send(400, "text/html", output);
        }
    });


    // Setup MQTT subscription for the 'set' topic.
    mqtt.subscribe(&mqtt_sub_set);

    
    mqtt_sub_set.setCallback([](char *str, uint16_t len){

        char buf [len + 1];
        buf[len] = 0;
        strncpy(buf, str, len);

        Serial.println(String("Got mqtt message: ") + buf);

        const int JSON_SIZE = 1024;

        DynamicJsonDocument root(JSON_SIZE);
        DeserializationError error = deserializeJson(root, buf);
        
        if(error) return;

        if(root.containsKey("mode"))
            gCurrentMode = root["mode"];           // 0..17

        if(root.containsKey("speed"))
            gModeConfigs[gCurrentMode].speed = root["speed"];          

        if(root.containsKey("scale"))
            gModeConfigs[gCurrentMode].scale = root["scale"];          
        
        if(root.containsKey("brightness")){
            gBrightness = root["brightness"];          
            FastLED.setBrightness(gBrightness);
        }

        saveTheConfig();
    });

    
    ArduinoOTA.onStart([]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH) {
            type = "sketch";
        } else { // U_FS
            type = "filesystem";
        }

        // NOTE: if updating FS this would be the place to unmount FS using FS.end()
        Serial.println("Start updating " + type);
    });
    ArduinoOTA.onEnd([]() {
        Serial.println("\nEnd");
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) {
        Serial.println("Auth Failed");
        } else if (error == OTA_BEGIN_ERROR) {
        Serial.println("Begin Failed");
        } else if (error == OTA_CONNECT_ERROR) {
        Serial.println("Connect Failed");
        } else if (error == OTA_RECEIVE_ERROR) {
        Serial.println("Receive Failed");
        } else if (error == OTA_END_ERROR) {
        Serial.println("End Failed");
        }
    });

    ArduinoOTA.begin();

    Serial.println("*** end setup ***");
}




void loop() 
{
    effectsTick();
    webService.loop();
    ArduinoOTA.handle();

    // Ensure the connection to the MQTT server is alive (this will make the first
    // connection and automatically reconnect when disconnected).  See the MQTT_connect()
    MQTT_connect(&mqtt);
        
        
    // wait X milliseconds for subscription messages
    mqtt.processPackets(10);

    if(gRestart){
        webService.loop();
        ESP.restart();
    }
}