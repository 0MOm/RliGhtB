/*
  WS2812FX Webinterface.
  
  Harm Aldick - 2016
  www.aldick.org

  
  FEATURES
    * Webinterface with mode, color, speed and brightness selectors


  LICENSE

  The MIT License (MIT)

  Copyright (c) 2016  Harm Aldick 

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.

  
  CHANGELOG
  2016-11-26 initial version
  2018-01-06 added custom effects list option and auto-cycle feature
  
*/
#include <FS.h>                   //this needs to be first, or it all crashes and burns...

#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <WS2812FX.h>

//needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager

#include <ArduinoOTA.h>

#include <Ticker.h>               //for LED status

#include <GyverButton.h>

#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson

#define BUTTON_PIN D3 //NODE MCU Flash button connect to PIN0 - D3
GButton button(BUTTON_PIN);

const uint16_t localPort = 54321; // Local port to listen for UDP packets

const uint32_t stepDuration = 2000;
//static uint32_t nextTime = 0;
IPAddress broadcastAddress;
IPAddress myAddress;
WiFiUDP udp;
uint32_t sendIPcnt = 0;

Ticker ticker; // запускается по насраиваемому интервалу
Ticker sendIP;
extern const char index_html[];
extern const char main_js[];

//#define WIFI_SSID "Keenetic-1069"
//#define WIFI_PASSWORD "RxrrKy6e"

//#define STATIC_IP                       // uncomment for static IP, set IP below
// #ifdef STATIC_IP
//   IPAddress ip(192,168,0,123);
//   IPAddress gateway(192,168,0,1);
//   IPAddress subnet(255,255,255,0);
// #endif

// QUICKFIX...See https://github.com/esp8266/Arduino/issues/263
#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>(b)?(a):(b))

static const uint8_t  LED_PIN   =  D6;
static const uint8_t  LED       =  D4;  //blue LED on ESP-12E
static const uint8_t  LED_COUNT =  24;

#define WIFI_TIMEOUT 30000              // checks WiFi every ...ms. Reset after this time, if WiFi cannot reconnect.
#define HTTP_PORT 80

bool shouldSaveConfig = false;

//parametrs wich reads from FS
uint8_t powerOnMode;
uint32_t powerOnColor;
uint16_t powerOnSpeed;
uint8_t powerOnBrightness;

unsigned long auto_last_change = 0;
unsigned long last_wifi_check_time = 0;
String modes = "";
uint8_t myModes[] = {}; // *** optionally create a custom list of effect/mode numbers
boolean auto_cycle = false;

WS2812FX ws2812fx = WS2812FX(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

ESP8266WebServer server(HTTP_PORT);

///////////////
WiFiManager wifiManager;

///
void modes_setup();

void srv_handle_not_found();

void srv_handle_index_html();

void srv_handle_main_js();

void srv_handle_modes();

void srv_handle_set();

bool sendPacket(const IPAddress& address, const uint8_t* buf, uint8_t bufSize);
////

void tick()
{
  //toggle state
  int state = digitalRead(LED);  // get the current state of GPIO1 pin
  digitalWrite(LED, !state);     // set pin to the opposite state
}

//gets called when WiFiManager enters configuration mode
void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
  //entered config mode, make led toggle faster
  ticker.attach(0.2, tick);
}

void sendIPfun() {
  int state = digitalRead(LED);  // get the current state of GPIO1 pin
  digitalWrite(LED, !state);     // set pin to the opposite state  
  sendIPcnt++;
  myAddress = WiFi.localIP();
  Serial.print(F("Send IP address: "));
  Serial.println(myAddress);
  if (! sendPacket(broadcastAddress, (uint8_t*)&myAddress, sizeof(myAddress))) {
    Serial.println(F("Error sending broadcast UDP packet!"));
  }
}

void wifi_setup() {
  // put your setup code here, to run once:
  
  //set led pin as output
  pinMode(LED, OUTPUT);
  // start ticker with 0.5 because we start in AP mode and try to connect
  ticker.attach(0.6, tick);

  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around

  //reset settings - for testing
  //wifiManager.resetSettings();

  //set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wifiManager.setAPCallback(configModeCallback);

  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  if (!wifiManager.autoConnect()) {
    Serial.println("failed to connect and hit timeout");
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(1000);
  }

  //if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");
  
  ticker.detach();
  //keep LED on
  digitalWrite(LED, LOW);
}


