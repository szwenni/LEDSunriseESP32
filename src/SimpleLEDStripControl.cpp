#include <Arduino.h>
#include <WIFI.h>
#include <ArduinoJson.h>
#include <AsyncJson.h>
#include "SimpleLEDStripControl.h"
#include <esp_int_wdt.h>
#include <esp_task_wdt.h>
#include <Update.h>
#include <AsyncMQTTClientWrapper.h>

DEFINE_GRADIENT_PALETTE(definedPalette){
    0, 0, 0, 0,
    84, 255, 77, 0,
    170, 255, 128, 0,
    255, 255, 145, 0};

SimpleLEDStripControl *SimpleLEDStripControl::self = NULL;

SimpleLEDStripControl::SimpleLEDStripControl() : server(80),
                                                 config(),
                                                 sundownEnabled(false),
                                                 sunriseEnabled(false),
                                                 restartRequired(false),
                                                 uploadFileInited(false),
                                                 uploadFileName(""),
                                                 uploadFIleFailed(false),
                                                 mqttEnabled(false)

{
    SimpleLEDStripControl::self = this;
    sunrisePalette = definedPalette;
}

void SimpleLEDStripControl::setup()
{
    Serial.println("Version 1.0");
    config.setup();
    File logoFile = SPIFFS.open("/logo.png", "r");
    logoSize = logoFile.size();
    logo = new char[logoSize];
    logoFile.readBytes(logo, logoSize);
    logoFile.close();
    FastLED.addLeds<P9813, DATA_PIN, CLOCK_PIN, COLOR_SEQUENCE>(leds, NUM_LEDS);
    if (BLACK_ON_START)
    {
        aktColor = new CRGB(0, 0, 0);
    }
    else
    {
        uint8_t lastR = config.getU8Property("LAST_COLOR_R");
        uint8_t lastG = config.getU8Property("LAST_COLOR_G");
        uint8_t lastB = config.getU8Property("LAST_COLOR_B");
        aktColor = new CRGB(lastR, lastG, lastB);
    }
    applyColor();
    FastLED.show();

    calculateIntervals();
}

void sunriseMqttChanged() {
    if(SimpleLEDStripControl::self->sunriseProp->getValueAsBool()) {
        if(!SimpleLEDStripControl::self->sunriseEnabled && !SimpleLEDStripControl::self->sundownEnabled) {
            SimpleLEDStripControl::self->enableSunrise(false);
        }
    } else {
        if(SimpleLEDStripControl::self->sunriseEnabled) {
            SimpleLEDStripControl::self->sunriseEnabled = false;
        }
    }
}

void sundownMqttChanged() {
    if(SimpleLEDStripControl::self->sundownProp->getValueAsBool()) {
        if(!SimpleLEDStripControl::self->sunriseEnabled && !SimpleLEDStripControl::self->sundownEnabled) {
            SimpleLEDStripControl::self->enableSundown(false);
        }
    } else {
        if(SimpleLEDStripControl::self->sundownEnabled) {
            SimpleLEDStripControl::self->sundownEnabled = false;
        }
    }
}

void colorMqttChanged() {
    if(!SimpleLEDStripControl::self->sunriseEnabled && !SimpleLEDStripControl::self->sundownEnabled) {
            SimpleLEDStripControl::self->setColor(SimpleLEDStripControl::self->colorProp->getR_HValue(), SimpleLEDStripControl::self->colorProp->getG_SValue(), SimpleLEDStripControl::self->colorProp->getB_VValue(), false);
    } else {
        uint8_t r = 0;
        uint8_t g = 0;
        uint8_t b = 0;
        SimpleLEDStripControl::self->getColor(&r, &g, &b);
        SimpleLEDStripControl::self->colorProp->setRGB_HSVValue(r, g, b);
    }
}

void onBrightnessChanged() {
    SimpleLEDStripControl::self->setBrightness((uint8_t)SimpleLEDStripControl::self->brightProp->getValueAsIntMax());
}

