/*
  EspFeeder -- Web Server Enabled Pet Feeder
  Base code is https://github.com/esp8266/Arduino/blob/master/libraries/ESP8266WebServer/examples/FSBrowser/FSBrowser.ino
  Base Code Copyright (c) 2015 Hristo Gochkov. All rights reserved. LGPL
  Code is highly modifed.
*/

/* Requirements:
  Arduino-1.6.11
  ESP8266/Arduino :Additional Boards Manager URL: http://arduino.esp8266.com/stable/package_esp8266com_index.json
  ESP8266FS plugin, installed in tools https://github.com/esp8266/arduino-esp8266fs-plugin/releases/download/0.2.0/ESP8266FS-0.2.0.zip
  Bounce2 Library, installed in library https://github.com/thomasfredericks/Bounce2/releases/tag/V2.21
  ArduinoJson Library, install in libarry https://github.com/bblanchon/ArduinoJson/releases/tag/v5.6.7
      (the libraries can be installed with the library manager instead)

  Don't forget to restart the Arduino IDE after installing these things.


  Set your esp settings.. the board, program method, flash size and spiffs size.

  This uses the SPIFFS file system.  So we need to load that in your esp-xx first.
  Upload the contents of the data folder with MkSPIFFS Tool ("ESP8266 Sketch Data Upload" in Tools menu in Arduino IDE)

  Then compile and upload the .ino.

*/

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <FS.h>

#include <ArduinoJson.h>
#include <Bounce2.h>
#include <string.h>

#include <Adafruit_NeoPixel.h>

#define BUTTON_PIN D3 //NODE MCU Flash button connect to PIN0 - D3

Bounce button = Bounce();

#define DBG_OUTPUT_PORT Serial

const char* configFile = "/config.json";   // The config file name

// Note that these are the default values if no /config.json exists, or items are missing from it.

char ssid[31] = { "Keenetic-1069" };               // This is the access point to connect to
char password[31] = { "RxrrKy6e" };       // And its password
char host[31] = { "Light" };          // The host name for .local (mdns) hostname

int offsetGMT = -18000;       // Local timezone offset in seconds
char offsetGMTstring[10] = { "-18000" };

int apMode = false;           // Are we in Acess Point mode?

char timeServer[31] = { "0.pool.ntp.org" };   // the NTP timeServer to use
char getTime[10] = { "02:01" };               // what time to resync with the NTP server
int getHour = 2;                              // parsed hour of above
int getMinute = 1;                            // parsed minute of above
char resetTime[10] = { "00:00" };             // what time to auto reset (note the servos may unlatch) 00:00=no reset
int resetHour = 0;                            // parsed hour of above
int resetMinute = 0;                          // parsed minute of above

#define PIN D6
uint8_t col[3][6]; //r,g,b color for 10 LED
bool rainbowMode;
uint16_t i, j;
//unsigned long currentMillis = 0;
//unsigned long previousMillis = 0;
const long interval = 50;
char temp[400];
// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(6, PIN, NEO_GRB + NEO_KHZ800);

ESP8266WebServer server(80);  // The Web Server
File fsUploadFile;            //holds the current upload when files are uploaded (see edit.htm)

WiFiUDP udp;

IPAddress timeServerIP;
unsigned int localPort = 2390; // local port to listen for UDP packets
const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets
// A UDP instance to let us send and receive packets over UDP

time_t timeNow = 0;       // current time is stored here
time_t ms = 0;            // tracking milliseconds
int lastMinute = -1;      // tracking if minute changed
int gettingNtp = false;
int flagRestart = false;

//format bytes
String formatBytes(size_t bytes) {
  if (bytes < 1024) {
    return String(bytes) + "B";
  } else if (bytes < (1024 * 1024)) {
    return String(bytes / 1024.0) + "KB";
  } else if (bytes < (1024 * 1024 * 1024)) {
    return String(bytes / 1024.0 / 1024.0) + "MB";
  } else {
    return String(bytes / 1024.0 / 1024.0 / 1024.0) + "GB";
  }
}

String getContentType(String filename) {
  if (server.hasArg("download")) return "application/octet-stream";
  else if (filename.endsWith(".htm")) return "text/html";
  else if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".json")) return "text/json";
  else if (filename.endsWith(".png")) return "image/png";
  else if (filename.endsWith(".gif")) return "image/gif";
  else if (filename.endsWith(".jpg")) return "image/jpeg";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  else if (filename.endsWith(".xml")) return "text/xml";
  else if (filename.endsWith(".pdf")) return "application/x-pdf";
  else if (filename.endsWith(".zip")) return "application/x-zip";
  else if (filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}

