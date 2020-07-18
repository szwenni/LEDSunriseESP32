#include "SunTimers.h"

SunTimer::SunTimer(String dayOfWeek, uint8_t minutesDown, uint8_t minutesUp, uint8_t hoursUp, uint8_t hoursDown, bool enabled) {
    this->dayOfWeek = dayOfWeek;
    this->sunTimer = new SunTimerConf();
    this->sunTimer->enabled = enabled;
    this->sunTimer->hoursDown = hoursDown;
    this->sunTimer->minutesDown = minutesDown;
    this->sunTimer->minutesUp = minutesUp;
    this->sunTimer->hoursUp = hoursUp;
}

SunTimer::SunTimer(JsonObject obj) {
    this->dayOfWeek = obj.getMember("dayOfWeek").as<String>();
    this->sunTimer = new SunTimerConf();
    this->sunTimer->minutesDown = obj.getMember("minutesDown").as<uint8_t>();
    this->sunTimer->minutesUp = obj.getMember("minutesUp").as<uint8_t>();
    this->sunTimer->hoursUp = obj.getMember("hoursUp").as<uint8_t>();
    this->sunTimer->hoursDown = obj.getMember("hoursDown").as<uint8_t>();
    this->sunTimer->enabled = obj.getMember("enabled").as<bool>();
    
}
SunTimer::~SunTimer() {
    delete this->sunTimer;
}


JsonObject SunTimer::toJson(JsonObject object) {
    object["dayOfWeek"] = this->dayOfWeek;
    object["minutesDown"] = this->sunTimer->minutesDown;
    object["minutesUp"] = this->sunTimer->minutesUp;
    object["hoursUp"] = this->sunTimer->hoursUp;
    object["hoursDown"] = this->sunTimer->hoursDown;
    object["enabled"] = this->sunTimer->enabled;
    return object;
}