void SimpleLEDStripControl::setupHomie() {
    if(config.getBoolProperty("MQTT", "MQTT_ENABLED")) {
        Serial.println("Here 1");
        String address = config.getStringProperty("MQTT", "MQTT_ADDRESS");
        uint16_t port = config.getU16Property("MQTT", "MQTT_PORT");
        String username = config.getStringProperty("MQTT", "MQTT_USERNAME");
        String password = config.getStringProperty("MQTT", "MQTT_PASSWORD");
        String deviceid = config.getStringProperty("MQTT", "MQTT_DEVICEID");
        String devicename = config.getStringProperty("MQTT", "MQTT_DEVICENAME");
        String nodeName = config.getStringProperty("MQTT", "MQTT_NODENAME");
        String sunuppropname = config.getStringProperty("MQTT", "MQTT_SUNUPPROPNAME");
        String sundownpropname = config.getStringProperty("MQTT", "MQTT_SUNDOWNPROPNAME");
        String colorname = config.getStringProperty("MQTT", "MQTT_COLORPROPNAME");
        String brightname = config.getStringProperty("MQTT", "MQTT_BRIGHTPROPNAME");
        Serial.println("Here 2");
        homie = new Homie(address, port, username, password, devicename, deviceid, 1);
        lightNode = new HomieNode(homie, "light", nodeName, "sunupdownlight", 4);
        sunriseProp = new HomieNodeBooleanProperty("sunrise", sunuppropname, true, true, sunriseMqttChanged);
        sundownProp = new HomieNodeBooleanProperty("sundown", sundownpropname, true, true, sundownMqttChanged);
        colorProp = new HomieNodeColorProperty("color", colorname, true, true, false, colorMqttChanged);
        brightProp = new HomieNodeIntegerProperty("brightness2", brightname, "0:255", "%", true, true, onBrightnessChanged);
        Serial.println("Here 3");
        lightNode->addProperty(sunriseProp);
        lightNode->addProperty(sundownProp);
        lightNode->addProperty(colorProp);
        lightNode->addProperty(brightProp);
        Serial.println("Here 4");
        homie->addNode(lightNode);
        Serial.println("Here 5");
        homie->init<AsyncMQTTClientWrapper>();
        mqttEnabled = true;
    }
}

void SimpleLEDStripControl::loop()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        setupWIFIConnect(true);
    }
    EVERY_N_SECONDS(1)
    {
        syncLocalTime();
    }
    EVERY_N_SECONDS(10)
    {
        checkTimers();
    }
    if (sunriseEnabled)
    {
        sunrise();
    }
    if (sundownEnabled)
    {
        sundown();
    }
    FastLED.show();
    FastLED.delay(1);
    if (restartRequired)
    {
        yield();
        delay(1000);
        yield();
#if defined(ESP8266)
        ESP.restart();
#elif defined(ESP32)
        esp_task_wdt_init(1, true);
        esp_task_wdt_add(NULL);
        while (true)
            ;
#endif
    }
}

void SimpleLEDStripControl::calculateIntervals()
{
    sunriseLength = config.getU16Property("SunTimerConfig", "SUNRISE_LENGTH");
    sundownLength = config.getU16Property("SunTimerConfig", "SUNDOWN_LENGTH");
    sunriseInterval = ((float)(sunriseLength * 60) / (COLOR_PALETTE_END + 1)) * 1000;
    sundownInterval = ((float)(sundownLength * 60) / (COLOR_PALETTE_END + 1)) * 1000;
    Serial.printf("New sunrise interval: %d (%d)\n", sunriseLength, sunriseInterval);
    Serial.printf("New sunrise interval: %d (%d)\n", sundownLength, sundownInterval);
}

