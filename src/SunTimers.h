#include <Arduino.h>
#include <ArduinoJson.h>


class SunTimerConf {
    public:
        uint8_t minutesDown;
        uint8_t minutesUp;
        uint8_t hoursUp;
        uint8_t hoursDown;
        bool enabled;
};



class SunTimer {
    public:
        String dayOfWeek;
        SunTimerConf *sunTimer;
        SunTimer(String dayOfWeek, uint8_t minutesDown, uint8_t minutesUp, uint8_t hoursUp, uint8_t hoursDown, bool enabled);
        SunTimer(JsonObject obj);
        ~SunTimer();
        JsonObject toJson(JsonObject object);
};

