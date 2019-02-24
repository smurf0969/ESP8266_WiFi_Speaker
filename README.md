# ESP8266_WiFi_Speaker
ESP8266 ESP12E MP3 WiFi Speaker with OLED screen, DAC & AMP
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

## Putting it all together
Due to the possibility of frequent IP changes of the Raspberry Pi server I have used [WiFiManager](https://github.com/tzapu/WiFiManager) as it allows the use of custom paramters. I did find that after an upload a manual reset was sometimes needed if trying to use the access point.
After trying lots of different configurations for the server mp3 encoding(ffmpeg,darkice) and distribution(icecast2,shoutcast), I have settled on using [Icecast2](http://icecast.org/) and [DarkIce](http://www.darkice.org/) with the following [Darkice configuration](https://github.com/smurf0969/WiFi_Speaker_OLED/blob/master/docs/darkice.cfg).

## Thanks
Many thanks to the authors and contibutors for the main libraries that made this project possible  
* [ESP8266Audio]()
* [WiFiManager]()
* [Adafruit SSD1306](https://github.com/adafruit/Adafruit_SSD1306)
* [Adafruit GFX](https://github.com/adafruit/Adafruit-GFX-Library)

Stuart Blair (smurf0969)
