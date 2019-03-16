#include <FS.h> //this needs to be first, or it all crashes and burns...

#include "Arduino.h"

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager

#include <ArduinoJson.h> //https://github.com/bblanchon/ArduinoJson

/*
   Set Speed to 160MHz
   Set IwIP to V2 (stable) or V2 MSS=1460(this is faster, but causes random crashes??!!)
*/

//audio https://github.com/earlephilhower/ESP8266Audio
#define useICY false //ICY=true,HTTP=false
#define useBuffer 0 //Can cause more stuttering on slow WiFi connections
const int bufferMultiply = 3; //max 5


#if useICY
#include "AudioFileSourceICYStream.h"
#else
#include "AudioFileSourceHTTPStream.h"
#endif
#include "AudioFileSourceBuffer.h"

#include "AudioGeneratorMP3.h"

#include "AudioOutputI2S.h"


/* PCM5102 DAC
   NodeMCU ESP8266 12E
   3.3V from ESP8266 -> VCC, 33V, XMT
   GND from ESP8266 -> GND, FLT, DMP, FMT, SCL
   GPIO15 (D8,TXD2) from ESP8266 -> BCK
   GPIO3 (RX,RXD0) from ESP8266 -> DIN
   GPIO2 (D4, TXD1) from ESP8266 -> LCK
*/

AudioGeneratorMP3 *ag;

#if useICY
AudioFileSourceICYStream *file;
#else
AudioFileSourceHTTPStream  *file;
#endif

AudioFileSourceBuffer *buff;
AudioOutputI2S *out;

#define sbVersion "1.3"

#define displayMemory 0
#define displayDebug 0

//******* OLED *********
// connect esp D1 to SCL, D2 to SDA
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "imagedata.h"

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     LED_BUILTIN //for ESP8266
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
#define oledPWR 10 //Pin to supply power to OLED
//********** END OLED ***************