void SimpleLEDStripControl::setupWIFIConnect(bool connectOnly)
{
    Serial.println("[LWS] setting up WiFi Connect");
    WiFi.disconnect();
    server.reset();
    String wifiName = config.getStringProperty("WIFI_NAME");
    WiFi.begin(wifiName.c_str(), config.getStringProperty("WIFI_PASSWORD").c_str());
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.printf("Connecting to %s\n", wifiName.c_str());
    }
    Serial.println("Connected");
    initHttp();
    if (!inited)
    {
        configureNtp(0);
        inited = true;
        setupHomie();
    }

    //setupMQTT(connectOnly);
    //handle mqtt broker here
}

void SimpleLEDStripControl::configureNtp(uint16_t daylight)
{
    configTime(config.getU16Property("TIMEZONE_OFFSET"), daylight, config.getStringProperty("NTP_SERVER").c_str());
}

void SimpleLEDStripControl::sunrise()
{
    EVERY_N_MILLISECONDS(sunriseInterval)
    {
        Serial.println("Sunrising...");
        if (sunStep < COLOR_PALETTE_END)
        {
            sunStep++;
            CRGB color = ColorFromPalette(sunrisePalette, sunStep);
            aktColor = &color;
            applyColor(false);
            Serial.printf("R: %d, G: %d, B: %d\n", aktColor->r, aktColor->g, aktColor->b);
        }
        if (sunStep == COLOR_PALETTE_END)
        {
            sunriseEnabled = false;
            sunriseProp->setBoolValue(false);
        }
    }
}

void SimpleLEDStripControl::sundown()
{
    EVERY_N_MILLISECONDS(sundownInterval)
    {
        Serial.println("Sundowning...");
        if (sunStep > 0)
        {
            sunStep--;
            CRGB color = ColorFromPalette(sunrisePalette, sunStep);
            aktColor = &color;
            applyColor(false);
            Serial.printf("R: %d, G: %d, B: %d\n", aktColor->r, aktColor->g, aktColor->b);
        }
        if (sunStep == 0)
        {
            sundownEnabled = false;
            sundownProp->setBoolValue(false);
        }
    }
}

void SimpleLEDStripControl::enableSundown(bool publishMqtt)
{
    Serial.printf("Sunrising with Interval %d\n", sundownInterval);
    sunriseEnabled = false;
    sundownEnabled = true;
    sunStep = COLOR_PALETTE_END;
    CRGB color = ColorFromPalette(sunrisePalette, sunStep);
    fill_solid(leds, NUM_LEDS, color);
    if(publishMqtt && mqttEnabled) {
        sundownProp->setBoolValue(true);
    }
}

void SimpleLEDStripControl::enableSunrise(bool publishMqtt)
{
    Serial.printf("Sunrising with Interval %d\n", sunriseInterval);
    sunriseEnabled = true;
    sundownEnabled = false;
    sunStep = 0;
    CRGB color = ColorFromPalette(sunrisePalette, sunStep);
    fill_solid(leds, NUM_LEDS, color);
    if(publishMqtt && mqttEnabled) {
        sunriseProp->setBoolValue(true);
    }
}

void SimpleLEDStripControl::setColor(uint8_t r, uint8_t g, uint8_t b, bool publishToMqtt)
{
    
    aktColor->setRGB(r, g, b);
    applyColor();
    if(publishToMqtt && mqttEnabled) {
        colorProp->setRGB_HSVValue(r, g, b);
    }
}

void SimpleLEDStripControl::applyColor(bool save)
{
    fill_solid(leds, NUM_LEDS, *aktColor);

    if (save)
    {
        config.setU8Property("LAST_COLOR_R", aktColor->r);
        config.setU8Property("LAST_COLOR_G", aktColor->g);
        config.setU8Property("LAST_COLOR_B", aktColor->b);
        config.save();
    }
}

void SimpleLEDStripControl::setBrightness(uint8_t br)
{
    FastLED.setBrightness(br);
}

void SimpleLEDStripControl::brightnessDown()
{
    uint8_t br = FastLED.getBrightness();
    if (br - 10 < 0)
    {
        br = 0;
    }
    else
    {
        br -= 10;
    }
    FastLED.setBrightness(br);
}

