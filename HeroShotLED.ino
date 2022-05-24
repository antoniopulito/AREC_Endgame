#include <Adafruit_NeoPixel.h>

#define PIN      2
#define N_LEDS 257
//int status = 0; //0 = Red, 1 = yellow, 2 = green
int blinkSwitch = 1;
boolean redCheck = false;
int previousSwitch = 1; //1 = high, 0 = low


Adafruit_NeoPixel strip = Adafruit_NeoPixel(N_LEDS, PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  strip.begin();
  Serial.begin(9600);
  chase(strip.Color(255, 255, 0)); // Yellow
}

void loop() {
  onlights(strip.Color(255, 255, 255)); // White
  indication(strip.Color(255, 0, 0)); // Red
}

static void chase(uint32_t c) {
  for(uint16_t i=0; i<strip.numPixels()+4; i++) {
      strip.setPixelColor(i  , c); // Draw new pixel
      strip.setPixelColor(i-4, 0); // Erase pixel a few steps back
      strip.show();
      delay(5);
  }
}


static void onlights(uint32_t c) {
  for(uint16_t i=0; i<41; i++) {
      strip.setPixelColor(i  , c); // Draw new pixel
      strip.show();
  }
  for(uint16_t i=70; i<111; i++) {
      strip.setPixelColor(i  , c); // Draw new pixel
      strip.show();
  }
}

static void indication(uint32_t c) {
  for(uint16_t i=43; i<=48; i++) {
      strip.setPixelColor(i  , c); // Draw new pixel
      strip.show();
  }
  for(uint16_t i=62; i<=67; i++) {
      strip.setPixelColor(i  , c); // Draw new pixel
      strip.show();
  }
}

static void chaseReverse(uint32_t c) {
  for(uint16_t i=strip.numPixels()+4; i>0; i--) {
      strip.setPixelColor(i-5, c); // Draw new pixel
      strip.setPixelColor(i-1, 0); // Erase pixel a few steps back
      strip.show();
      delay(5);
      Serial.println("We loopin");
  }
}

static void breatheBlink(uint32_t c) {
  if (blinkSwitch == 1) {
     strip.setPixelColor(0  , c); // Draw new pixel
     strip.setPixelColor(1  , 0); // Draw new pixel
     strip.setPixelColor(2  , c); // Draw new pixel
     strip.setPixelColor(3  , 0); // Draw new pixel
     strip.setPixelColor(4  , c); // Draw new pixel
     strip.setPixelColor(5  , 0); // Draw new pixel
     strip.setPixelColor(6  , c); // Draw new pixel
     strip.setPixelColor(7  , 0); // Draw new pixel
     strip.setPixelColor(8  , c); // Draw new pixel
     strip.setPixelColor(9  , 0); // Draw new pixel
     strip.setPixelColor(10  , c); // Draw new pixel
     strip.setPixelColor(11  , 0); // Draw new pixel
     strip.setPixelColor(12  , c); // Draw new pixel
     strip.setPixelColor(13  , 0); // Draw new pixel
     strip.setPixelColor(14  , c); // Draw new pixel
     strip.setPixelColor(15  , 0); // Draw new pixel
  } else {
     strip.setPixelColor(0  , 0); // Draw new pixel
     strip.setPixelColor(1  , c); // Draw new pixel
     strip.setPixelColor(2  , 0); // Draw new pixel
     strip.setPixelColor(3  , c); // Draw new pixel
     strip.setPixelColor(4  , 0); // Draw new pixel
     strip.setPixelColor(5  , c); // Draw new pixel
     strip.setPixelColor(6  , 0); // Draw new pixel
     strip.setPixelColor(7  , c); // Draw new pixel
     strip.setPixelColor(8  , 0); // Draw new pixel
     strip.setPixelColor(9  , c); // Draw new pixel
     strip.setPixelColor(10  , 0); // Draw new pixel
     strip.setPixelColor(11  , c); // Draw new pixel
     strip.setPixelColor(12  , 0); // Draw new pixel
     strip.setPixelColor(13  , c); // Draw new pixel
     strip.setPixelColor(14  , 0); // Draw new pixel
     strip.setPixelColor(15  , c); // Draw new pixel
  }
     blinkSwitch = blinkSwitch * -1;
     strip.show();
     delay(1000);
}
