#include <Adafruit_NeoPixel.h>

#define PIN D6

class Lbox {
  Adafruit_NeoPixel strip = Adafruit_NeoPixel(24, PIN, NEO_GRB + NEO_KHZ800); //lBoxCount * LED_COUNT
  uint8_t lBoxCount;
  uint8_t commonBrightness;
  const uint8_t LED_COUNT = 4; // count LED in one lBox
  const uint8_t MAX_BRIGHTNESS = 255;
  const uint8_t MIN_BRIGHTNESS = 10;
  /*
  -       -       - 
  - 3   2 - 7   6 - 11  10
  -       -       -
  - 0   1 - 4   5 - 8   9
  -       -       -
  +0,+3,+2,+1
  */
  //uint16_t ind[12] = {0,3,2,1,4,7,6,5,8,11,10,9};
  uint32_t i, j;
  unsigned long ms = 0;            // tracking milliseconds
  
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

  public:
    Lbox(uint8_t port, uint8_t count, uint8_t bright=30)
    {
      lBoxCount = count;
      commonBrightness = bright;
      //strip = Adafruit_NeoPixel(lBoxCount * LED_COUNT, port, NEO_GRB + NEO_KHZ800);
      strip.setBrightness(commonBrightness);
      strip.begin();
      strip.show(); // Initialize all pixels to 'off'
      strip.clear();

      for (int i = 0; i < count; ++i)
      {
          setBoxColor(i, 255, 255, 255);
          delay(500);
      }
      delay(1000);
    }

    uint8_t get_brightness()
    {
      return commonBrightness;
    }

    void set_brightness(uint8_t bright)
    {
      if (bright < MIN_BRIGHTNESS) { commonBrightness = MIN_BRIGHTNESS; }
      else if (bright > MAX_BRIGHTNESS) { commonBrightness = MAX_BRIGHTNESS; }
      else { commonBrightness = bright; }
      strip.setBrightness(commonBrightness);
    }

    void inc_brightness(uint8_t n){
      if ((commonBrightness + n) > MAX_BRIGHTNESS) { commonBrightness = MAX_BRIGHTNESS; }
      else { commonBrightness +=n; }
      set_brightness(commonBrightness);
    }

    void dec_brightness(uint8_t n){
      if ((commonBrightness - n) < MIN_BRIGHTNESS) { commonBrightness = MIN_BRIGHTNESS; }
      else { commonBrightness-=n; }
      set_brightness(commonBrightness);
    }

    void setBoxColor(uint8_t n, uint8_t r, uint8_t g, uint8_t b)
    {
      strip.setPixelColor(LED_COUNT*n+0, r, g, b);
      strip.setPixelColor(LED_COUNT*n+3, r, g, b);
      strip.setPixelColor(LED_COUNT*n+2, r, g, b);
      strip.setPixelColor(LED_COUNT*n+1, r, g, b);
      strip.show();
    }
    void setBoxColor(uint8_t n, uint32_t color)
    {
      strip.setPixelColor(LED_COUNT*n+0, color);
      strip.setPixelColor(LED_COUNT*n+3, color);
      strip.setPixelColor(LED_COUNT*n+2, color);
      strip.setPixelColor(LED_COUNT*n+1, color);
      strip.show();
    }

    void rainbowBoxMode(unsigned long interval)
    {
      ms = millis();
      if ( (ms % interval) == 0) {
        if (i < lBoxCount) {
          setBoxColor(i, Wheel(((i * 256 / lBoxCount*30) + j) & 255));
          i++;
        }
        else
        {
          //strip.show();
          i = 0;
          j++;
          if (j > 256 * 5 ) {
            j = 0;
          }
        }
      }
    }

    void setLineColor(uint8_t n, uint8_t r, uint8_t g, uint8_t b)
    {
      if ((n%2) == 0)
      {
        strip.setPixelColor(LED_COUNT*n+0, r, g, b);
        strip.setPixelColor(LED_COUNT*n+3, r, g, b);
        strip.show();
      } 
      else
      {
        strip.setPixelColor(LED_COUNT*n+2, r, g, b);
        strip.setPixelColor(LED_COUNT*n+1, r, g, b);
        strip.show();
      }
    }

    void setLineColor(uint8_t n, uint32_t color)
    {
      uint8_t c;
      c = n / 2;
      if ((n%2) == 0)
      {
        strip.setPixelColor(LED_COUNT*c+0, color);
        strip.setPixelColor(LED_COUNT*c+3, color);
        strip.show();
      } 
      else
      {
        strip.setPixelColor(LED_COUNT*c+2, color);
        strip.setPixelColor(LED_COUNT*c+1, color);
        strip.show();
      }
    }

    void rainbowLineMode(unsigned long interval)
    {
      ms = millis();
      if ( (ms % interval) == 0) {
        if (i < (lBoxCount*2)) {
          setLineColor(i, Wheel(((i * 256 / (lBoxCount*2) + j) & 255)));
          i++;
        }
        else
        {
          //strip.show();
          i = 0;
          j++;
          if (j > 256 * 5 ) {
            j = 0;
          }
        }
      }
    }

    void rainbowLine2Mode(unsigned long interval)
    {
      ms = millis();
      if ( (ms % interval) == 0) {
        if (i < (lBoxCount*2)) {
          setLineColor(i, Wheel(((i * 256 / (lBoxCount*20) + j) & 255)));
          i++;
        }
        else
        {
          //strip.show();
          i = 0;
          j++;
          if (j > 256 * 5 ) {
            j = 0;
          }
        }
      }
    }

    void rainbowLine3Mode(unsigned long interval)
    {
      ms = millis();
      if ( (ms % interval) == 0) {
        if (i < (lBoxCount*2)) {
          setLineColor(i, Wheel(((i * 256 / 256 + j) & 255)));
          i++;
        }
        else
        {
          //strip.show();
          i = 0;
          j++;
          if (j > 256 * 5 ) {
            j = 0;
          }
        }
      }
    }


};


