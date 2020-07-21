#include <ArduinoOTA.h>
#include <ArduinoJson.h>
#include <FastLED.h>
#include <ESP8266WiFi.h>
#include <Adafruit_MQTT.h>
#include <Adafruit_MQTT_Client.h>
#include <EEPROM.h>
#include "WebService.h"
#include "Queue.h"
#include "utils.h"


ADC_MODE(ADC_VCC); // for make ESP.getVCC() work

#define APP_VERSION         "0.5"
#define DEVICE_ID           "PixelLamp"
#define LED_PIN             0
#define MOSFET_PIN          3 //RX
#define VCC2BAT_CORRECTION  0.7f
#define TURN_OFF_VOLTAGE    3.7f

#define MQTT_HOST               "192.168.1.157"   // MQTT host (e.g. m21.cloudmqtt.com)
#define MQTT_PORT               11883             // MQTT port (e.g. 18076)
#define MQTT_PUBLISH_INTERVAL   60000             // 1 min
#define VCC_MEASURE_INTERVAL    1000              // 1 sec

CRGB leds[256] = {0};
#include "ledeffects.h"


const char *        gConfigFile = "/config.json";
byte                gBrightness = 40;
unsigned long       gRestart = 0;
Queue<20>           vccQueue;


WiFiClient          client;                      // WiFi Client
WiFiManager         wifiManager;                 // WiFi Manager
WebService          webService;                  // Web Server



Adafruit_MQTT_Client    mqtt(&client, MQTT_HOST, MQTT_PORT);   // MQTT client
Adafruit_MQTT_Subscribe mqttSubscription    = Adafruit_MQTT_Subscribe (&mqtt, "wifi2mqtt/pixellamp/set");
Adafruit_MQTT_Publish   mqttPublish         = Adafruit_MQTT_Publish   (&mqtt, "wifi2mqtt/pixellamp");


unsigned long lastPublishTime = 0;
unsigned long lastVccMeasureTime = 0;

float getVcc(){
    return (float)ESP.getVcc() / 1024.0f;
}

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

    DynamicJsonDocument json(1024);
    json["temp"]   = 123;
    json["mode"]   = 123;
    saveConfig(gConfigFile, json);

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

void publishState()
{
    String jsonStr;
    StaticJsonDocument<512> doc;
    float vccRouned = round(vccQueue.average() * 100.0) / 100.0;

    doc["mode"]         = gCurrentMode;
    doc["mode-name"]    = gModeStructs[gCurrentMode].name;
    doc["speed"]        = gModeConfigs[gCurrentMode].speed;
    doc["scale"]        = gModeConfigs[gCurrentMode].scale;
    doc["brightness"]   = gBrightness;
    doc["vcc"]          = vccRouned;
    doc["vbat"]         = vccRouned + VCC2BAT_CORRECTION;
    doc["version"]      = APP_VERSION;

    serializeJson(doc, jsonStr);

    // Ensure the connection to the MQTT server is alive (this will make the first
    // connection and automatically reconnect when disconnected).  See the MQTT_connect()
    MQTT_connect(&mqtt);

    // Publish state to output topic
    mqttPublish.publish(jsonStr.c_str());
}

