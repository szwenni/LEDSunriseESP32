#include <AsyncMqttClient.h>
#include <FastLED.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <time.h>
#include "UserConfig.h"

class SimpleLEDStripControl {
    private:
        
        AsyncWebServer server;
        void setupWIFIConnect(bool connectOnly = false);
        void setupMQTT(bool connectOnly = false);
        void initHttp();
        void calculateIntervals();
        CRGB leds[NUM_LEDS];
        struct tm timeinfo;
        void syncLocalTime();
        void checkTimers();
        void configureNtp(uint16_t daylight);

        
    public:
        static SimpleLEDStripControl *self;
        //AsyncMqttClient mqttClient;
        
        SimpleLEDStripControl();
        UserConfig config;
        bool sundownEnabled;
        bool sunriseEnabled;            
        
        bool inited = false;
        bool reconecting = false;
        
        
        uint16_t sunriseLength = 1;
        uint16_t sundownLength = 1;
        uint16_t sunriseInterval = 1;
        uint16_t sundownInterval = 1;
        void setup();
        void loop();
        void sunrise();
        void sundown();
        void enableSunrise();
        void enableSundown();
        void setColor(uint8_t r, uint8_t g, uint8_t b);
        void setBrightness(uint8_t br);
        void brightnessUp();
        void brightnessDown();
        
};