void SimpleLEDStripControl::brightnessUp()
{
    uint8_t br = FastLED.getBrightness();
    if (br + 10 > 255)
    {
        br = 255;
    }
    else
    {
        br += 10;
    }
    FastLED.setBrightness(br);
}

void SimpleLEDStripControl::initHttp()
{
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(SPIFFS, "/index.html", "text/html");
    });

    server.on("/update", HTTP_GET, [](AsyncWebServerRequest *request) {
        File indexFile = SPIFFS.open("/update.html", "r");
        String content = indexFile.readString();
        content.replace("#updateFileTempl", "<li class=\"nav-item\"><a class=\"nav-link\" href=\"/updateFile\">Update File</a></li>");
        content.replace("#updateOTATempl", "<li class=\"nav-item active\"><a class=\"nav-link\" href=\"/update\">Update OTA<span class=\"sr-only\">(current)</span></a></li>");
        content.replace("#navbarTitleTempl", "OTA Update");
        request->send(200, "text/html", content);
        indexFile.close();
    });

    //curl --request POST -F file="@firmware.bin" http://192.168.2..../update
    server.on("/update", HTTP_POST, [&](AsyncWebServerRequest *request) {
        // the request handler is triggered after the upload has finished...
        // create the response, add header, and send response
        if(Update.hasError()) {
            AsyncWebServerResponse *response = request->beginResponse((Update.hasError()) ? 500 : 200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
            response->addHeader("Connection", "close");
            response->addHeader("Access-Control-Allow-Origin", "*");
            request->send(response);
        } else {
            request->redirect("/");
        }
        
        SimpleLEDStripControl::self->restartRequired = true; }, [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
        //Upload handler chunks in data
        
        if (!index)
        {

                if (!Update.begin(UPDATE_SIZE_UNKNOWN))
                { // Start with max available size
                    Update.printError(Serial);
                }

                /*#if defined(ESP8266)
                        Update.runAsync(true); // Tell the updaterClass to run in async mode
                    #endif*/
            }

            // Write chunked data to the free sketch space
            if (Update.write(data, len) != len)
            {
                Update.printError(Serial);
            }

            if (final)
            { // if the final flag is set then this is the last frame of data
                if (Update.end(true))
                { //true to set the size to the current progress
                }
            } });

    server.on("/updateFile", HTTP_GET, [](AsyncWebServerRequest *request) {
        File indexFile = SPIFFS.open("/update.html", "r");
        String content = indexFile.readString();
        content.replace("'/update'", "'/updateFile'");
        content.replace("#updateFileTempl", "<li class=\"nav-item active\"><a class=\"nav-link\" href=\"/updateFile\">Update File<span class=\"sr-only\">(current)</span></a></li>");
        content.replace("#updateOTATempl", "<li class=\"nav-item\"><a class=\"nav-link\" href=\"/update\">Update OTA</a></li>");
        content.replace("#navbarTitleTempl", "Update File");
        request->send(200, "text/html", content);
        indexFile.close();
    });

    server.on("/mqtt", HTTP_GET, [](AsyncWebServerRequest *request) {
        File indexFile = SPIFFS.open("/configMqtt.html", "r");
        String content = indexFile.readString();
        
        request->send(200, "text/html", content);
        indexFile.close();
    });

    
    //curl --request POST -F file="@file" http://192.168.2..../updateFile
    server.on("/updateFile", HTTP_POST, [&](AsyncWebServerRequest *request) {
        // the request handler is triggered after the upload has finished...
        // create the response, add header, and send response
        if(SimpleLEDStripControl::self->uploadFIleFailed) {
            AsyncWebServerResponse *response = request->beginResponse((SimpleLEDStripControl::self->uploadFIleFailed) ? 500 : 200, "text/plain", (SimpleLEDStripControl::self->uploadFIleFailed) ? "FAIL" : "OK");
            response->addHeader("Connection", "close");
            response->addHeader("Access-Control-Allow-Origin", "*");
            request->send(response);
        } else {
            request->redirect("updateFile");
        }
        SimpleLEDStripControl::self->uploadFIleFailed = false; }, [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
        //Upload handler chunks in data
        if(!SimpleLEDStripControl::self->uploadFileInited || SimpleLEDStripControl::self->uploadFileName.equals("/"+filename))
        {
            
            if (!index)
            {
                SimpleLEDStripControl::self->uploadFileInited = true;
                SimpleLEDStripControl::self->uploadFileName = "/"+filename;
                if(SPIFFS.exists(SimpleLEDStripControl::self->uploadFileName)) {
                    SPIFFS.rename(SimpleLEDStripControl::self->uploadFileName, SimpleLEDStripControl::self->uploadFileName+".old");
                }
                SimpleLEDStripControl::self->uploadfile = SPIFFS.open(SimpleLEDStripControl::self->uploadFileName, "w");
            }
            // Write chunked data to the free sketch space

            if (SimpleLEDStripControl::self->uploadfile.write(data, len) != len)
            {
                SimpleLEDStripControl::self->uploadFIleFailed = true;
            }
            

            if (final)
            { // if the final flag is set then this is the last frame of data
                SimpleLEDStripControl::self->uploadfile.close();
                SimpleLEDStripControl::self->uploadFileInited = false;
                if(SimpleLEDStripControl::self->uploadFIleFailed) {
                    SPIFFS.remove(SimpleLEDStripControl::self->uploadFileName);
                    SPIFFS.rename(SimpleLEDStripControl::self->uploadFileName+".old", SimpleLEDStripControl::self->uploadFileName);
                } else {
                    SPIFFS.remove(SimpleLEDStripControl::self->uploadFileName+".old");
                }
                
            }
        } });

    server.on("/logo.png", HTTP_GET, [](AsyncWebServerRequest *request) {
        AsyncResponseStream *response = request->beginResponseStream("image/png");
        response->write(SimpleLEDStripControl::self->logo, SimpleLEDStripControl::self->logoSize);
        request->send(response);
    });

    server.on("/rest/config", HTTP_GET, [](AsyncWebServerRequest *request) {
        AsyncResponseStream *response = request->beginResponseStream("application/json");
        serializeJson(SimpleLEDStripControl::self->config.json["SunTimerConfig"], *response);
        response->addHeader("Access-Control-Allow-Origin", "*");
        request->send(response);
    });

    server.on("/rest/mqttConfig", HTTP_GET, [](AsyncWebServerRequest *request) {
        AsyncResponseStream *response = request->beginResponseStream("application/json");
        
        serializeJson(SimpleLEDStripControl::self->config.json["MQTT"], *response);
        response->addHeader("Access-Control-Allow-Origin", "*");
        request->send(response);
    });

    server.on("/rest/status", HTTP_GET, [](AsyncWebServerRequest *request) {
        AsyncResponseStream *response = request->beginResponseStream("application/json");
        StaticJsonDocument<1024> statusBuffer;
        time_t now;
        time(&now);
        statusBuffer["sunrise"] = SimpleLEDStripControl::self->sunriseEnabled;
        statusBuffer["sundown"] = SimpleLEDStripControl::self->sundownEnabled;
        statusBuffer["time"] = now;
        response->addHeader("Access-Control-Allow-Origin", "*");
        serializeJson(statusBuffer, *response);
        request->send(response);
    });

    server.on("/rest/color", HTTP_GET, [](AsyncWebServerRequest *request) {
        AsyncResponseStream *response = request->beginResponseStream("application/json");
        StaticJsonDocument<100> colorJson;
        uint8_t *r = new uint8_t();
        uint8_t *g = new uint8_t();
        uint8_t *b = new uint8_t();
        SimpleLEDStripControl::self->getColor(r, g, b);
        colorJson["r"] = *r;
        colorJson["g"] = *g;
        colorJson["b"] = *b;
        serializeJson(colorJson, *response);
        delete r;
        delete g;
        delete b;
        response->addHeader("Access-Control-Allow-Origin", "*");
        request->send(response);
    });

    server.on("/rest/config", HTTP_OPTIONS, [](AsyncWebServerRequest *request) {
        AsyncResponseStream *response = request->beginResponseStream("text/plain");
        response->addHeader("Access-Control-Allow-Origin", "*");
        response->addHeader("Access-Control-Allow-Methods", "POST, GET, PUT, UPDATE, DELETE, OPTIONS");
        response->addHeader("Access-Control-Max-Age", "1000");
        response->addHeader("Access-Control-Allow-Headers", "*");
        response->setCode(200);
        request->send(response);
    });

    server.on("/rest/mqttConfig", HTTP_OPTIONS, [](AsyncWebServerRequest *request) {
        AsyncResponseStream *response = request->beginResponseStream("text/plain");
        response->addHeader("Access-Control-Allow-Origin", "*");
        response->addHeader("Access-Control-Allow-Methods", "POST, GET, PUT, UPDATE, DELETE, OPTIONS");
        response->addHeader("Access-Control-Max-Age", "1000");
        response->addHeader("Access-Control-Allow-Headers", "*");
        response->setCode(200);
        request->send(response);
    });

    server.on("/rest/color", HTTP_OPTIONS, [](AsyncWebServerRequest *request) {
        AsyncResponseStream *response = request->beginResponseStream("text/plain");
        response->addHeader("Access-Control-Allow-Origin", "*");
        response->addHeader("Access-Control-Allow-Methods", "POST, GET, PUT, UPDATE, DELETE, OPTIONS");
        response->addHeader("Access-Control-Max-Age", "1000");
        response->addHeader("Access-Control-Allow-Headers", "*");
        response->setCode(200);
        request->send(response);
    });

    server.on("/rest/brightUp", HTTP_POST, [](AsyncWebServerRequest *request) {
        SimpleLEDStripControl::self->brightnessUp();
        request->send(200, "text/plain", "");
    });

    server.on("/rest/brightDown", HTTP_POST, [](AsyncWebServerRequest *request) {
        SimpleLEDStripControl::self->brightnessDown();
        request->send(200, "text/plain", "");
    });

    server.on("/rest/stopSundown", HTTP_POST, [](AsyncWebServerRequest *request) {
        SimpleLEDStripControl::self->sundownEnabled = false;
        SimpleLEDStripControl::self->sundownProp->setBoolValue(false);
        request->send(200, "text/plain", "");
    });

    server.on("/rest/stopSunup", HTTP_POST, [](AsyncWebServerRequest *request) {
        SimpleLEDStripControl::self->sunriseEnabled = false;
        SimpleLEDStripControl::self->sunriseProp->setBoolValue(false);
        request->send(200, "text/plain", "");
    });

    AsyncCallbackJsonWebHandler *configHandler = new AsyncCallbackJsonWebHandler("/rest/config", [](AsyncWebServerRequest *request, JsonVariant json) {
        Serial.println("having new config");
        JsonObject jsonObj = json.as<JsonObject>();
        SimpleLEDStripControl::self->config.checkAndNewConfig(jsonObj);
        SimpleLEDStripControl::self->calculateIntervals();
        AsyncResponseStream *response = request->beginResponseStream("application/json");
        serializeJson(SimpleLEDStripControl::self->config.json, *response);
        response->addHeader("Access-Control-Allow-Origin", "*");
        request->send(response);
    });


    server.addHandler(configHandler);

    AsyncCallbackJsonWebHandler *mqttConfigHandler = new AsyncCallbackJsonWebHandler("/rest/mqttConfig", [](AsyncWebServerRequest *request, JsonVariant json) {
        Serial.println("having new mqtt config");
        JsonObject jsonObj = json.as<JsonObject>();
        SimpleLEDStripControl::self->config.checkMQTTConfig(jsonObj);
        AsyncResponseStream *response = request->beginResponseStream("application/json");
        serializeJson(SimpleLEDStripControl::self->config.json, *response);
        response->addHeader("Access-Control-Allow-Origin", "*");
        request->send(response);
        SimpleLEDStripControl::self->restartRequired = true;
    });

    
    server.addHandler(mqttConfigHandler);

    AsyncCallbackJsonWebHandler *colorHandler = new AsyncCallbackJsonWebHandler("/rest/color", [](AsyncWebServerRequest *request, JsonVariant json) {
        JsonObject jsonObj = json.as<JsonObject>();
        uint8_t r = jsonObj["r"].as<uint8_t>();
        uint8_t g = jsonObj["g"].as<uint8_t>();
        uint8_t b = jsonObj["b"].as<uint8_t>();
        Serial.printf("having r: %d, g: %d, b: %d\n", r, g, b);
        SimpleLEDStripControl::self->setColor(r, g, b);
        AsyncResponseStream *response = request->beginResponseStream("application/json");
        StaticJsonDocument<100> colorJson;
        SimpleLEDStripControl::self->getColor(&r, &g, &b);
        colorJson["r"] = r;
        colorJson["g"] = g;
        colorJson["b"] = b;
        serializeJson(colorJson, *response);
        response->addHeader("Access-Control-Allow-Origin", "*");
        request->send(response);
    });

    server.addHandler(colorHandler);
    server.begin();
}