// **************************************************
// **********    WiFiManager Callbacks    ***********
// **************************************************
void configModeCallback (WiFiManager *myWiFiManager) {

  Serial.println("Entered config mode");


  Serial.println(WiFi.softAPIP());
  displaySB();
  display.print(F("AP Mode: "));
  display.println(myWiFiManager->getConfigPortalSSID());
  display.display();

  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());

}
bool shouldSaveConfig = false;
void saveConfigCallback() {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

// **************************************************
// ********   WiFiManager Start Function   **********
// **************************************************
char pi_ip[16] = "192.168.1.64";
int json_doc_size = 128;
void WiFiStart(int tout = 120)
{
  WiFiManagerParameter custom_pi_ip("pi_ip", "pi_ip", pi_ip, 16);
  WiFiManager wifiManager;
  wifiManager.setTimeout(120);
  //wifiManager.resetSettings();
  wifiManager.setAPCallback(configModeCallback);
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  wifiManager.addParameter(&custom_pi_ip);
  wifiManager.setConfigPortalTimeout(tout);
  wifiManager.startConfigPortal();
  


  if (!wifiManager.autoConnect()) {

    Serial.println("failed to connect and hit timeout");
    displaySB();
    display.println(F("Wifi connect failed, unit will restart..."));
    display.display();
    delay(5000);
    //reset and try again, or maybe put it to deep sleep

    // ESP.reset();
    ESP.restart();

    delay(1000);

  }
  // start server
  if (shouldSaveConfig) {
    shouldSaveConfig = false;
    if(!SPIFFS.begin()){
      Serial.println("UNABLE to open SPIFFS");
      return;
    }

    strcpy(pi_ip, custom_pi_ip.getValue());

    Serial.println("saving config");
    DynamicJsonDocument doc(json_doc_size);


    doc["pi_ip"] = pi_ip;

    File configFile = SPIFFS.open("/config.json", "w");

    if (!configFile) {
      Serial.println("failed to open config file for writing");
    }
    serializeJson(doc, Serial);
    Serial.println("");
    serializeJson(doc, configFile);
    configFile.close();
    //end save
    SPIFFS.end();
  }
}

// **************************************************
// **********   Default Display Output    ***********
// **************************************************
void displaySB() {
  // Clear the buffer
  display.clearDisplay();
  display.setTextSize(1);             // Normal 1:1 pixel scale
  display.setTextColor(WHITE);        // Draw white text
  display.setCursor(0, 0);            // Start at top-left corner
  display.println(F("S.Blair Wifi Speaker"));
  display.print(F("Version: "));
  display.println(sbVersion);
  display.println(F(" "));
}
void displayMEM() {
  if (displayMemory) {
    display.print(F("Free Mem: "));
    display.println(ESP.getFreeHeap());
  }
}

// **************************************************
// **********   Generic RSSI WiFi Speed    **********
// **************************************************
char *RSSISpeed() {
  int sig = WiFi.RSSI();
  int rssi = abs(sig);
  if (rssi <= 98 && rssi > 93) {
    return "1 Mbps";
  } else if (rssi <= 93 && rssi > 91) {
    return "6 Mbps";
  } else if (rssi <= 91 && rssi > 75) {
    return "11 Mbps";
  } else if (rssi <= 75 && rssi > 71) {
    return "54 Mbps";
  } else if (rssi <= 71) {
    return "65Mbps";
  }
  String s = "" + sig;
  int l = s.length() + 1;
  char c[l];
  s.toCharArray(c, l);
  return c;
}

// **************************************************
// ************   ESP8266Audio Callbacks    *********
// **************************************************
// Called when a metadata event occurs (i.e. an ID3 tag, an ICY block, etc.

void MDCallback(void *cbData, const char *type, bool isUnicode, const char *string)

{

  const char *ptr = reinterpret_cast<const char *>(cbData);

  (void) isUnicode; // Punt this ball for now

  // Note that the type and string may be in PROGMEM, so copy them to RAM for printf

  char s1[32], s2[64];

  strncpy_P(s1, type, sizeof(s1));

  s1[sizeof(s1) - 1] = 0;

  strncpy_P(s2, string, sizeof(s2));

  s2[sizeof(s2) - 1] = 0;

  Serial.printf("METADATA(%s) '%s' = '%s'\n", ptr, s1, s2);

  Serial.flush();

}



// Called when there's a warning or error (like a buffer underflow or decode hiccup)

void StatusCallback(void *cbData, int code, const char *string)

{

  const char *ptr = reinterpret_cast<const char *>(cbData);

  // Note that the string may be in PROGMEM, so copy it to RAM for printf

  char s1[64];

  strncpy_P(s1, string, sizeof(s1));

  s1[sizeof(s1) - 1] = 0;

  Serial.printf("STATUS(%s) '%d' = '%s'\n", ptr, code, s1);

  Serial.flush();

}
// **************************************************
// ************   Main Setup Function     ***********
// **************************************************

char *URL = "http://%s:8000/live";
//const int preallocateBufferSize = 5*1024;
const int preallocateBufferSize = bufferMultiply * 1024;
const int preallocateCodecSize = 29192; // MP3 codec max mem needed
void *preallocateBuffer = NULL;
void *preallocateCodec = NULL;

void setup() {

  Serial.begin(115200);
  system_update_cpu_freq(SYS_CPU_160MHZ);
  pinMode(oledPWR, OUTPUT);
  digitalWrite(oledPWR, LOW);
  delay(100);
  digitalWrite(oledPWR, HIGH);
  delay(300);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    for (;;); // Don't proceed, loop forever
  }
  display.clearDisplay();
  
  //Draw startup logo
  display.drawBitmap((SCREEN_WIDTH - imageWidth) / 2, (SCREEN_HEIGHT - imageHeight) / 2, bitmap, imageWidth, imageHeight, WHITE);
  display.display();
  delay(4000);

  // First, preallocate all the memory needed for the buffering and codecs, never to be freed
  if (useBuffer)preallocateBuffer = malloc(preallocateBufferSize);
  preallocateCodec = malloc(preallocateCodecSize);
  if ((useBuffer && !preallocateBuffer) || !preallocateCodec) {
    Serial.printf_P(PSTR("FATAL ERROR:  Unable to preallocate %d bytes for app\n"), preallocateBufferSize + preallocateCodecSize);
    display.clearDisplay();
    display.print(F("FATAL ERROR:  Unable to preallocate "));
    display.print( preallocateBufferSize + preallocateCodecSize);
    display.println(F(" bytes for app"));
    display.display();
    while (1) delay(1000); // Infinite halt
  } else {
    Serial.println("");
    if (useBuffer) {
      Serial.printf_P(PSTR("Preallocate %d bytes for Buffer\n"), preallocateBufferSize);
    } else {
      Serial.println("NOT USING BUFFER");
    }
    Serial.printf_P(PSTR("Preallocate %d bytes for Codec\n"), preallocateCodecSize);
  }

  displaySB();
  display.println(F("Starting ..."));
  display.display();
  delay(1000);

  //Clear file system, use once for old chips or to delete config
  /*
    SPIFFS.begin();
    if(SPIFFS.format()){
      displaySB();
      display.println("File System Formatted");
     display.display();
     delay(1000);
      for(;;); // Don't proceed, loop forever
    }
  */
  if (!SPIFFS.begin()) {
    Serial.println("Unable to start SPIFFS");
    display.clearDisplay();
    display.println(F("UNABLE to start SPIFFS"));
    display.display();
    while (1) {
      delay(1000);
    }
  } else {
    Serial.println("mounted file system");

    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("reading config file");

      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("opened config file");
        size_t sizec = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[sizec]);

        configFile.readBytes(buf.get(), sizec);
        DynamicJsonDocument doc(1024);
        DeserializationError error = deserializeJson(doc, buf.get());
        if (error) {
          Serial.println("failed to load json config");
        } else {
          strcpy(pi_ip, doc["pi_ip"]);
        }
        configFile.close();

      }
    }
    SPIFFS.end();
  }
  
  WiFiStart(30); //Load AP and connect to WiFi

  Serial.printf("\nWiFi RSSI: %d\n",WiFi.RSSI()); //our initial WiFi speed estimate

  displaySB();
  display.print(F("IP: "));
  display.println(WiFi.localIP());
  display.print(F("Speed Est.: "));
  display.println(RSSISpeed());

  char url_buf[(sizeof(URL)-2)+sizeof(pi_ip)];
  sprintf(url_buf,URL,pi_ip);
  Serial.printf("\nURL: %s\n",url_buf);
  display.printf("\nURL: %s\n",url_buf);
  display.display();
  delay(3000);
  
