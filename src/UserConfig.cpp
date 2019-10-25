
#include <FS.h>
#include <SPIFFS.h>
#include "UserConfig.h"

UserConfig::UserConfig()
{
    SPIFFS.begin(true);
}

void UserConfig::setup() {
    if(!SPIFFS.exists("/config.json")) {
        Serial.println("Config is not available, generating default");
        generateDefault();
        save();

    } else {
        Serial.println("Config is available.");
        load();
    }
}

void UserConfig::save() {
    File configFile = SPIFFS.open("/config.json", "w");
    serializeJson(json, configFile);
    configFile.close();
}

void UserConfig::load() {
    File configFile = SPIFFS.open("/config.json", "r");
    String input = configFile.readString();
    deserializeJson(json, input);
    check();
    serializeJsonPretty(json, Serial);
    configFile.close();
    loadSunTimers();
}

void UserConfig::loadSunTimers(){
    suntimers[0] = new SunTimer(json.getMember("SunTimers").getMember(MONDAY).as<JsonObject>());
    suntimers[1] = new SunTimer(json.getMember("SunTimers").getMember(TUESDAY).as<JsonObject>());
    suntimers[2] = new SunTimer(json.getMember("SunTimers").getMember(WEDNESDAY).as<JsonObject>());
    suntimers[3] = new SunTimer(json.getMember("SunTimers").getMember(THURSDAY).as<JsonObject>());
    suntimers[4] = new SunTimer(json.getMember("SunTimers").getMember(FRIDAY).as<JsonObject>());
    suntimers[5] = new SunTimer(json.getMember("SunTimers").getMember(SATURDAY).as<JsonObject>());
    suntimers[6] = new SunTimer(json.getMember("SunTimers").getMember(SUNDAY).as<JsonObject>());
}

void UserConfig::check() {
    if(json.getMember("SunTimers").isNull()) {
        SunTimer *monday = new SunTimer(MONDAY, 0, 0, 0, 0, false);
        SunTimer *tuesday = new SunTimer(TUESDAY, 0, 0, 0, 0, false);
        SunTimer *wednesday = new SunTimer(WEDNESDAY, 0, 0, 0, 0, false);
        SunTimer *thursday = new SunTimer(THURSDAY, 0, 0, 0, 0, false);
        SunTimer *friday = new SunTimer(FRIDAY, 0, 0, 0, 0, false);
        SunTimer *saturday = new SunTimer(SATURDAY, 0, 0, 0, 0, false);
        SunTimer *sunday = new SunTimer(SUNDAY, 0, 0, 0, 0, false);
        JsonObject timers = json.createNestedObject("SunTimers");
        
        monday->toJson(timers.createNestedObject(monday->dayOfWeek));
        tuesday->toJson(timers.createNestedObject(tuesday->dayOfWeek));
        wednesday->toJson(timers.createNestedObject(wednesday->dayOfWeek));
        thursday->toJson(timers.createNestedObject(thursday->dayOfWeek));
        friday->toJson(timers.createNestedObject(friday->dayOfWeek));
        saturday->toJson(timers.createNestedObject(saturday->dayOfWeek));
        sunday->toJson(timers.createNestedObject(sunday->dayOfWeek));
        save();
        delete monday;
        delete tuesday;
        delete wednesday;
        delete thursday;
        delete friday;
        delete saturday;
        delete sunday;

    }
    if(json.getMember("NTP_SERVER").isNull()) {
        json["NTP_SERVER"] = NTP_SERVER;
        json["TIMEZONE_OFFSET"] = TIMEZONE_OFFSET;
    }
}

void UserConfig::generateDefault() {
    json["WIFI_NAME"] = WIFI_NAME;
    json["WIFI_PASSWORD"] = WIFI_PASSWORD;
    json["SUNDOWN_LENGTH"] = SUNDOWN_LENGTH;
    json["SUNRISE_LENGTH"] = SUNRISE_LENGTH;
    check();
    loadSunTimers();
}

void UserConfig::setStringProperty(String name, String value) {
    json[name] = value;
}

String UserConfig::getStringProperty(String name) {
    return json.getMember(name).as<String>();
}

void UserConfig::setU8Property(String name, uint8_t value) {
    json[name] = value;
}

uint8_t UserConfig::getU8Property(String name) {
    return json.getMember(name).as<uint8_t>();
}

void UserConfig::setU16Property(String name, uint16_t value) {
    json[name] = value;
}

uint16_t UserConfig::getU16Property(String name) {
    return json.getMember(name).as<uint16_t>();
}

void UserConfig::checkAndNewConfig(JsonObject object) {
    if(!object["SunTimers"].isNull()) {
        checkAddSingleTimer(object["SunTimers"][MONDAY], MONDAY);
        checkAddSingleTimer(object["SunTimers"][TUESDAY], TUESDAY);
        checkAddSingleTimer(object["SunTimers"][WEDNESDAY], WEDNESDAY);
        checkAddSingleTimer(object["SunTimers"][THURSDAY], THURSDAY);
        checkAddSingleTimer(object["SunTimers"][FRIDAY], FRIDAY);
        checkAddSingleTimer(object["SunTimers"][SATURDAY], SATURDAY);
        checkAddSingleTimer(object["SunTimers"][SUNDAY], SUNDAY);
        
        delete suntimers[0];
        delete suntimers[1];
        delete suntimers[2];
        delete suntimers[3];
        delete suntimers[4];
        delete suntimers[5];
        delete suntimers[6];
        loadSunTimers();
    }
    if(!object["SUNRISE_LENGTH"].isNull() && object["SUNRISE_LENGTH"].is<uint16_t>()) {
        uint16_t length = object["SUNRISE_LENGTH"].as<uint16_t>();
        Serial.printf("New sunrise length: %d\n", length);
        if(length >= 1) {
            json["SUNRISE_LENGTH"] = length;
        }
    }

    if(!object["SUNDOWN_LENGTH"].isNull() && object["SUNDOWN_LENGTH"].is<uint16_t>()) {
        uint16_t length = object["SUNDOWN_LENGTH"].as<uint16_t>();
        Serial.printf("New sundown length: %d\n", length);
        if(length >= 1) {
            json["SUNDOWN_LENGTH"] = length;
        }
    }
    save();
}

void UserConfig::checkAddSingleTimer(JsonObject object, const char * day) {
    if(!object.isNull() && !object["dayOfWeek"].isNull() && !object["minutesDown"].isNull() && !object["minutesUp"].isNull() &&
     !object["hoursUp"].isNull() && !object["hoursDown"].isNull() && !object["enabled"].isNull()) {
         json["SunTimers"][day]["minutesDown"] = object["minutesDown"].as<uint8_t>();
         json["SunTimers"][day]["minutesUp"] = object["minutesUp"].as<uint8_t>();
         json["SunTimers"][day]["hoursUp"] = object["hoursUp"].as<uint8_t>();
         json["SunTimers"][day]["hoursDown"] = object["hoursDown"].as<uint8_t>();
         json["SunTimers"][day]["enabled"] = object["enabled"].as<bool>();
         Serial.printf("Successfully updated %s\n", day);
     } else {
         Serial.printf("Something is null\n");
         serializeJson(object, Serial);
     }
}

SunTimer* UserConfig::getSunTimerForDay(uint8_t day) {
    Serial.printf("Gettin suntimer for day %d\n", day);
    if(day == 0) {
        day = 6;
    } else {
        day = day -1;
    }
    return suntimers[day];
}