void SimpleLEDStripControl::syncLocalTime()
{
    if (!getLocalTime(&timeinfo))
    {
        Serial.println("Failed to obtain time");
        return;
    }
    uint8_t offset = 7 - (timeinfo.tm_wday);
    if (offset == 7)
    {
        offset = 0;
    }
    if (config.summerTime && ((offset == 0 && timeinfo.tm_hour >= 3 && timeinfo.tm_mon == 9 && timeinfo.tm_mday + offset > 31) ||
                              (offset != 0 && timeinfo.tm_mday + offset > 31 && timeinfo.tm_mon == 9) ||
                              timeinfo.tm_mon > 9 || timeinfo.tm_mon < 2))
    {
        Serial.println("Setting wintertime");
        config.summerTime = false;
        configureNtp(0);
        getLocalTime(&timeinfo);
    }
    else if (!config.summerTime && ((offset == 0 && timeinfo.tm_hour >= 2 && timeinfo.tm_mon == 2 && timeinfo.tm_mday + offset > 31) ||
                                    (offset != 0 && timeinfo.tm_mday + offset > 31 && timeinfo.tm_mon == 2) ||
                                    (timeinfo.tm_mon > 2 && timeinfo.tm_mon < 10)))
    {
        Serial.println("Setting summertime");
        config.summerTime = true;
        configureNtp(3600);
        getLocalTime(&timeinfo);
    }
}

