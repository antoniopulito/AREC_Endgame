#include "Arduino.h"
#include "SoftwareSerial.h"
#include "DFRobotDFPlayerMini.h"

#define switcher 5

SoftwareSerial mySoftwareSerial(10, 11); // RX, TX
DFRobotDFPlayerMini myDFPlayer;
void printDetail(uint8_t type, int value);
int songFlip = -1;
int lastSwitch = HIGH;

void setup() {
  mySoftwareSerial.begin(9600);
  Serial.begin(115200);

  if (!myDFPlayer.begin(mySoftwareSerial)) {  //Use softwareSerial to communicate with mp3.
    Serial.println(F("Unable to begin:"));
    Serial.println(F("1.Please recheck the connection!"));
    Serial.println(F("2.Please insert the SD card!"));
    while(true);
  }
  Serial.println(F("DFPlayer Mini online."));
  Serial.println(myDFPlayer.readFileCounts());

  myDFPlayer.volume(12);  //Set volume value. From 0 to 30

  pinMode(switcher, INPUT_PULLUP);
  myDFPlayer.play(1);
  delay(15000);
  myDFPlayer.stop();
  delay(3000);
  myDFPlayer.play(6);
  delay(3000);
  myDFPlayer.play(2);
  delay(4000);
  myDFPlayer.play(3);
  delay(4000);
  myDFPlayer.play(4);
  delay(1500);
  myDFPlayer.stop();
  delay(3000);
  myDFPlayer.play(5);
  delay(1500);
  myDFPlayer.stop();
}

void loop() {
//  if (digitalRead(switcher) == HIGH) {
//      if (songFlip == -1){
//         Serial.println("Song1");
//         myDFPlayer.play(1);  //Play the first mp3
//      } else {
//        Serial.println("Song2");
//        myDFPlayer.play(2);  //Play the second mp3
//      }
//      lastSwitch = HIGH;
//  } else {
//    myDFPlayer.stop();
//    if (lastSwitch == HIGH){
//      songFlip = songFlip * -1;
//    }
//    lastSwitch = LOW;
//  }
}



void printDetail(uint8_t type, int value){
  switch (type) {
    case TimeOut:
      Serial.println(F("Time Out!"));
      break;
    case WrongStack:
      Serial.println(F("Stack Wrong!"));
      break;
    case DFPlayerCardInserted:
      Serial.println(F("Card Inserted!"));
      break;
    case DFPlayerCardRemoved:
      Serial.println(F("Card Removed!"));
      break;
    case DFPlayerCardOnline:
      Serial.println(F("Card Online!"));
      break;
    case DFPlayerPlayFinished:
      Serial.print(F("Number:"));
      Serial.print(value);
      Serial.println(F(" Play Finished!"));
      break;
    case DFPlayerError:
      Serial.print(F("DFPlayerError:"));
      switch (value) {
        case Busy:
          Serial.println(F("Card not found"));
          break;
        case Sleeping:
          Serial.println(F("Sleeping"));
          break;
        case SerialWrongStack:
          Serial.println(F("Get Wrong Stack"));
          break;
        case CheckSumNotMatch:
          Serial.println(F("Check Sum Not Match"));
          break;
        case FileIndexOut:
          Serial.println(F("File Index Out of Bound"));
          break;
        case FileMismatch:
          Serial.println(F("Cannot Find File"));
          break;
        case Advertise:
          Serial.println(F("In Advertise"));
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }
}
