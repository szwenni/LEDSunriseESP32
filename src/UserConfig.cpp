
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
    JsonObject sunTimerConfig = json.getMember("SunTimerConfig");
    suntimers[0] = new SunTimer(sunTimerConfig.getMember("SunTimers").getMember(MONDAY).as<JsonObject>());
    suntimers[1] = new SunTimer(sunTimerConfig.getMember("SunTimers").getMember(TUESDAY).as<JsonObject>());
    suntimers[2] = new SunTimer(sunTimerConfig.getMember("SunTimers").getMember(WEDNESDAY).as<JsonObject>());
    suntimers[3] = new SunTimer(sunTimerConfig.getMember("SunTimers").getMember(THURSDAY).as<JsonObject>());
    suntimers[4] = new SunTimer(sunTimerConfig.getMember("SunTimers").getMember(FRIDAY).as<JsonObject>());
    suntimers[5] = new SunTimer(sunTimerConfig.getMember("SunTimers").getMember(SATURDAY).as<JsonObject>());
    suntimers[6] = new SunTimer(sunTimerConfig.getMember("SunTimers").getMember(SUNDAY).as<JsonObject>());
}

void UserConfig::check() {
    if(!json.getMember("SunTimers").isNull()) {
        json.remove("SunTimers");
    }
    //if(json.getMember("SunTimers").isNull()) {
    if(json.getMember("SunTimerConfig").isNull()) {
        JsonObject sunTimerConfig = json.createNestedObject("SunTimerConfig");
        SunTimer *monday = new SunTimer(MONDAY, 0, 0, 0, 0, false);
        SunTimer *tuesday = new SunTimer(TUESDAY, 0, 0, 0, 0, false);
        SunTimer *wednesday = new SunTimer(WEDNESDAY, 0, 0, 0, 0, false);
        SunTimer *thursday = new SunTimer(THURSDAY, 0, 0, 0, 0, false);
        SunTimer *friday = new SunTimer(FRIDAY, 0, 0, 0, 0, false);
        SunTimer *saturday = new SunTimer(SATURDAY, 0, 0, 0, 0, false);
        SunTimer *sunday = new SunTimer(SUNDAY, 0, 0, 0, 0, false);
        JsonObject timers = sunTimerConfig.createNestedObject("SunTimers");
        
        monday->toJson(timers.createNestedObject(monday->dayOfWeek));
        tuesday->toJson(timers.createNestedObject(tuesday->dayOfWeek));
        wednesday->toJson(timers.createNestedObject(wednesday->dayOfWeek));
        thursday->toJson(timers.createNestedObject(thursday->dayOfWeek));
        friday->toJson(timers.createNestedObject(friday->dayOfWeek));
        saturday->toJson(timers.createNestedObject(saturday->dayOfWeek));
        sunday->toJson(timers.createNestedObject(sunday->dayOfWeek));
        sunTimerConfig["SUNDOWN_LENGTH"] = SUNDOWN_LENGTH;
        sunTimerConfig["SUNRISE_LENGTH"] = SUNRISE_LENGTH;
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

    if(json.getMember("WIFI_NAME").isNull()) {
        json["WIFI_NAME"] = WIFI_NAME;
        json["WIFI_PASSWORD"] = WIFI_PASSWORD;
    }

    if(json.getMember("LAST_COLOR_R").isNull()) {
        json["LAST_COLOR_R"] = LAST_COLOR_R;
    }
    if(json.getMember("LAST_COLOR_G").isNull()) {
        json["LAST_COLOR_G"] = LAST_COLOR_G;
    }
    if(json.getMember("LAST_COLOR_B").isNull()) {
        json["LAST_COLOR_B"] = LAST_COLOR_B;
    }
    checkMqtt();
}

void UserConfig::checkMqtt() {
    if(json.getMember("MQTT").isNull()) {
        json.createNestedObject("MQTT");
        json["MQTT"]["MQTT_ENABLED"] = false;
        json["MQTT"]["MQTT_ADDRESS"] = "";
        json["MQTT"]["MQTT_PORT"] = 1883;
        json["MQTT"]["MQTT_USERNAME"] = "";
        json["MQTT"]["MQTT_PASSWORD"] = "";
        json["MQTT"]["MQTT_DEVICEID"] = "";
        json["MQTT"]["MQTT_DEVICENAME"] = "";
        json["MQTT"]["MQTT_NODENAME"] = "";
        json["MQTT"]["MQTT_SUNUPPROPNAME"] = "";
        json["MQTT"]["MQTT_SUNDOWNPROPNAME"] = "";
        json["MQTT"]["MQTT_COLORPROPNAME"] = "";
    }
    if(json.getMember("MQTT").getMember("MQTT_BRIGHTPROPNAME").isNull()) {
        json["MQTT"]["MQTT_BRIGHTPROPNAME"] = "";
        json["MQTT"]["MQTT_ENABLED"] = false;
        Serial.println("BrightProp is null");
    }
    save();

}

void UserConfig::generateDefault() {
    json["WIFI_NAME"] = WIFI_NAME;
    json["WIFI_PASSWORD"] = WIFI_PASSWORD;
    check();
    loadSunTimers();
}

void UserConfig::setStringProperty(String name, String value) {
    json[name] = value;
}

String UserConfig::getStringProperty(String name) {
    return json.getMember(name).as<String>();
}

void UserConfig::setStringProperty(String sub, String name, String value) {
    json[sub][name] = value;
}

String UserConfig::getStringProperty(String sub, String name) {
    return json.getMember(sub).getMember(name).as<String>();
}

void UserConfig::setU8Property(String name, uint8_t value) {
    json[name] = value;
}

uint8_t UserConfig::getU8Property(String name) {
    return json.getMember(name).as<uint8_t>();
}

void UserConfig::setU8Property(String sub, String name, uint8_t value) {
    json[sub][name] = value;
}

uint8_t UserConfig::getU8Property(String sub, String name) {
    return json.getMember(sub).getMember(name).as<uint8_t>();
}

void UserConfig::setU16Property(String name, uint16_t value) {
    json[name] = value;
}

uint16_t UserConfig::getU16Property(String name) {
    return json.getMember(name).as<uint16_t>();
}

void UserConfig::setU16Property(String sub, String name, uint16_t value) {
    json[sub][name] = value;
}

uint16_t UserConfig::getU16Property(String sub, String name) {
    return json.getMember(sub).getMember(name).as<uint16_t>();
}

bool UserConfig::getBoolProperty(String name) {
    return json.getMember(name).as<bool>();
}


bool UserConfig::getBoolProperty(String sub, String name) {
    return json.getMember(sub).getMember(name).as<bool>();
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
            json["SunTimerConfig"]["SUNRISE_LENGTH"] = length;
        }
    }

    if(!object["SUNDOWN_LENGTH"].isNull() && object["SUNDOWN_LENGTH"].is<uint16_t>()) {
        uint16_t length = object["SUNDOWN_LENGTH"].as<uint16_t>();
        Serial.printf("New sundown length: %d\n", length);
        if(length >= 1) {
            json["SunTimerConfig"]["SUNDOWN_LENGTH"] = length;
        }
    }
    
    save();
}