void SimpleLEDStripControl::getColor(uint8_t *r, uint8_t *g, uint8_t *b)
{
    *r = aktColor->r;
    *g = aktColor->g;
    *b = aktColor->b;
}

void SimpleLEDStripControl::checkTimers()
{
    SunTimer *timer = config.getSunTimerForDay(timeinfo.tm_wday);
    //Serial.printf("got suntimer %s\n", timer->dayOfWeek.c_str());
    if (timer != nullptr && timer->sunTimer->enabled && sundownEnabled == false && sunriseEnabled == false)
    {
        uint8_t aktHour = timeinfo.tm_hour;
        uint8_t aktMinute = timeinfo.tm_min;
        Serial.printf("Checking timers (%d:%d)\n", aktHour, aktMinute);
        Serial.printf("Timer: Up %d:%d, Down: %d:%d\n", timer->sunTimer->hoursUp, timer->sunTimer->minutesUp, timer->sunTimer->hoursDown, timer->sunTimer->minutesDown);
        if (timer->sunTimer->hoursUp == timeinfo.tm_hour && timer->sunTimer->minutesUp == timeinfo.tm_min)
        {
            enableSunrise();
        }
        else if (timer->sunTimer->hoursDown == timeinfo.tm_hour && timer->sunTimer->minutesDown == timeinfo.tm_min)
        {
            enableSundown();
        }
    }
}