void FS_setup() {
  //clean FS, for testing
  //SPIFFS.format();

  //read configuration from FS json
  Serial.println("mounting FS...");

  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
          Serial.println("\nparsed json");
          powerOnMode = json["mode"];
          powerOnColor = json["color"];
          powerOnSpeed = json["speed"];
          powerOnBrightness = json["brightness"];
          //strcpy(blynk_token, json["blynk_token"]);

        } else {
          Serial.println("failed to load json config");
        }
        configFile.close();
      }
    }
    else {
      Serial.println("failed to open /config.json");
    }
  } else {
    Serial.println("failed to mount FS");
  }
  
}
void setup(){

  Serial.begin(115200);
  Serial.println();
  Serial.println();
  Serial.println("Starting...");

  FS_setup();

  modes.reserve(5000);
  modes_setup();

  button.setTimeout(400);   // настройка таймаута на удержание и второй клик (по умолчанию 500 мс)

  Serial.println("WS2812FX setup");
  ws2812fx.init();
  if (powerOnMode != 0) {
    ws2812fx.setMode(powerOnMode);
    ws2812fx.setColor(powerOnColor);
    ws2812fx.setSpeed(powerOnSpeed);
    ws2812fx.setBrightness(powerOnBrightness);
  }
  else {
    ws2812fx.setMode(DEFAULT_MODE);
    ws2812fx.setColor(DEFAULT_COLOR);
    ws2812fx.setSpeed(DEFAULT_SPEED);
    ws2812fx.setBrightness(DEFAULT_BRIGHTNESS);
  }
  ws2812fx.start();

  Serial.println("Wifi setup");
  wifi_setup();
 
  Serial.println("HTTP server setup");
  server.on("/", srv_handle_index_html);
  server.on("/main.js", srv_handle_main_js);
  server.on("/modes", srv_handle_modes);
  server.on("/set", srv_handle_set);
  server.onNotFound(srv_handle_not_found);
  server.begin();
  Serial.println("HTTP server started.");

  Serial.println("ready!");
  
  //UDP
  broadcastAddress = (uint32_t)WiFi.localIP() | ~((uint32_t)WiFi.subnetMask());
  Serial.print(F("Broadcast address: "));
  Serial.println(broadcastAddress);

  Serial.println(F("Starting UDP"));
  udp.begin(localPort);
  Serial.print(F("Local port: "));
  Serial.println(udp.localPort());
  
  sendIP.attach(2,sendIPfun);

  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_SPIFFS
      type = "filesystem";
    }

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

}


bool sendPacket(const IPAddress& address, const uint8_t* buf, uint8_t bufSize) {
  udp.beginPacket(address, localPort);
  udp.write(buf, bufSize);
  return (udp.endPacket() == 1);
}

// ненужная функция
void receivePacket() {
  bool led;

  udp.read((uint8_t*)&led, sizeof(led));
  udp.flush();
  if (udp.destinationIP() != broadcastAddress) {
    Serial.print(F("Client with IP "));
    Serial.print(udp.remoteIP());
    Serial.print(F(" turned led "));
    Serial.println(led ? F("off") : F("on"));
  } else {
    Serial.println(F("Skip broadcast packet"));
  }
}

void loop() {
  unsigned long now = millis();

  ArduinoOTA.handle();

  //disable send IP adress after 30 post
  if (sendIPcnt > 30) {
    sendIP.detach();
  }

  button.tick();  // обязательная функция отработки. Должна постоянно опрашиваться

  server.handleClient();
  ws2812fx.service();

  //  if (millis() >= nextTime) {
  //    //digitalWrite(ledPin, led);
  //    Serial.print(F("Send IP address: "));
  //    Serial.println(broadcastAddress);
  //    if (! sendPacket(broadcastAddress, (uint8_t*)&broadcastAddress, sizeof(broadcastAddress)))
  //      Serial.println(F("Error sending broadcast UDP packet!"));
  //    nextTime = millis() + stepDuration;
  //  }

  if(now - last_wifi_check_time > WIFI_TIMEOUT) {
    Serial.print("Checking WiFi... ");

    if (!wifiManager.autoConnect()) {
      Serial.println("failed to connect and hit timeout");
      //reset and try again, or maybe put it to deep sleep
      digitalWrite(LED, false);
      ticker.attach(0.2, tick);
      //ESP.reset();
      //delay(1000);
    }
    else {
      ticker.detach();
      digitalWrite(LED, true);
      Serial.println("OK");
    }



    // if(WiFi.status() != WL_CONNECTED) {
    //   Serial.println("WiFi connection lost. Reconnecting...");
    //   wifi_setup(); 
    // } else {
    //   Serial.println("OK");
    // }

    last_wifi_check_time = now;
  }
  
//save the custom parameters to FS
  if (button.hasClicks()|| shouldSaveConfig) {
    shouldSaveConfig = false;
    Serial.println("saving config");
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json["mode"] = powerOnMode;
    json["color"] = powerOnColor;
    json["speed"] = powerOnSpeed;
    json["brightness"] = powerOnBrightness;
    //json["blynk_token"] = blynk_token;
    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("failed to open config file for writing");
    }

    json.printTo(Serial);
    json.printTo(configFile);
    configFile.close();
    //end save
  }

  if ((false)||(auto_cycle && (now - auto_last_change > 10000))) { // cycle effect mode every 10 seconds
    uint8_t next_mode = (ws2812fx.getMode() + 1) % ws2812fx.getModeCount();
    if(sizeof(myModes) > 0) { // if custom list of modes exists
      for(uint8_t i=0; i < sizeof(myModes); i++) {
        if(myModes[i] == ws2812fx.getMode()) {
          next_mode = ((uint8_t)(i + 1) < sizeof(myModes)) ? myModes[i + 1] : myModes[0];
          break;
        }
      }
    } 
    ws2812fx.setMode(next_mode);
    Serial.print("mode is "); Serial.println(ws2812fx.getModeName(ws2812fx.getMode()));
    auto_last_change = now;
  }
}