bool handleFileRead(String path) {
  DBG_OUTPUT_PORT.print(timeNow);
  DBG_OUTPUT_PORT.println(" handleFileRead: " + path);
  if (path.endsWith("/")) path += apMode ? "setup.html" : "index.html";

  String contentType = getContentType(path);
  String pathWithGz = path + ".gz";
  if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)) {
    if (SPIFFS.exists(pathWithGz))
      path += ".gz";
    File file = SPIFFS.open(path, "r");
    size_t sent = server.streamFile(file, contentType);
    file.close();
    return true;
  }
  return false;
}

void handleFileUpload() {
  if (server.uri() != "/edit") return;
  HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    String filename = upload.filename;
    if (!filename.startsWith("/")) filename = "/" + filename;
    DBG_OUTPUT_PORT.print("handleFileUpload Name: "); DBG_OUTPUT_PORT.println(filename);
    fsUploadFile = SPIFFS.open(filename, "w");
    filename = String();
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    //DBG_OUTPUT_PORT.print("handleFileUpload Data: "); DBG_OUTPUT_PORT.println(upload.currentSize);
    if (fsUploadFile)
      fsUploadFile.write(upload.buf, upload.currentSize);
  } else if (upload.status == UPLOAD_FILE_END) {
    if (fsUploadFile)
      fsUploadFile.close();
    DBG_OUTPUT_PORT.print("handleFileUpload Size: "); DBG_OUTPUT_PORT.println(upload.totalSize);
    if (upload.filename == configFile)
    {
      loadConfig();
    }
  }
}

void handleFileDelete() {
  if (server.args() == 0) return server.send(500, "text/plain", "BAD ARGS");
  String path = server.arg(0);
  DBG_OUTPUT_PORT.println("handleFileDelete: " + path);
  if (path == "/")
    return server.send(500, "text/plain", "BAD PATH");
  if (!SPIFFS.exists(path))
    return server.send(404, "text/plain", "FileNotFound");
  SPIFFS.remove(path);
  server.send(200, "text/plain", "");
  path = String();
}

void handleFileCreate() {
  if (server.args() == 0)
    return server.send(500, "text/plain", "BAD ARGS");
  String path = server.arg(0);
  DBG_OUTPUT_PORT.println("handleFileCreate: " + path);
  if (path == "/")
    return server.send(500, "text/plain", "BAD PATH");
  if (SPIFFS.exists(path))
    return server.send(500, "text/plain", "FILE EXISTS");
  File file = SPIFFS.open(path, "w");
  if (file)
    file.close();
  else
    return server.send(500, "text/plain", "CREATE FAILED");
  server.send(200, "text/plain", "");
  path = String();
}

void handleFileList() {
  if (!server.hasArg("dir")) {
    server.send(500, "text/plain", "BAD ARGS");
    return;
  }

  String path = server.arg("dir");
  DBG_OUTPUT_PORT.print(timeNow);
  DBG_OUTPUT_PORT.println(" handleFileList: " + path);
  Dir dir = SPIFFS.openDir(path);
  path = String();

  String output = "[";
  while (dir.next()) {
    File entry = dir.openFile("r");
    if (output != "[") output += ',';
    bool isDir = false;
    output += "{\"type\":\"";
    output += (isDir) ? "dir" : "file";
    output += "\",\"name\":\"";
    output += String(entry.name()).substring(1);
    output += "\"}";
    entry.close();
  }

  output += "]";
  server.send(200, "text/json", output);
}

