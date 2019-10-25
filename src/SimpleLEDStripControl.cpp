#include <Arduino.h>
#include <WIFI.h>
#include <ArduinoJson.h>
#include <AsyncJson.h>
#include "SimpleLEDStripControl.h"


SimpleLEDStripControl *SimpleLEDStripControl::self = NULL;

SimpleLEDStripControl::SimpleLEDStripControl():
server(80),
config(),
sundownEnabled(false),
sunriseEnabled(false)

{
    SimpleLEDStripControl::self = this;

}

void SimpleLEDStripControl::setup() {
    config.setup();
    FastLED.addLeds<P9813, DATA_PIN, CLOCK_PIN, RGB>(leds, NUM_LEDS);
    
    fill_solid(leds, NUM_LEDS, CRGB::Black);
    
    calculateIntervals();
    

}

void SimpleLEDStripControl::loop() {
    if(WiFi.status() != WL_CONNECTED) {
        setupWIFIConnect(true);
    }
    EVERY_N_SECONDS(1) {
        syncLocalTime();
    }
    EVERY_N_SECONDS(10) {
        checkTimers();
    }
    if(sunriseEnabled) {
        sunrise();
    }
    if(sundownEnabled) {
        sundown();
    }
    FastLED.show();
    FastLED.delay(1);
    
}

void SimpleLEDStripControl::calculateIntervals() {
    sunriseLength = config.getU16Property("SUNRISE_LENGTH");
    sundownLength = config.getU16Property("SUNDOWN_LENGTH");
    sunriseInterval = ((float)(sunriseLength * 60) / 256)*1000;
    sundownInterval = ((float)(sundownLength * 60) / 256)*1000;
    Serial.printf("New sunrise interval: %d (%d)", sunriseLength, sunriseInterval);
    Serial.printf("New sunrise interval: %d (%d)", sundownLength, sundownInterval);
}

void SimpleLEDStripControl::setupWIFIConnect(bool connectOnly) {
    Serial.println("[LWS] setting up WiFi Connect");
    WiFi.disconnect();
    server.reset();
    String wifiName = config.getStringProperty("WIFI_NAME");
    WiFi.begin(wifiName.c_str(), config.getStringProperty("WIFI_PASSWORD").c_str());
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.printf("Connecting to %s\n", wifiName.c_str());
    }
    Serial.println("Connected");
    initHttp();
    if(!inited) {
        configureNtp(0);
        inited = true;
    }
    
    //setupMQTT(connectOnly);
    //handle mqtt broker here
}

void SimpleLEDStripControl::configureNtp(uint16_t daylight) {
    configTime(config.getU16Property("TIMEZONE_OFFSET"), daylight, config.getStringProperty("NTP_SERVER").c_str());
}

void SimpleLEDStripControl::sunrise() {
    EVERY_N_MILLISECONDS(sunriseInterval) {
        Serial.println("Sunrising...");
        for(uint8_t i = 0;i<NUM_LEDS; i++) {
            if(leds[i].red < 255) {
                leds[i] = leds[i].setRGB(leds[i].r+1, leds[i].g+1, leds[i].b+1);
            }
            if(leds[i].red == 255) {
                sunriseEnabled = false;
            }
            Serial.printf("R: %d, G: %d, B: %d\n", leds[i].r, leds[i].g, leds[i].b);
        }
        
    }
}

void SimpleLEDStripControl::sundown() {
    EVERY_N_MILLISECONDS(sundownInterval) {
        Serial.println("Sundowning...");
        for(uint8_t i = 0;i<NUM_LEDS; i++) {
            if(leds[i].red > 0) {
                leds[i] = leds[i].setRGB(leds[i].r-1, leds[i].g-1, leds[i].b-1);
            }
            if(leds[i].red == 0) {
                leds[i] = leds[i].setRGB(0, 0, 0);
                sundownEnabled = false;
            }
            Serial.printf("R: %d, G: %d, B: %d\n", leds[i].r, leds[i].g, leds[i].b);
        }
    }
}

void SimpleLEDStripControl::enableSundown() {
    Serial.printf("Sunrising with Interval %d\n", sundownInterval);
    sunriseEnabled = false;
    sundownEnabled = true;
    for(int i=0;i<NUM_LEDS;i++) {
            leds[i].setRGB(255, 255, 255);
    }
}


void SimpleLEDStripControl::enableSunrise() {
    Serial.printf("Sunrising with Interval %d\n", sunriseInterval);
    sunriseEnabled = true;
    sundownEnabled = false;
    for(int i=0;i<NUM_LEDS;i++) {
            leds[i].setRGB(0, 0, 0);
    }
}

void SimpleLEDStripControl::setColor(uint8_t r, uint8_t g , uint8_t b) {
    for(int i=0;i<NUM_LEDS;i++) {
            leds[i].setRGB(r, g, b);
    }
}

void SimpleLEDStripControl::setBrightness(uint8_t br) {
    FastLED.setBrightness(br);
}

void SimpleLEDStripControl::brightnessDown() {
    uint8_t br = FastLED.getBrightness();
    if(br - 10 < 0) {
        br = 0;
    } else {
        br -= 10;
    }
    FastLED.setBrightness(br);
}

void SimpleLEDStripControl::brightnessUp() {
    uint8_t br = FastLED.getBrightness();
    if(br + 10 > 255) {
        br = 255;
    } else {
        br += 10;
    }
    FastLED.setBrightness(br);
}

