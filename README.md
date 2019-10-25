# Sunrise/Sundown Timer
Quick and dirty implementation of a configurable sunrise and sundown timer together with FastLED.h on ESP32.

## Configuration
Just add WIFI_NAME and WIFI_PASSWORD in UserConfig.h before first flash.

## Connection
For a P9813 controller you can use it as is. >ou might need to adopt `NUM_LEDS`, `DATA_PIN`, `CLOCK_PIN`. `NUM_LEDS` corresponds to the P9813 controller chain length.

## Web-Interface
In PlatformIO run the "Upload File System Image" prior to first boot. It will add the main Web page to the storage. After successfull build it will be available if you navigate to the ip of your ESP32.