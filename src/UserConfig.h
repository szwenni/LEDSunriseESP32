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
    
#define WIFI_NAME "jjkroeger5a@t-online.de"
#define WIFI_PASSWORD "MeinNameIstJJKGeborenAm170568"

#define SUNRISE_LENGTH 1
#define SUNDOWN_LENGTH 1

#define COLOR_PALETTE_END 240

#define LAST_COLOR_R 0
#define LAST_COLOR_G 0
#define LAST_COLOR_B 0

#define NTP_SERVER "pool.ntp.org"
#define TIMEZONE_OFFSET 3600

#define COLOR_SEQUENCE RGB

#define BLACK_ON_START false

#define NUM_LEDS 2
#define DATA_PIN 13
#define CLOCK_PIN 12
    private:
        
        void generateDefault();
        void checkAddSingleTimer(JsonObject timer, const char *);
        SunTimer *suntimers[7];
        void loadSunTimers();
    
    public:
        StaticJsonDocument<3072> json;
        UserConfig();
        void setup();
        void save();
        void load();
        void check();
        void checkMqtt();
        bool summerTime = false;

        void checkAndNewConfig(JsonObject newConfig);
        void checkMQTTConfig(JsonObject object);

        void setStringProperty(String name, String value);
        String getStringProperty(String name);

        void setStringProperty(String sub, String name, String value);
        String getStringProperty(String sub, String name);

        void setU8Property(String name, uint8_t value);
        uint8_t getU8Property(String name);

        void setU8Property(String sub, String name, uint8_t value);
        uint8_t getU8Property(String sub, String name);

        void setU16Property(String name, uint16_t value);
        uint16_t getU16Property(String name);

        void setU16Property(String sub, String name, uint16_t value);
        uint16_t getU16Property(String sub, String name);

        SunTimer* getSunTimerForDay(uint8_t day);

        bool getBoolProperty(String name);
        bool getBoolProperty(String sub, String name);
};