void SimpleLEDStripControl::initHttp() {
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        File indexFile = SPIFFS.open("/index.html", "r");
        String content = indexFile.readString();
        request->send(200, "text/html", content);
        indexFile.close();
    });

    server.on("/rest/config", HTTP_GET, [](AsyncWebServerRequest *request){
        AsyncResponseStream *response = request->beginResponseStream("application/json");
        serializeJson(SimpleLEDStripControl::self->config.json, *response);
        response->addHeader("Access-Control-Allow-Origin", "*");
        request->send(response);
    });

    server.on("/rest/config", HTTP_OPTIONS, [](AsyncWebServerRequest *request){
        Serial.println("Having option");
        AsyncResponseStream *response = request->beginResponseStream("text/plain");
        response->addHeader("Access-Control-Allow-Origin", "*");
        response->addHeader("Access-Control-Allow-Methods", "POST, GET, PUT, UPDATE, DELETE, OPTIONS");
        response->addHeader("Access-Control-Max-Age", "1000");
        response->addHeader("Access-Control-Allow-Headers", "*");
        response->setCode(200); 
        request->send(response);
    });

    server.on("/rest/color", HTTP_OPTIONS, [](AsyncWebServerRequest *request){
        Serial.println("Having option");
        AsyncResponseStream *response = request->beginResponseStream("text/plain");
        response->addHeader("Access-Control-Allow-Origin", "*");
        response->addHeader("Access-Control-Allow-Methods", "POST, GET, PUT, UPDATE, DELETE, OPTIONS");
        response->addHeader("Access-Control-Max-Age", "1000");
        response->addHeader("Access-Control-Allow-Headers", "*");
        response->setCode(200); 
        request->send(response);
    });

    server.on("/rest/brightUp", HTTP_POST, [](AsyncWebServerRequest *request){
        SimpleLEDStripControl::self->brightnessUp();
        request->send(200, "text/plain", "");
    });

    server.on("/rest/brightDown", HTTP_POST, [](AsyncWebServerRequest *request){
        SimpleLEDStripControl::self->brightnessDown();
        request->send(200, "text/plain", "");
    });

    AsyncCallbackJsonWebHandler* configHandler = new AsyncCallbackJsonWebHandler("/rest/config", [](AsyncWebServerRequest *request, JsonVariant json) {
        
        Serial.println("having new config");
        JsonObject jsonObj = json.as<JsonObject>();
        SimpleLEDStripControl::self->config.checkAndNewConfig(jsonObj);
        SimpleLEDStripControl::self->calculateIntervals();
        AsyncResponseStream *response = request->beginResponseStream("application/json");
        serializeJsonPretty(SimpleLEDStripControl::self->config.json, *response);
        response->addHeader("Access-Control-Allow-Origin", "*");
        request->send(response);
    });
    server.addHandler(configHandler);

    AsyncCallbackJsonWebHandler* colorHandler = new AsyncCallbackJsonWebHandler("/rest/color", [](AsyncWebServerRequest *request, JsonVariant json) {
        JsonObject jsonObj = json.as<JsonObject>();
        uint8_t r = jsonObj["r"].as<uint8_t>();
        uint8_t g = jsonObj["g"].as<uint8_t>();
        uint8_t b = jsonObj["b"].as<uint8_t>();
        Serial.printf("having r: %d, g: %d, b: %d\n", r, g, b);
        SimpleLEDStripControl::self->setColor(r, g, b);
        AsyncResponseStream *response = request->beginResponseStream("application/json");
        response->addHeader("Access-Control-Allow-Origin", "*");
        request->send(response);
    });

    
    server.addHandler(colorHandler);
    server.begin();
}

void SimpleLEDStripControl::syncLocalTime()
{
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  uint8_t offset = 7 - (timeinfo.tm_wday);
      if(offset == 7) {
          offset = 0;
      }
  if(!config.summerTime && ((offset == 0 && timeinfo.tm_hour >= 3 && timeinfo.tm_mon == 9 && timeinfo.tm_mday + offset > 31) ||
   (offset != 0 && timeinfo.tm_mday + offset > 31 && timeinfo.tm_mon == 9) ||
    timeinfo.tm_mon > 9)) {
      Serial.println("Setting wintertime");
      config.summerTime = false;
      configureNtp(0);
      getLocalTime(&timeinfo);
  } else if (!config.summerTime && ((offset == 0 && timeinfo.tm_hour >= 2 && timeinfo.tm_mon == 2 && timeinfo.tm_mday + offset > 31) ||
   (offset != 0 && timeinfo.tm_mday + offset > 31 && timeinfo.tm_mon == 2) ||
    timeinfo.tm_mon > 2)) {
      Serial.println("Setting summertime");
      config.summerTime = true;
      configureNtp(3600);
      getLocalTime(&timeinfo);
  }
}

void SimpleLEDStripControl::checkTimers() {
    SunTimer* timer = config.getSunTimerForDay(timeinfo.tm_wday);
    //Serial.printf("got suntimer %s\n", timer->dayOfWeek.c_str());
    if(timer != nullptr && timer->sunTimer->enabled && sundownEnabled == false && sunriseEnabled == false) {
        uint8_t aktHour = timeinfo.tm_hour;
        uint8_t aktMinute = timeinfo.tm_min;
        Serial.printf("Checking timers (%d:%d)\n", aktHour, aktMinute);
        Serial.printf("Timer: Up %d:%d, Down: %d:%d\n", timer->sunTimer->hoursUp, timer->sunTimer->minutesUp, timer->sunTimer->hoursDown, timer->sunTimer->minutesDown);
        if(timer->sunTimer->hoursUp == timeinfo.tm_hour && timer->sunTimer->minutesUp == timeinfo.tm_min) {
            enableSunrise();
        } else if(timer->sunTimer->hoursDown == timeinfo.tm_hour && timer->sunTimer->minutesDown == timeinfo.tm_min) {
            enableSundown();
        }

    }
}
