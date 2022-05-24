// don't open serial monitor while running this pleaseee trust me

#include"pitches.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "SoftwareSerial.h"
#include "DFRobotDFPlayerMini.h"
//#include <DFPlayerMini_Fast.h>


const char* mqtt_subscribe = "command/door1";
const char* mqtt_subscribe_sound = "UI/notifications";
const char* mqtt_subscribe_volume = "UI/volume";
const char* mqtt_topic = "tele/%x/DOOR1";
const char* mqtt_topic_points = "points/%x/DOOR1";

const char* ssid = "AREC_WL";
const char* password = "AREC_WL_PASSWD";
const char* mqtt_server = "192.168.2.101";
char g_window1_mqtt_topic[50];        // MQTT topic for reporting window1 
char g_points_mqtt_topic[50];         // MQTT topic for reporting points 
char g_mqtt_message_buffer[150];    // General purpose buffer for MQTT messages        
const char* mqtt_user = "eric";
const char* mqtt_password = "eric";
const char* mqtt_device_id = "Window_Sensor1";
const boolean mqtt_retain = 0; //set to 0 to resolve reconnect LOP issues
const uint8_t mqtt_qos = 0; //can only be a value of (0, 1, 2)
char vInp13 = 0;
String rx;
String rx_previous;         
int rxLength = 0;
int sensorPin = 14;  
int ledBluePin = 0;
int ledGreenPin = 4;
int ledRedPin = 5;
boolean newmessage = true;

unsigned long tnow, tnextoff, tnextblink, tredchange, tredoff;
const int nextTimeEvent = 5000;               // time to turn green LED off
const int nextRedTimeEvent = 15000;           // time to change red LED from fast blink to slow blink
const int nextRedOff = 45000;                 // time to turn red LED off
const int nextRedBlink_fast = 500;            // red LED fast blink rate
const int nextRedBlink_slow_on = 2000;        // red LED slow blink on time
const int nextRedBlink_slow_off = 3000;       // red LED slow blink off time
char* color = "off";
int ledState=0;

unsigned long tpointcountstart;
const int nextPointLevelTime = 10000;     // time interval of point level change


// SPEAKER SETUP
SoftwareSerial mySoftwareSerial(13, 12); // RX, TX
//DFPlayerMini_Fast myDFPlayer;
DFRobotDFPlayerMini myDFPlayer;
//void printDetail(uint8_t type, int value);
int songFlip = -1;
int lastSwitch = HIGH;
unsigned long tmusicstop;
const int goodMusicIndex = 3;
const int badMusicIndex = 5;
const int nextMusicStop_good = 4000;       
const int nextMusicStop_bad = 2000;       
char* soundonoff_bad = "on";              // change this parameter to mute or unmute the speaker
char* soundonoff_good = "on";              // change this parameter to mute or unmute the speaker
boolean playedsound = false;
char* music = "off";
int volume = 20;                          // value from 0 to 30 

// window sensor D5 (14), Blue D3 (0), Green D2 (4), Red D1 (5)


WiFiClient espClient;
PubSubClient client(espClient);

void setup() {   
  // Set up topic for publishing sensor readings
  sprintf(g_window1_mqtt_topic, mqtt_topic,  ESP.getChipId());
  sprintf(g_points_mqtt_topic, mqtt_topic_points,  ESP.getChipId());                                  
  
  pinMode(ledBluePin, OUTPUT);
  pinMode(ledRedPin, OUTPUT);
  pinMode(ledGreenPin, OUTPUT);
  pinMode(sensorPin, INPUT_PULLUP);
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  pinMode(16, OUTPUT);
  pinMode(2, OUTPUT);
  digitalWrite(16, HIGH);            // turn off on-board blue LED
  digitalWrite(2,HIGH);

  mySoftwareSerial.begin(9600);
    
  if (!myDFPlayer.begin(mySoftwareSerial)) {  //Use softwareSerial to communicate with mp3.
    Serial.println(F("Unable to begin:"));
    Serial.println(F("1.Please recheck the connection!"));
    Serial.println(F("2.Please insert the SD card!"));
    while(true);
  }
  Serial.println(F("DFPlayer Mini online."));

  myDFPlayer.volume(volume);  //Set initial volume value. From 0 to 30
}