/*
 * Build <li> string for all modes.
 */
void modes_setup() {
  modes = "";
  uint8_t num_modes = sizeof(myModes) > 0 ? sizeof(myModes) : ws2812fx.getModeCount();
  for(uint8_t i=0; i < num_modes; i++) {
    uint8_t m = sizeof(myModes) > 0 ? myModes[i] : i;
    modes += "<li><a href='#' class='m' id='";
    modes += m;
    modes += "'>";
    modes += ws2812fx.getModeName(m);
    modes += "</a></li>";
  }
}

/* #####################################################
#  Webserver Functions
##################################################### */

void srv_handle_not_found() {
  server.send(404, "text/plain", "File Not Found");
}

void srv_handle_index_html() {
  server.send_P(200,"text/html", index_html);
}

void srv_handle_main_js() {
  server.send_P(200,"application/javascript", main_js);
}

void srv_handle_modes() {
  server.send(200,"text/plain", modes);
}

void srv_handle_set() {
  for (uint8_t i=0; i < server.args(); i++){
    if(server.argName(i) == "c") {
      uint32_t tmp = (uint32_t) strtol(server.arg(i).c_str(), NULL, 16);
      if(tmp >= 0x000000 && tmp <= 0xFFFFFF) {
        ws2812fx.setColor(tmp);
        powerOnColor = ws2812fx.getColor();
        Serial.print("color is "); Serial.println(powerOnColor);
      }
    }

    if(server.argName(i) == "m") {
      uint8_t tmp = (uint8_t) strtol(server.arg(i).c_str(), NULL, 10);
      ws2812fx.setMode(tmp % ws2812fx.getModeCount());
      powerOnMode = ws2812fx.getMode();
      Serial.print("mode is "); Serial.println(ws2812fx.getModeName(powerOnMode));
    }

    if(server.argName(i) == "b") {
      if(server.arg(i)[0] == '-') {
        ws2812fx.setBrightness(ws2812fx.getBrightness() * 0.8);
      } else if(server.arg(i)[0] == ' ') {
        ws2812fx.setBrightness(min(max(ws2812fx.getBrightness(), 5) * 1.2, 255));
      } else { // set brightness directly
        uint8_t tmp = (uint8_t) strtol(server.arg(i).c_str(), NULL, 10);
        ws2812fx.setBrightness(tmp);
      }
      powerOnBrightness = ws2812fx.getBrightness();
      Serial.print("brightness is "); Serial.println(powerOnBrightness);
    }

    if(server.argName(i) == "s") {
      if(server.arg(i)[0] == '-') {
        ws2812fx.setSpeed(max(ws2812fx.getSpeed(), 5) * 1.2);
      } else {
        ws2812fx.setSpeed(ws2812fx.getSpeed() * 0.8);
      }
      powerOnSpeed = ws2812fx.getSpeed();
      Serial.print("speed is "); Serial.println(powerOnSpeed);
    }

    if(server.argName(i) == "a") {
      if(server.arg(i)[0] == '-') {
        auto_cycle = false;
      } else {
        auto_cycle = true;
        auto_last_change = 0;
      }
    }

    if(server.argName(i) == "d") {
      if(server.arg(i)[0] == '-') {
        Serial.print("Pressed reserv button");
      } else {
        shouldSaveConfig = true;
        Serial.print("Command should Save Config");
      }
    }

  }
  server.send(200, "text/plain", "OK");
}