void loadConfig()
{
  if (SPIFFS.exists(configFile))
  {
    File file = SPIFFS.open(configFile, "r");
    char json[500];
    memset(json, 0, sizeof(json));
    file.readBytes(json, sizeof(json));
    file.close();
    StaticJsonBuffer<500> jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(json);
    if (!root.success()) {
      DBG_OUTPUT_PORT.println("json parse of configFile failed.");
    }
    else
    {
      if (root.containsKey("ssid")) strncpy(ssid, root["ssid"], 30);
      if (root.containsKey("password")) strncpy(password, root["password"], 30);
      if (root.containsKey("host")) strncpy(host, root["host"], 30);
      if (root.containsKey("timeServer")) strncpy(timeServer, root["timeServer"], 30);
      if (root.containsKey("getTime")) strncpy(getTime, root["getTime"], 10);
      if (root.containsKey("resetTime")) strncpy(resetTime, root["resetTime"], 10);
      if (root.containsKey("offsetGMT")) strncpy(offsetGMTstring, root["offsetGMT"], 10);
      offsetGMT = atoi(offsetGMTstring);
      getHour = atoi(getTime);
      if (strchr(getTime, ':')) getMinute = atoi(strchr(getTime, ':') + 1);
      resetHour = atoi(resetTime);
      if (strchr(resetTime, ':')) resetMinute = atoi(strchr(resetTime, ':') + 1);

      DBG_OUTPUT_PORT.printf("Config: host: %s ssid: %s timeServer: %s\n", host, ssid, timeServer);
      DBG_OUTPUT_PORT.printf("getTime: %s %d %d resetTime:%s %d %d offsetGMT:%d\n",
                             getTime, getHour, getMinute, resetTime, resetHour, resetMinute, offsetGMT);
    }
  }
  else
  {
    DBG_OUTPUT_PORT.printf("config file: %s not found\n", configFile);
  }
}

