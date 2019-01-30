//#include <ESP8266WiFi.h>
//#include <WiFiUdp.h>

#include <Arduino.h>
#include "GyverButton.h"
//#include <string.h>

#include "lbox.cpp"
//#include "udp.h"
#define BUTTON_PIN D3 //NODE MCU Flash button connect to PIN0 - D3

#define DBG_OUTPUT_PORT Serial

Lbox lbox = Lbox(PIN, 6);

GButton button = GButton(BUTTON_PIN);

uint32_t mode = 0;
/*
//////////
#define MASTER // Comment this define to compile sketch on slave device

const int8_t ledPin = LED_BUILTIN;

const char* const ssid = "Keenetic-1069"; // Your network SSID (name)
const char* const pass = "RxrrKy6e"; // Your network password

const uint16_t localPort = 54321; // Local port to listen for UDP packets

#ifdef MASTER
const uint32_t stepDuration = 2000;

IPAddress broadcastAddress;
#endif

WiFiUDP udp;

bool sendPacket(const IPAddress& address, const uint8_t* buf, uint8_t bufSize);
void receivePacket();


void UDPsetup() {
  Serial.begin(115200);
  Serial.println();

  pinMode(ledPin, OUTPUT);

  Serial.print(F("Connecting to "));
  Serial.println(ssid);
  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(ledPin, ! digitalRead(ledPin));
    delay(500);
    Serial.print('.');
  }
  digitalWrite(ledPin, HIGH);
  Serial.println();

  Serial.println(F("WiFi connected"));
  Serial.println(F("IP address: "));
  Serial.println(WiFi.localIP());

#ifdef MASTER
  broadcastAddress = (uint32_t)WiFi.localIP() | ~((uint32_t)WiFi.subnetMask());
  Serial.print(F("Broadcast address: "));
  Serial.println(broadcastAddress);
#endif

  Serial.println(F("Starting UDP"));
  udp.begin(localPort);
  Serial.print(F("Local port: "));
  Serial.println(udp.localPort());
}

void UDPloop() {
#ifdef MASTER
  static uint32_t nextTime = 0;

  if (millis() >= nextTime) {
    bool led = ! digitalRead(ledPin);
    digitalWrite(ledPin, led);
    Serial.print(F("Turn led "));
    Serial.println(led ? F("off") : F("on"));
    if (! sendPacket(broadcastAddress, (uint8_t*)&led, sizeof(led)))
      Serial.println(F("Error sending broadcast UDP packet!"));
    nextTime = millis() + stepDuration;
  }
#endif

  if (udp.parsePacket())
    receivePacket();

  delay(1);
}

bool sendPacket(const IPAddress& address, const uint8_t* buf, uint8_t bufSize) {
  udp.beginPacket(address, localPort);
  udp.write(buf, bufSize);
  return (udp.endPacket() == 1);
}

void receivePacket() {
  bool led;

  udp.read((uint8_t*)&led, sizeof(led));
  udp.flush();
#ifdef MASTER
  if (udp.destinationIP() != broadcastAddress) {
    Serial.print(F("Client with IP "));
    Serial.print(udp.remoteIP());
    Serial.print(F(" turned led "));
    Serial.println(led ? F("off") : F("on"));
  } else {
    Serial.println(F("Skip broadcast packet"));
  }
#else
  digitalWrite(ledPin, led);
  led = digitalRead(ledPin);
  Serial.print(F("Turn led "));
  Serial.println(led ? F("off") : F("on"));
  if (! sendPacket(udp.remoteIP(), (uint8_t*)&led, sizeof(led)))
    Serial.println(F("Error sending answering UDP packet!"));
#endif
}


/////////

*/

void setup(void) {
  DBG_OUTPUT_PORT.begin(115200);
  DBG_OUTPUT_PORT.print("\nPROGRAMM RUN!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
  DBG_OUTPUT_PORT.setDebugOutput(true);
//Button initialisation
  button.setDebounce(50);        // настройка антидребезга (по умолчанию 80 мс)
  button.setTimeout(500);        // настройка таймаута на удержание (по умолчанию 500 мс)
  button.setStepTimeout(500);    // 

//  UDPsetup();
}

void loop(void) {
  //lbox.setBoxColor(0,255,0,0);
  // handle buttons
  
  //UDPloop();
  
  button.tick();

  if (button.isStep()){
    lbox.inc_brightness(10);
  }

  if (button.hasClicks()) {
    if(uint32_t cnt = button.getClicks() == 1) {
      lbox.toggle();
    }
    else if (cnt == 2) {
      mode++;
    }
  }
  
  switch (mode) {
    case 0:
      lbox.setBoxColor(0,255,255,255);
      lbox.setBoxColor(1,255,255,255);
      lbox.setBoxColor(2,255,255,255);
    break;
    case 1:
      lbox.setBoxColor(0,255,0,0);
      lbox.setBoxColor(1,0,255,0);
      lbox.setBoxColor(2,0,0,255);
    break;
    case 2:
      lbox.rainbowBoxMode(50);
    break;
    case 3:
      lbox.rainbowLineMode(50);
    break;
    case 4:
      lbox.rainbowLine2Mode(50);
    break;
    case 5:
      lbox.rainbowLine3Mode(50);
    break;
    case 6:
      lbox.rainbowLineMode(10);
    break;
    case 7:
      lbox.rainbowLine2Mode(10);
    break;
    case 8:
      lbox.rainbowLine3Mode(10);
    break;

    default:
      mode = 0;
  }
  
}

