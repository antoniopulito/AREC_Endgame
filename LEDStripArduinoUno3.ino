#include <Adafruit_NeoPixel.h>

#define PIN      6
#define N_LEDS 15
#define switchReed 4
//int status = 0; //0 = Red, 1 = yellow, 2 = green
int blinkSwitch = 1;
boolean redCheck = false;
int previousSwitch = 1; //1 = high, 0 = low

//Time data
unsigned long redStartTime = 0;
unsigned long greenStartTime = 0;
unsigned long currentTime = 0;

//Inputs
unsigned long redCooldownTime = 5; //Seconds 5400
unsigned long greenCooldownTime = 900; //Seconds
unsigned long redBlinkTime = 20;  //Seconds
unsigned long greenBlinkTime = 10; //Seconds

Adafruit_NeoPixel strip = Adafruit_NeoPixel(N_LEDS, PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  strip.begin();
  pinMode(switchReed, INPUT);
  Serial.begin(9600);
  redCooldownTime = redCooldownTime * 1000;
  greenCooldownTime = greenCooldownTime * 1000;
  chase(strip.Color(255, 255, 0)); // Yellow
}

void loop() {
  currentTime = millis();
  
  //Check if RED
  if (digitalRead(switchReed)==HIGH){
    strip.clear();

      //Check if cooldown is done
      if ((currentTime - redStartTime) > redCooldownTime) {
        for (int i=0; i<redBlinkTime; i=i+1){
          delay(1000);
          greenStartTime = millis();
          if ( digitalRead(switchReed)==LOW){
            return;
          }
          if (i==0){
            chase(strip.Color(255, 0, 0)); // Red
            chaseReverse(strip.Color(255, 0, 0)); // Red
            chase(strip.Color(255, 0, 0)); // Red
          }
        }
      }

  //Check if YELLOW or GREEN
  } else {
    //Green
    if ((currentTime - greenStartTime) > greenCooldownTime) {
      strip.clear();
      for (int i=0; i<greenBlinkTime; i=i+1){
        delay(1000);
        redStartTime = millis();
        if ( digitalRead(switchReed)==HIGH){
          return;
        }
        if (i==0){
          chase(strip.Color(0, 255, 0)); // Green
          chase(strip.Color(0, 255, 0)); // Green
        }
      }
    } else {
      //Yellow
      breatheBlink(strip.Color(22, 35, 75)); // Yellow
    }
  }
}

static void chase(uint32_t c) {
  for(uint16_t i=0; i<strip.numPixels()+4; i++) {
      strip.setPixelColor(i  , c); // Draw new pixel
      strip.setPixelColor(i-4, 0); // Erase pixel a few steps back
      strip.show();
      delay(75);
  }
}

static void chaseReverse(uint32_t c) {
  for(uint16_t i=strip.numPixels()+4; i>0; i--) {
      strip.setPixelColor(i-5, c); // Draw new pixel
      strip.setPixelColor(i-1, 0); // Erase pixel a few steps back
      strip.show();
      delay(75);
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