void getNTP()
{

  if (gettingNtp) return;
  time_t failms = millis();
  time_t ims = millis();
  int tries = 0;
  gettingNtp = true;
  while (gettingNtp)
  {
    tries++;
    if (timeNow > 100) // we have successfully got the time before
      if ((millis() - failms) > 60 * 1000 ) // 1 minutes
      {
        gettingNtp = false;
        return;  // lets just foreget it
      }
    if ((millis() - failms) > 5 * 60 * 1000) // 5 minutes
    {
      ESP.restart();
    }
    if (timeServerIP == INADDR_NONE || (tries % 3) == 1)
    {
      //get a random server from the pool
      DBG_OUTPUT_PORT.print("\nLooking up:");
      DBG_OUTPUT_PORT.println(timeServer);

      WiFi.hostByName(timeServer, timeServerIP);
      DBG_OUTPUT_PORT.print("timeServer IP:");
      DBG_OUTPUT_PORT.println(timeServerIP);

      if (timeServerIP == INADDR_NONE)
      {
        DBG_OUTPUT_PORT.println("bad IP, try again");
        delay(1000);
        continue;
      }
    }
    DBG_OUTPUT_PORT.println("sending NTP packet...");
    // set all bytes in the buffer to 0
    memset(packetBuffer, 0, NTP_PACKET_SIZE);
    // Initialize values needed to form NTP request
    // (see URL above for details on the packets)
    packetBuffer[0] = 0b11100011;   // LI, Version, Mode
    packetBuffer[1] = 0;     // Stratum, or type of clock
    packetBuffer[2] = 6;     // Polling Interval
    packetBuffer[3] = 0xEC;  // Peer Clock Precision
    // 8 bytes of zero for Root Delay & Root Dispersion
    packetBuffer[12]  = 49;
    packetBuffer[13]  = 0x4E;
    packetBuffer[14]  = 49;
    packetBuffer[15]  = 52;

    // all NTP fields have been given values, now
    // you can send a packet requesting a timestamp:
    udp.beginPacket(timeServerIP, 123); //NTP requests are to port 123
    udp.write(packetBuffer, NTP_PACKET_SIZE);
    udp.endPacket();

    ims = millis();
    while (gettingNtp)
    {
      if ((millis() - ims) > 5000) break; // if > 15 seconds waiting for packet, send packet again (break into outer loop)
      // wait to see if a reply is available
      delay(1000);

      int cb = udp.parsePacket();
      if (!cb) {
        DBG_OUTPUT_PORT.print(".");
      }
      else {
        DBG_OUTPUT_PORT.print("packet received, length=");
        DBG_OUTPUT_PORT.println(cb);
        // We've received a packet, read the data from it
        udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer

        //the timestamp starts at byte 40 of the received packet and is four bytes,
        // or two words, long. First, esxtract the two words:

        unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
        unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
        // combine the four bytes (two words) into a long integer
        // this is NTP time (seconds since Jan 1 1900):
        unsigned long secsSince1900 = highWord << 16 | lowWord;
        DBG_OUTPUT_PORT.print("Seconds since Jan 1 1900 = " );
        DBG_OUTPUT_PORT.println(secsSince1900);

        // now convert NTP time into everyday time:
        DBG_OUTPUT_PORT.print("Unix time = ");
        // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
        const unsigned long seventyYears = 2208988800UL;
        // subtract seventy years:
        unsigned long epoch = secsSince1900 - seventyYears;
        // print Unix time:
        DBG_OUTPUT_PORT.println(epoch);

        // print the hour, minute and second:
        DBG_OUTPUT_PORT.print("The UTC time is ");       // UTC is the time at Greenwich Meridian (GMT)
        DBG_OUTPUT_PORT.print((epoch  % 86400L) / 3600); // print the hour (86400 equals secs per day)
        DBG_OUTPUT_PORT.print(':');
        if ( ((epoch % 3600) / 60) < 10 ) {
          // In the first 10 minutes of each hour, we'll want a leading '0'
          DBG_OUTPUT_PORT.print('0');
        }
        DBG_OUTPUT_PORT.print((epoch  % 3600) / 60); // print the minute (3600 equals secs per minute)
        DBG_OUTPUT_PORT.print(':');
        if ( (epoch % 60) < 10 ) {
          // In the first 10 seconds of each minute, we'll want a leading '0'
          DBG_OUTPUT_PORT.print('0');
        }
        DBG_OUTPUT_PORT.println(epoch % 60); // print the second
        timeNow = epoch;
        gettingNtp = false;
      }
    }
  }


}
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if (WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle() {
  /*
  for (j = 0; j < 256 * 5; j++) { // 5 cycles of all colors on wheel
    for (i = 0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
      server.handleClient();////////////////////////////////////////////////////////////
    }
  */
 ms = millis();
 if( (ms % interval) == 0){
    if (i < strip.numPixels()) {
        strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
        i++;
        
    }
    else
    {
      strip.show();
      i=0;
      j++;
      if (j > 256*5 ) { 
        j=0;
      }
    }
 }

    //currentMillis = millis();
    //if (currentMillis - previousMillis >= interval) {
    //  previousMillis = currentMillis;
    
    //}
  }

void setup(void) {
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'

  apMode = false;
  DBG_OUTPUT_PORT.begin(74880);
  DBG_OUTPUT_PORT.print("\n");
  DBG_OUTPUT_PORT.setDebugOutput(true);

  SPIFFS.begin();
  {
    Dir dir = SPIFFS.openDir("/");
    while (dir.next()) {
      String fileName = dir.fileName();
      size_t fileSize = dir.fileSize();
      DBG_OUTPUT_PORT.printf("FS File: %s, size: %s\n", fileName.c_str(), formatBytes(fileSize).c_str());
    }
    DBG_OUTPUT_PORT.printf("\n");
  }

  loadConfig();

  //WIFI INIT

  WiFi.mode(WIFI_STA);

  time_t wifims = millis();

  DBG_OUTPUT_PORT.printf("Connecting to %s\n", ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    DBG_OUTPUT_PORT.print(".");
    if ((millis() - wifims) > 60 * 1000) // 60 seconds of no wifi connect
    {
      break;
    }
  }
  if (WiFi.status() != WL_CONNECTED)
  {
    DBG_OUTPUT_PORT.println("\ngoing to AP mode ");
    delay(500);
    WiFi.mode(WIFI_AP);
    delay(500);
    apMode = true;
    uint8_t mac[6];
    delay(500);
    WiFi.softAPmacAddress(mac);
    delay(500);
    sprintf(ssid, "EspFeeder_%02x%02x%02x", mac[3], mac[4], mac[5]); // making a nice unique SSID
    DBG_OUTPUT_PORT.print("SoftAP ssid:");
    DBG_OUTPUT_PORT.println(ssid);
    WiFi.softAP(ssid);
    DBG_OUTPUT_PORT.println("");
    DBG_OUTPUT_PORT.print("AP mode. IP address: ");
    DBG_OUTPUT_PORT.println(WiFi.softAPIP());
  }
  else
  {
    DBG_OUTPUT_PORT.println("");
    DBG_OUTPUT_PORT.print("Connected! IP address: ");
    DBG_OUTPUT_PORT.println(WiFi.localIP());
  }

  MDNS.begin(host);
  DBG_OUTPUT_PORT.print("Open http://");
  DBG_OUTPUT_PORT.print(host);
  DBG_OUTPUT_PORT.print(".local/ or http://");
  if (apMode)
  {
    DBG_OUTPUT_PORT.print(WiFi.softAPIP());
  }
  else
  {
    DBG_OUTPUT_PORT.print(WiFi.localIP());
  }
  DBG_OUTPUT_PORT.println("/");

  // NTP init
  if (!apMode)    // if we're in AP Mode we have no internet, so no NTP
  {
    DBG_OUTPUT_PORT.println("Starting UDP for NTP");
    udp.begin(localPort);
    DBG_OUTPUT_PORT.print("Local port: ");
    DBG_OUTPUT_PORT.println(udp.localPort());

    delay(1000);

    getNTP();

  }
  button.attach(BUTTON_PIN);
  button.interval(10);

  //SERVER INIT
  //list directory
  server.on("/list", HTTP_GET, handleFileList);
  //load editor
  server.on("/edit", HTTP_GET, []() {
    if (!handleFileRead("/edit.html")) server.send(404, "text/plain", "FileNotFound");
  });
  //create file
  server.on("/edit", HTTP_PUT, handleFileCreate);
  //delete file
  server.on("/edit", HTTP_DELETE, handleFileDelete);
  //first callback is called after the request has ended with all parsed arguments
  //second callback handles file uploads at that location
  server.on("/edit", HTTP_POST, []() {
    server.send(200, "text/plain", "");
  }, handleFileUpload);

  //called when the url is not defined here
  //use it to load content from SPIFFS
  server.onNotFound([]() {
    if (!handleFileRead(server.uri()))
      server.send(404, "text/plain", "FileNotFound");
  });
  /*
    server.on("/status", HTTP_GET, [](){
      String json = "{";
      //json += String(  "\"servo1\":\"")+String(servo1State?"latched":"unlatched")+String("\"");
      //json += String(", \"servo2\":\"")+String(servo2State?"latched":"unlatched")+String("\"");
      json += String(", \"time\":")+String(timeNow);
      json += "}";
      server.send(200, "text/json", json);
      DBG_OUTPUT_PORT.print("status ");
      DBG_OUTPUT_PORT.println(json);
      json = String();
    });
*/
    server.on("/toggle1", HTTP_GET, [](){
      server.send(200, "text/text", "OK");
      DBG_OUTPUT_PORT.printf("toggle1 \n");
      rainbowMode = !rainbowMode;
    });
    server.on("/toggle2", HTTP_GET, [](){
      server.send(200, "text/text", "OK");
      DBG_OUTPUT_PORT.printf("toggle2 \n");
      rainbowMode = false;
      strip.clear();
      strip.show();
    });
  
  server.on("/restart", HTTP_GET, []() {
    server.send(200, "text/text", apMode ? "Stopping AP, Restarting... to connect to WiFi. Use your browser on your network to reconnect in a minute" : "Restarting.... Wait a minute or so and then refresh.");
    delay(2000);
    ESP.restart();
  });

  server.begin();
  DBG_OUTPUT_PORT.println("HTTP server started");


  i=0;
  j=0;
}

void loop(void) {

  // deal with http server.
  server.handleClient();

  // handle buttons
  button.update();

  if (button.fell())
  {
    rainbowMode = !rainbowMode;
  }
  if (rainbowMode) {
    rainbowCycle();
  }

  // keep track of the time
  if (!apMode && ms != millis())
  {
    ms = millis();
    if ( (ms % 1000) == 0)
    {
      timeNow++;
      time_t t = timeNow + offsetGMT;
      int hour = (t % 86400) / 3600;
      int minute = (t % 3600) / 60;
      if (lastMinute != minute)  // time to check for things to do?
      {
        DBG_OUTPUT_PORT.printf("%d:%02d\n", hour, minute);
        lastMinute = minute;
        if (flagRestart)        // if we flagged a reset
        {
          flagRestart = false;
          ESP.restart();
        }
        if (getHour == hour && getMinute == minute)   // time to resync the time?
        {
          if (!gettingNtp && !apMode) getNTP();
        }
        if (resetHour == hour && resetMinute == minute && resetHour != 0 && resetMinute != 0) // time to reset?
        {
          flagRestart = true;
        }
      }
    }
  }
}
