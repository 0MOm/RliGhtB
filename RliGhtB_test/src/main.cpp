#include <Arduino.h>
//#include <ESP8266WiFi.h>
//#include <WiFiClient.h>
//#include <ESP8266WebServer.h>
//#include <ESP8266mDNS.h>
//#include <WiFiUdp.h>
//#include <FS.h>

#include <GyverButton.h>
//#include <string.h>

#include "lbox.cpp"

#define BUTTON_PIN D3 //NODE MCU Flash button connect to PIN0 - D3

#define DBG_OUTPUT_PORT Serial

Lbox lbox = Lbox(PIN, 6);

GButton button = GButton(BUTTON_PIN);

uint32_t mode = 0;

void setup(void) {
  DBG_OUTPUT_PORT.begin(115200);
  DBG_OUTPUT_PORT.print("\nPROGRAMM RUN!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
  DBG_OUTPUT_PORT.setDebugOutput(true);
//Button initialisation
  button.setDebounce(50);        // настройка антидребезга (по умолчанию 80 мс)
  button.setTimeout(500);        // настройка таймаута на удержание (по умолчанию 500 мс)
  button.setStepTimeout(500);    // 
}

void loop(void) {

  //lbox.setBoxColor(0,255,0,0);
  // handle buttons
  button.tick();

  if (button.isStep()){
    lbox.inc_brightness(10);
  }

  if (button.isDouble()){
    lbox.set_brightness(0);
  }

  if (button.isClick())
  {
    mode++;
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

