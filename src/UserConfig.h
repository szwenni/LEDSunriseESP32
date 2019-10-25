#include <Arduino.h>
#include <ArduinoJson.h>
#include <SunTimers.h>

#define MONDAY "Monday"
#define TUESDAY "Tuesday"
#define WEDNESDAY "Wednesday"
#define THURSDAY "Thursday"
#define FRIDAY "Friday"
#define SATURDAY "Saturday"
#define SUNDAY "Sunday"


class UserConfig {
    
#define WIFI_NAME ""
#define WIFI_PASSWORD ""

#define SUNRISE_LENGTH 1
#define SUNDOWN_LENGTH 1

#define NTP_SERVER "pool.ntp.org"
#define TIMEZONE_OFFSET 3600

#define NUM_LEDS 2
#define DATA_PIN 13
#define CLOCK_PIN 12
    private:
        
        void generateDefault();
        void checkAddSingleTimer(JsonObject timer, const char *);
        SunTimer *suntimers[7];
        void loadSunTimers();
    
    public:
        StaticJsonDocument<2048> json;
        UserConfig();
        void setup();
        void save();
        void load();
        void check();
        bool summerTime = false;

        void checkAndNewConfig(JsonObject newConfig);

        void setStringProperty(String name, String value);
        String getStringProperty(String name);

        void setU8Property(String name, uint8_t value);
        uint8_t getU8Property(String name);

        void setU16Property(String name, uint16_t value);
        uint16_t getU16Property(String name);
        SunTimer* getSunTimerForDay(uint8_t day);

        bool getBoolProperty(String name);
};