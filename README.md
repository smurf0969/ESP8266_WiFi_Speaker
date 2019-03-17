# ESP8266_WiFi_Speaker [![Build Status](https://travis-ci.com/smurf0969/ESP8266_WiFi_Speaker.svg?branch=master)](https://travis-ci.com/smurf0969/ESP8266_WiFi_Speaker)
ESP8266 ESP12E MP3 WiFi Speaker with OLED screen, DAC & AMP 

_**This project has been replaced by [ESP32 ESP8266 WiFi Speaker OLED](https://github.com/smurf0969/ESP32_ESP8266_WiFi_Speaker_OLED)**_  
## Overview
During a recent pantomime production we were unable to get the internal building sound system working and had to come up with a cheap solution to pipe music into the dressing rooms.
Cobbling together a Raspberry Pi with sound card, ffmpeg and node rmtp server we were able to stream live AAC audio to a couple of mobiles running VLC Player. However due to the limitations of running live streaming on a PI, also on poor WiFi and the buffering lag of VLC we ended up with a live stream that could be upto 40 seconds behind depending on the numer of connections, which was difficult for the cast and callers.

Having a couple of node mcu ESP8266 12E development boards that were destined for some extra home automation, I decided to see if it was possible to make a WiFi speaker that would perform better than our previous solution.

## Components  
* ESP8266 ESP-12E Development Board NodeMcu
* 2x 4ohm 3W Loudspeaker
* Mini 3W+3W DC 5V Audio Amplifier PAM8043
* I2S PCM5102 DAC Decoder ( [Earle F. Philhower, III](https://github.com/earlephilhower/ESP8266Audio) [ESP8266Audio](https://github.com/earlephilhower/ESP8266Audio) does allow for no decoder, but I have not tried it. )
* 0.96" I2C IIC Serial 128X64 White OLED LCD LED 

First major component problem was figuring out the DAC connections to use with AudioOutputI2S in the [ESP8266Audio](https://github.com/earlephilhower/ESP8266Audio) library.  
The configuration I finnally manages to get to work was:  
> 3.3V from ESP8266 -> VCC, 33V, XMT  
> GND from ESP8266 -> GND, FLT, DMP, FMT, SCL  
> GPIO15 (D8,TXD2) from ESP8266 -> BCK  
> GPIO3 (RX,RXD0) from ESP8266 -> DIN  
> GPIO2 (D4, TXD1) from ESP8266 -> LCK  
   
The 5V Audio Amplifier PAM8043 needs a lot of juice and I found that as mine has a potentiometer I needed to turn it fully down to be able to program or run the ESP when only connected by USB serial connection. Powering from a USB power adapter for normal use, I had no problems.

## IDE settings & Library Versions
Please check/review instructions and screenshots in the [Wiki](https://github.com/smurf0969/ESP8266_WiFi_Speaker/wiki)

### Known Issues
Depending on the streaming service and the quality of the stream you are connecting to stability of the connection and the speed at which HttpClient gets the data can vary. [issue #2](https://github.com/smurf0969/ESP8266_WiFi_Speaker/issues/2)

## Putting it all together
Due to the possibility of frequent IP changes of the Raspberry Pi server I have used [WiFiManager](https://github.com/tzapu/WiFiManager) as it allows the use of custom paramters. I did find that after an upload a manual reset was sometimes needed if trying to use the access point.
After trying lots of different configurations for the server mp3 encoding(ffmpeg,darkice) and distribution(icecast2,shoutcast), I have settled on using [Icecast2](http://icecast.org/) and [DarkIce](http://www.darkice.org/) with the following [Darkice configuration](./docs/darkice.cfg).

## Thanks
Many thanks to the authors and contibutors for the main libraries that made this project possible  
* [ESP8266Audio](https://github.com/earlephilhower/ESP8266Audio)
* [WiFiManager](https://github.com/tzapu/WiFiManager)
* [Adafruit SSD1306](https://github.com/adafruit/Adafruit_SSD1306)
* [Adafruit GFX](https://github.com/adafruit/Adafruit-GFX-Library)

Stuart Blair (smurf0969)
