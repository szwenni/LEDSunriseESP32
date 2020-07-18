#include <AsyncMqttClient.h>
#include <FastLED.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <time.h>
#include "UserConfig.h"
#include <Homie.h>
#include <HomieNode.h>
#include <HomieNodeColorProperty.h>
#include <HomieNodeBooleanProperty.h>
#include <HomieNodeIntegerProperty.h>


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
        Homie* homie;
        HomieNode* lightNode;
        
        uint8_t sunStep = 0;
        CRGB *aktColor;
        CRGBPalette16 sunrisePalette;
        bool mqttEnabled;

        
    public:
        static SimpleLEDStripControl *self;
        //AsyncMqttClient mqttClient;
        
        SimpleLEDStripControl();
        UserConfig config;
        bool sundownEnabled;
        bool sunriseEnabled;            
        
        bool inited = false;
        bool reconecting = false;
        
        HomieNodeColorProperty* colorProp;
        HomieNodeBooleanProperty* sunriseProp;
        HomieNodeBooleanProperty* sundownProp;
        HomieNodeIntegerProperty* brightProp;
        uint16_t sunriseLength = 1;
        uint16_t sundownLength = 1;
        uint16_t sunriseInterval = 1;
        uint16_t sundownInterval = 1;
        char *logo;
        size_t logoSize;
        void setup();
        void setupHomie();
        void loop();
        void sunrise();
        void sundown();
        void enableSunrise(bool publishMqtt = true);
        void enableSundown(bool publishMqtt = true);
        void setColor(uint8_t r, uint8_t g, uint8_t b, bool publishToMqtt = true);
        void getColor(uint8_t *r, uint8_t *g, uint8_t *b);
        void applyColor(bool save = true);
        void setBrightness(uint8_t br);
        void brightnessUp();
        void brightnessDown();
        bool restartRequired;
        bool uploadFileInited;
        String uploadFileName;
        File uploadfile;
        bool uploadFIleFailed;
        
};