/**************************************************************************/
/*
    WiFi & MQTT Setup
*/
/**************************************************************************/

void setup_wifi() {
  delay(10);  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

                                                                                                  
void callback(char* topic, byte* payload, unsigned int length) {\                                                          
  Serial.print("RX: ");
  //Convert and clean up the MQTT payload messsage into a String
  rx = String((char *)payload);                    //Payload is a Byte Array, convert to char to load it into the "String" object 
  rxLength = rx.length();                          //Figure out how long the resulting String object is 
  for (int i = length; i <= rxLength; i++)         //Loop through setting extra characters to null as garbage may be there
  {
    rx.setCharAt(i, ' ');
  }
  rx.trim();                                       //Use the Trim function to finish cleaning up the string   
  Serial.print(rx);                                //Print the recieved message to serial
  Serial.println();

  if ((rx == "0") || (rx == "1") || (rx == "2")) {
    if (rx != rx_previous) {
      playedsound = false;
      newmessage = true;
      tpointcountstart = millis();
    }
    rx_previous = rx;
  }
  
  if (rx == "Full") {
    soundonoff_bad = "on";              // change this parameter to mute or unmute the speaker
    soundonoff_good = "on";
  }
  
  if (rx == "Success") {
    soundonoff_bad = "off";              // change this parameter to mute or unmute the speaker
    soundonoff_good = "on";
  }
  if (rx == "Off") {
    soundonoff_bad = "off";              // change this parameter to mute or unmute the speaker
    soundonoff_good = "off";
  }

  if (rx.toInt() >= 100){
    volume = map(rx.toInt(),100,200,0,25);
    myDFPlayer.volume(volume);
  }
  
}



void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
  Serial.print("Attempting MQTT connection...");
  // Attempt to connect
  if (client.connect(g_window1_mqtt_topic,mqtt_user,mqtt_password)) {
  //needs next line for proper connect to mqtt broker that doesn't allow anonymous
  //if (client.connect(mqtt_device_id, mqtt_user, mqtt_password, mqtt_topic, mqtt_qos, mqtt_retain, mqtt_lwt)) {
    Serial.println("connected");
 
      // ... and resubscribe
      // client.subscribe(g_window1_mqtt_topic);
      client.subscribe(mqtt_subscribe); 
      client.subscribe(mqtt_subscribe_sound); 
      client.subscribe(mqtt_subscribe_volume);                                                          
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


/**************************************************************************/
/*
    Read the door sensor
*/
/**************************************************************************/

void doorRead(void)
{  
  // Perform an action when rx changes value
  if (newmessage == true) {
      //Evaulate the recieved message to do stuff
      if (((rx_previous == "0") && (vInp13 == HIGH)) || ((rx_previous == "1") && (vInp13 == LOW)))
      {
        RGB_color(255,0,0);
        color = "red";
        ledState=1;             // LED is on
        tnow = millis();
        if (playedsound == false) 
        { 
          if (soundonoff_bad == "on") {
            myDFPlayer.play(badMusicIndex);
            music = "on";
            playedsound = true;
            tmusicstop = tnow + nextMusicStop_bad;
          }
          if (soundonoff_bad == "off") {
            myDFPlayer.stop();
            music = "off";
          }
        }
        tredchange = tnow + nextRedTimeEvent;
        tredoff = tnow + nextRedOff;
        tnextblink = tnow + nextRedBlink_fast;                                     
      }

      if ((rx_previous == "2")) {
        led_off();
      }

      newmessage = false;
  }
      
    
  // Read sensor pin and publish when it changes state
  if (digitalRead(sensorPin) != vInp13)
  {
       vInp13 = digitalRead(sensorPin);
       if (vInp13 == LOW)
         {    
            if (!client.connected()) {
                reconnect();
            }
            client.publish(g_window1_mqtt_topic, "0");
            Serial.println("TX: Closed");
            playedsound = false;
            newmessage = true;
            
            if ((rx_previous == "0"))
            {
              positiveFeedback();
            }
            
         }
       else
        {
           if (!client.connected()) {
                reconnect();
            }
           client.publish(g_window1_mqtt_topic, "1");
           Serial.println("TX: Opened");
           playedsound = false;
           newmessage = true;
           
            if ((rx_previous == "1"))
            {
              positiveFeedback();
            } 
        }
    }
  
  // Update LED state based upon timestamp (red blinks or green off)
  tnow = millis();


  if ((tnow >= tnextblink) && (color == "red")) {
      if (tnow <= tredchange) {
          if (ledState == 1) {
              led_off();
          } else {
              RGB_color(255,0,0);
              ledState=1;             // LED is on
          }    
          tnextblink = tnow + nextRedBlink_fast;
      } 
      if ((tnow >= tredchange) && (tnow <= tredoff)) {
          if (ledState == 1) {
              led_off();
              tnextblink = tnow + nextRedBlink_slow_off;
          } else {
              RGB_color(255,0,0);
              ledState=1;             // LED is on
              tnextblink = tnow + nextRedBlink_slow_on;
          }    
      } 
      if (tnow >= tredoff) {
         led_off();
         color = "off";
      }
  }

  if ((tnow >= tnextoff) && (color == "green")) {
    led_off();
    color = "off";
  }

  if ((tnow >= tmusicstop) && (music=="on")) {
    myDFPlayer.stop();
    music = "off";
  }

}



void RGB_color(int red_light_value, int green_light_value, int blue_light_value)
{
  analogWrite(ledRedPin, red_light_value);
  analogWrite(ledGreenPin, green_light_value);
  analogWrite(ledBluePin, blue_light_value);
}


void positiveFeedback(){
  tnow = millis();
  RGB_color(0,255,0);
  if (soundonoff_good == "on") {
    myDFPlayer.play(goodMusicIndex);
    music = "on";
    tmusicstop = tnow + nextMusicStop_good;
  }

  tnextoff = tnow + nextTimeEvent;
  color = "green";
  ledState=1;             // LED is on
  
  if ((tnow - tpointcountstart >= 0)&&(tnow - tpointcountstart <= nextPointLevelTime)) {
     client.publish(g_points_mqtt_topic,"100");
  }
  if ((tnow - tpointcountstart >= nextPointLevelTime)&&(tnow - tpointcountstart <= 2*nextPointLevelTime)) {
     client.publish(g_points_mqtt_topic,"80");
  }
  if ((tnow - tpointcountstart >= 2*nextPointLevelTime)&&(tnow - tpointcountstart <= 3*nextPointLevelTime)) {
     client.publish(g_points_mqtt_topic,"60");
  }
  if ((tnow - tpointcountstart >= 3*nextPointLevelTime)&&(tnow - tpointcountstart <= 4*nextPointLevelTime)) {
     client.publish(g_points_mqtt_topic,"40");
  }
  if ((tnow - tpointcountstart >= 4*nextPointLevelTime)&&(tnow - tpointcountstart <= 5*nextPointLevelTime)) {
     client.publish(g_points_mqtt_topic,"20");
  }
  
}


void led_off(){
  RGB_color(0,0,0);
  ledState=0;             // LED is off
}





/**************************************************************************/
/*
    Speaker Setup
*/
/**************************************************************************/

//Music

/*
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
*/


/**************************************************************************/
/*
    Arduino loop function, called once 'setup' is complete 
*/
/**************************************************************************/

void loop() {
  if (!client.connected()) {
    reconnect();
  }

  doorRead();
      
  client.loop();
  delay(10);  
}