void setup() 
{

    Serial.begin(115200, SERIAL_8N1, SERIAL_TX_ONLY);

    EEPROM.begin(512);


    //pinMode(1 /*TX*/, OUTPUT);
    //pinMode(3 /*RX*/, OUTPUT);

    pinMode(MOSFET_PIN, OUTPUT);
    digitalWrite(MOSFET_PIN, HIGH);

    // config loading
    loadTheConfig();

    // measure vcc first time
    vccQueue.add(getVcc());

    // ЛЕНТА
    FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, 256);//.setCorrection( TypicalLEDStrip );
    FastLED.setBrightness(gBrightness);
    FastLED.setMaxPowerInVoltsAndMilliamps(5, 5000);
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
    wifi_set_sleep_type(NONE_SLEEP_T); //prevent wifi sleep (stronger connection)


    wifiManager.setAPStaticIPConfig(IPAddress(10, 0, 1, 1), IPAddress(10, 0, 1, 1), IPAddress(255, 255, 255, 0));
    wifiManager.setConfigPortalTimeout(60);
    wifiManager.autoConnect(apName.c_str(), "12341234"); // IMPORTANT! Blocks execution. Waits until connected

    //WiFi.begin("OpenWrt_2GHz", "111");


    // Restart if not connected
    if (WiFi.status() != WL_CONNECTED)
    {
        ESP.restart();
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
        menu += "<a href='/turn-off'>turn off</a> ";
        menu += "<a href='/index2.html'>index2.html</a> ";
        menu += "</div><hr>";


    webService.server->on("/", [menu](){
        String str = ""; 
        str += menu;
        str += "<pre>";
        str += String() + "           Uptime: " + (millis() / 1000) + " \n";
        str += String() + "      FullVersion: " + ESP.getFullVersion() + " \n";
        str += String() + "      ESP Chip ID: " + ESP.getChipId() + " \n";
        str += String() + "       CpuFreqMHz: " + ESP.getCpuFreqMHz() + " \n";
        str += String() + "              VCC: " + vccQueue.average() + " \n";
        str += String() + "  ~Battery(aprox): " + (vccQueue.average() + VCC2BAT_CORRECTION) + " \n";
        str += String() + "      WiFi status: " + client.status() + " \n";
        str += String() + "         FreeHeap: " + ESP.getFreeHeap() + " \n";
        str += String() + "       SketchSize: " + ESP.getSketchSize() + " \n";
        str += String() + "  FreeSketchSpace: " + ESP.getFreeSketchSpace() + " \n";
        str += String() + "    FlashChipSize: " + ESP.getFlashChipSize() + " \n";
        str += String() + "FlashChipRealSize: " + ESP.getFlashChipRealSize() + " \n";
        str += "</pre>";

        for(int i = 0; i < MODE_COUNT; i++){
            str += "<button style='font-size:25px' onclick='document.location=\"/set-mode?mode="+String(i)+"\"'> ";
            str += String() + (gCurrentMode == i ? "* " : "") + gModeStructs[i].name;
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
        str += " <button style='font-size:25px; width:70px' onclick='document.location=\"/set-scale?scale="+String(gScale-1)+"\"'>-1</button> ";
        str += " <button style='font-size:25px; width:70px' onclick='document.location=\"/set-scale?scale="+String(gScale+1)+"\"'>+1</button> ";
        str += "&nbsp;&nbsp;";
        for(int i = 0; i < 61; i+=5){
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
        gCurrentMode = atoi(webService.server->arg(0).c_str()) % MODE_COUNT;
        saveTheConfig();
        publishState();
        webService.server->sendHeader("Location", "/",true);   //Redirect to index
        webService.server->send(302, "text/plane","");
    });

    webService.server->on("/set-speed", [](){
        gModeConfigs[gCurrentMode].speed = atoi(webService.server->arg(0).c_str());
        saveTheConfig();
        publishState();
        webService.server->sendHeader("Location", "/",true);   //Redirect to index  
        webService.server->send(302, "text/plane","");
    });

    webService.server->on("/set-scale", [](){
        gModeConfigs[gCurrentMode].scale = atoi(webService.server->arg(0).c_str());
        saveTheConfig();
        publishState();
        webService.server->sendHeader("Location", "/",true);   //Redirect to index  
        webService.server->send(302, "text/plane","");
    });

    webService.server->on("/set-brightness", [](){
        gBrightness = atoi(webService.server->arg(0).c_str());          
        FastLED.setBrightness(gBrightness);
        saveTheConfig();
        publishState();
        webService.server->sendHeader("Location", "/",true);   //Redirect to index  
        webService.server->send(302, "text/plane","");
    });

    webService.server->on("/restart", [menu](){
        webService.server->sendHeader("Location", "/",true);   //Redirect to index  
        webService.server->send(200, "text/html", "<script> setTimeout(()=> document.location = '/', 5000) </script> restarting ESP ...");
        gRestart = millis();
    });

    webService.server->on("/turn-off", [menu](){
        if(webService.server->method() == HTTP_POST){
            webService.server->send(200, "text/html", "OK");
            digitalWrite(MOSFET_PIN, LOW);
            ESP.deepSleep(0);
        }
        else{
            String output = "";
            output += menu;
            output += "<form method='post'><button>Turn off the lamp</button></form>";
            webService.server->send(200, "text/html", output);
        }
    });

    // Logout (reset wifi settings)
    webService.server->on("/logout", [menu](){
        if(webService.server->method() == HTTP_POST){
            webService.server->send(200, "text/html", "OK");
            wifiManager.resetSettings();
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
            webService.server->send(200, "text/html", output);
        }
    });


    // Setup MQTT subscription for the 'set' topic.
    mqtt.subscribe(&mqttSubscription);

    
    mqttSubscription.setCallback([](char *str, uint16_t len){

        char buf [len + 1];
        buf[len] = 0;
        strncpy(buf, str, len);

        Serial.println(String("Got mqtt message: ") + buf);

        const int JSON_SIZE = 1024;

        DynamicJsonDocument root(JSON_SIZE);
        DeserializationError error = deserializeJson(root, buf);
        
        if(error) return;

        if(root.containsKey("mode")){
            int mode = root["mode"];
            while(mode < 0) mode += MODE_COUNT;
            gCurrentMode = (mode % MODE_COUNT);           // 0..17
        }


        if(root.containsKey("speed"))
            gModeConfigs[gCurrentMode].speed = root["speed"];          

        if(root.containsKey("scale"))
            gModeConfigs[gCurrentMode].scale = root["scale"];          
        
        if(root.containsKey("brightness")){
            gBrightness = root["brightness"];          
            FastLED.setBrightness(gBrightness);
        }

        saveTheConfig();
        publishState();
    });

    
    ArduinoOTA.begin();

    //myBroker.init();


    Serial.println("*** end setup ***");
    

    publishState();
}


void loop() 
{
    effectsLoop();
    webService.loop();
    ArduinoOTA.handle();

    // Ensure the connection to the MQTT server is alive (this will make the first
    // connection and automatically reconnect when disconnected).  See the MQTT_connect()
    MQTT_connect(&mqtt);


    if(millis() > lastVccMeasureTime + VCC_MEASURE_INTERVAL)
    {
        vccQueue.add(getVcc());
        lastVccMeasureTime = millis();

        // turn off if battery is low
        if(vccQueue.isFull() && vccQueue.average() + VCC2BAT_CORRECTION <= TURN_OFF_VOLTAGE){
            digitalWrite(MOSFET_PIN, LOW);
            ESP.deepSleep(0);
        }
    }


    if(millis() > lastPublishTime + MQTT_PUBLISH_INTERVAL)
    {
        publishState();
        lastPublishTime = millis();
    }

        
    // wait X milliseconds for subscription messages
    mqtt.processPackets(10);

    if(gRestart && millis() - gRestart > 1000){
        ESP.restart();
    }
}