void UserConfig::checkAddSingleTimer(JsonObject object, const char * day) {
    if(!object.isNull() && !object["dayOfWeek"].isNull() && !object["minutesDown"].isNull() && !object["minutesUp"].isNull() &&
     !object["hoursUp"].isNull() && !object["hoursDown"].isNull() && !object["enabled"].isNull()) {
         json["SunTimerConfig"]["SunTimers"][day]["minutesDown"] = object["minutesDown"].as<uint8_t>();
         json["SunTimerConfig"]["SunTimers"][day]["minutesUp"] = object["minutesUp"].as<uint8_t>();
         json["SunTimerConfig"]["SunTimers"][day]["hoursUp"] = object["hoursUp"].as<uint8_t>();
         json["SunTimerConfig"]["SunTimers"][day]["hoursDown"] = object["hoursDown"].as<uint8_t>();
         json["SunTimerConfig"]["SunTimers"][day]["enabled"] = object["enabled"].as<bool>();
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

void UserConfig::checkMQTTConfig(JsonObject object) {
    bool mqttCanEnabled = true;
    
    if(!object.getMember("MQTT_ENABLED").isNull() && object["MQTT_ENABLED"].is<bool>()) {
        json["MQTT"]["MQTT_ENABLED"] = object["MQTT_ENABLED"].as<bool>();
    }

    if(!object.getMember("MQTT_ADDRESS").isNull() && object["MQTT_ADDRESS"].is<String>()) {
        if(object["MQTT_ADDRESS"].as<String>().length() > 0) {
            json["MQTT"]["MQTT_ADDRESS"] = object["MQTT_ADDRESS"].as<String>();
        } else {
            mqttCanEnabled = false;
        }
    }
    if(!object.getMember("MQTT_PORT").isNull() && object["MQTT_PORT"].is<uint16_t>()) {
        json["MQTT"]["MQTT_PORT"] = object["MQTT_PORT"].as<uint16_t>();
    }
    if(!object.getMember("MQTT_USERNAME").isNull() && object["MQTT_USERNAME"].is<String>()) {
            json["MQTT"]["MQTT_USERNAME"] = object["MQTT_USERNAME"].as<String>();
    }
    if(!object.getMember("MQTT_PASSWORD").isNull() && object["MQTT_PASSWORD"].is<String>()) {
        
            json["MQTT"]["MQTT_PASSWORD"] = object["MQTT_PASSWORD"].as<String>();
    }

    if(!object.getMember("MQTT_DEVICEID").isNull() && object["MQTT_DEVICEID"].is<String>()) {
        if(object["MQTT_DEVICEID"].as<String>().length() > 0) {
            json["MQTT"]["MQTT_DEVICEID"] = object["MQTT_DEVICEID"].as<String>();
        } else {
            mqttCanEnabled = false;
        }
    }

    if(!object.getMember("MQTT_DEVICENAME").isNull() && object["MQTT_DEVICENAME"].is<String>()) {
        if(object["MQTT_DEVICENAME"].as<String>().length() > 0) {
            json["MQTT"]["MQTT_DEVICENAME"] = object["MQTT_DEVICENAME"].as<String>();
        } else {
            mqttCanEnabled = false;
        }
    }
    if(!object.getMember("MQTT_NODENAME").isNull() && object["MQTT_NODENAME"].is<String>()) {
        if(object["MQTT_NODENAME"].as<String>().length() > 0) {
            json["MQTT"]["MQTT_NODENAME"] = object["MQTT_NODENAME"].as<String>();
        } else {
            mqttCanEnabled = false;
        }
    }
    if(!object.getMember("MQTT_SUNUPPROPNAME").isNull() && object["MQTT_SUNUPPROPNAME"].is<String>()) {
        if(object["MQTT_SUNUPPROPNAME"].as<String>().length() > 0) {
            json["MQTT"]["MQTT_SUNUPPROPNAME"] = object["MQTT_SUNUPPROPNAME"].as<String>();
        } else {
            mqttCanEnabled = false;
        }
    }
    if(!object.getMember("MQTT_SUNDOWNPROPNAME").isNull() && object["MQTT_SUNDOWNPROPNAME"].is<String>()) {
        if(object["MQTT_SUNDOWNPROPNAME"].as<String>().length() > 0) {
            json["MQTT"]["MQTT_SUNDOWNPROPNAME"] = object["MQTT_SUNDOWNPROPNAME"].as<String>();
        } else {
            mqttCanEnabled = false;
        }
    }
    if(!object.getMember("MQTT_COLORPROPNAME").isNull() && object["MQTT_COLORPROPNAME"].is<String>()) {
        if(object["MQTT_COLORPROPNAME"].as<String>().length() > 0) {
            json["MQTT"]["MQTT_COLORPROPNAME"] = object["MQTT_COLORPROPNAME"].as<String>();
        } else {
            mqttCanEnabled = false;
        }
    }
    if(!object.getMember("MQTT_BRIGHTPROPNAME").isNull() && object["MQTT_BRIGHTPROPNAME"].is<String>()) {
        if(object["MQTT_BRIGHTPROPNAME"].as<String>().length() > 0) {
            json["MQTT"]["MQTT_BRIGHTPROPNAME"] = object["MQTT_BRIGHTPROPNAME"].as<String>();
        } else {
            mqttCanEnabled = false;
        }
    }

    if(!object.getMember("MQTT_ENABLED").isNull() && object["MQTT_ENABLED"].is<bool>()) {
        if(mqttCanEnabled) {
            json["MQTT"]["MQTT_ENABLED"] = object["MQTT_ENABLED"].as<bool>();
        } else {
            json["MQTT"]["MQTT_ENABLED"] = false;
        }
    }
    save();
}