#if useICY
  file = new AudioFileSourceICYStream(url_buf);
  Serial.printf_P(PSTR("Using ICY Stream\n"));
  //file->RegisterMetadataCB(MDCallback, (void*)"ICY");
#else
  file = new AudioFileSourceHTTPStream(url_buf);
  Serial.printf_P(PSTR("Using HTTP Stream\n"));
#endif
  
  if (useBuffer) {
    //buff = new AudioFileSourceBuffer(file, 2048);
    buff = new AudioFileSourceBuffer(file, preallocateBuffer, preallocateBufferSize);
    buff->RegisterStatusCB(StatusCallback, (void*)"buffer");
  }

  out = new AudioOutputI2S();
  out->SetGain(0.2); // 0 - 4, 0.3 lower values reduce crackle
  out->SetRate(44100);
  out->SetBitsPerSample(16);
  out->SetChannels(2);

  ag = new AudioGeneratorMP3(preallocateCodec, preallocateCodecSize);

  if (useBuffer) {
    ag->begin(buff, out);
  } else {
    ag->begin(file, out);
  }
  
  Serial.printf("FreeHeap: %d\n",ESP.getFreeHeap());//Too low on free heap and we will never work!!
}//end setup
//###########################################################
//###########################################################
//###########################################################

//###########################################################
//####################    Main Loop    ######################
//###########################################################
int lastms = 0;
int runningState = -1;
int playStartMS = -1;
int pcount = 0;
void loop() {
  int ms = millis();


  if (ag->isRunning()) {
    runningState = 1;
    if (playStartMS == -1) {
      playStartMS = ms;
    }
    if (displayDebug && (ms - lastms) > 1000) {
      displaySB();
      display.print(F("IP: "));
      display.println(WiFi.localIP());
      display.print(F("Speed Est.: "));
      display.println(RSSISpeed());
      displayMEM();
      display.printf("Playing: %d secs", (ms - playStartMS) / 1000);
      display.println("");
      display.display();
      lastms = ms;
    } else if ((ms - lastms) > 10000) {
      if (pcount < 3) {
        if (pcount == 0) {
          display.clearDisplay();
          display.drawBitmap((SCREEN_WIDTH - imageWidth) / 2, (SCREEN_HEIGHT - imageHeight) / 2, bitmapPlay, imageWidth, imageHeight, WHITE);
          display.display();
        }
        pcount++;
      } else {
        displaySB();
        display.print(F("IP: "));
        display.println(WiFi.localIP());
        display.print(F("Speed Est.: "));
        display.println(RSSISpeed());
        display.display();
        pcount = 0;
      }
      lastms = ms;

      //Serial.printf("loop time %d\n", ms - millis());
      //Serial.printf("Heap: %d \n", ESP.getFreeHeap());

    }

    if (!ag->loop()) {
      ag->stop();
    }


  } else {
    //Serial.printf("MP3 done\n");
    Serial.println("");
    Serial.print(F("******* Played: "));
    Serial.print((ms - playStartMS) / 1000);
    Serial.println(F(" secs *******"));
    Serial.println("");
    display.clearDisplay();
    display.drawBitmap((SCREEN_WIDTH - imageWidth) / 2, (SCREEN_HEIGHT - imageHeight) / 2, bitmapPoo, imageWidth, imageHeight, WHITE);
    display.display();
    delay(15000);
    displaySB();
    display.print(F("IP: "));
    display.println(WiFi.localIP());
    display.print(F("Speed Est.: "));
    display.println(RSSISpeed());
    display.println(F(" "));
    displayMEM();
    switch (runningState) {
      case 1:
        runningState = 0;
        playStartMS = -1;
        display.println(F("Playing: Stopped"));
        break;
      case 0:
        display.println(F("Playing: retry???"));
        break;
      default:
        display.println(F("NO MP3 Connction made"));
    }
    if ((ms) >= 90000 ) {
      display.println(F("AWAITING RESET"));
      display.display();

      delete ag;
      delete out;
      delete buff;
      delete file;

      delay(5000);
      ESP.restart();
      delay(1000);
    } else {
      display.println(F("ERROR: MANUALLY RESET"));
      display.display();
      while (1) delay(1000); // Don't proceed, loop forever
    }
  }

}
