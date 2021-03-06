/* TSL2591 Digital Light Sensor */
/* Dynamic Range: 600M:1 */
/* Maximum Lux: 88K */

#include <Wire.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_TSL2591.h"
#include "SoftwareSerial.h"
//#include "DFRobotDFPlayerMini.h"
#include <DFPlayerMini_Fast.h>


/* MQTT */
char g_mqtt_message_buffer[150];    // General purpose buffer for MQTT messages
char g_command_topic[50];           // MQTT topic for receiving commands

char g_light1_mqtt_topic[50];        // MQTT topic for reporting temp1
char g_window1_mqtt_topic[50];       // MQTT topic for reporting window1
char g_points_mqtt_topic[50];        // MQTT topic for reporting points

const char* mqtt_subscribe = "command/window1";
const char* mqtt_subscribe_sound = "UI/notifications";
const char* mqtt_subscribe_volume = "UI/volume";
const char* mqtt_subscribe_goodsound = "UI/goodsound";
const char* mqtt_subscribe_badsound = "UI/badsound";
const char* mqtt_topic_window = "tele/%x/WINDOW1";
const char* mqtt_topic_points = "points/%x/WINDOW1";
const char* mqtt_topic_light = "tele/%x/LIGHT1";

/****************************************************************************/
// WIFI
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

const char* ssid = "AREC_WL";
const char* password = "AREC_WL_PASSWD";
const char* mqtt_server = "192.168.2.101";
const char* mqtt_user = "eric";
const char* mqtt_password = "eric";
const char* mqtt_device_id = "Light_Sensor1";
const boolean mqtt_retain = 0; //set to 0 to resolve reconnect LOP issues
const uint8_t mqtt_qos = 0; //can only be a value of (0, 1, 2)
String rx;
int rxLength = 0;

WiFiClient espClient;
PubSubClient client(espClient);


// SENSOR & LED SETUP
char vInp13 = 0;
String rx_previous;         
int sensorPin = 14;  
int ledBluePin = 16;    
int ledGreenPin = 0;
int ledRedPin = 15;
boolean newmessage = true;
unsigned long tnow, tnextread, tnextoff, tnextblink, tredchange, tredoff;
const int nextReading = 60000;      // sampling rate for light sensor reading
const int nextTimeEvent = 5000;               // time to turn green LED off
const int nextRedTimeEvent = 15000;           // time to change red LED from fast blink to slow blink
const int nextRedOff = 45000;                 // time to turn red LED off
const int nextRedBlink_fast = 500;            // red LED fast blink rate
const int nextRedBlink_slow_on = 2000;        // red LED slow blink on time
const int nextRedBlink_slow_off = 3000;       // red LED slow blink off time
char* color = "off";
int ledState=0;

// window sensor D5 (14), Sound D8 (15), Blue D0 (16), Green D3 (0), Red D8 (15)

unsigned long tpointcountstart;
const int nextPointLevelTime = 10000;     // time interval of point level change
char* points = "on";


// SPEAKER SETUP
SoftwareSerial mySoftwareSerial(13, 12); // RX, TX (D7,D6)
DFPlayerMini_Fast myDFPlayer;
//void printDetail(uint8_t type, int value);
int songFlip = -1;
int lastSwitch = HIGH;
unsigned long tmusicstop;
int goodMusicIndex = 1;
int badMusicIndex = 2;
int nextMusicStop_good = 5000;       
int nextMusicStop_bad = 3000;       
char* soundonoff_bad = "on";              // change this parameter to mute or unmute the speaker
char* soundonoff_good = "on";              // change this parameter to mute or unmute the speaker
boolean playedsound = false;
char* music = "off";
int volume = 10;                          // value from 0 to 30  
 

/****************************************************************************/

// Example for demonstrating the TSL2591 library - public domain!

// connect SCL to I2C Clock
// connect SDA to I2C Data
// connect Vin to 3.3-5V DC
// connect GROUND to common ground

Adafruit_TSL2591 tsl = Adafruit_TSL2591(2591); // pass in a number for the sensor identifier (for your use later)


/**************************************************************************/
/*
    Configures the gain and integration time for the TSL2591
*/
/**************************************************************************/
void configureSensor(void)
{
  // You can change the gain on the fly, to adapt to brighter/dimmer light situations
  //tsl.setGain(TSL2591_GAIN_LOW);    // 1x gain (bright light)
  tsl.setGain(TSL2591_GAIN_LOW);      // 25x gain
  //tsl.setGain(TSL2591_GAIN_HIGH);   // 428x gain
  tsl.setTiming(TSL2591_INTEGRATIONTIME_300MS);


  /* Display the gain and integration time for reference sake */
  Serial.println(F("------------------------------------"));
  Serial.print  (F("Gain:         "));
  tsl2591Gain_t gain = tsl.getGain();
  switch (gain)
  {
    case TSL2591_GAIN_LOW:
      Serial.println(F("1x (Low)"));
      break;
    case TSL2591_GAIN_MED:
      Serial.println(F("25x (Medium)"));
      break;
    case TSL2591_GAIN_HIGH:
      Serial.println(F("428x (High)"));
      break;
    case TSL2591_GAIN_MAX:
      Serial.println(F("9876x (Max)"));
      break;
  }
  Serial.print  (F("Timing:       "));
  Serial.print((tsl.getTiming() + 1) * 100, DEC);
  Serial.println(F(" ms"));
  Serial.println(F("------------------------------------"));
  Serial.println(F(""));
}


/**************************************************************************/
/*
    Program entry point for the Arduino sketch
*/
/**************************************************************************/
void setup(void)
{
  // Set up topic for publishing sensor readings
  sprintf(g_light1_mqtt_topic, mqtt_topic_light,  ESP.getChipId());
  sprintf(g_window1_mqtt_topic, mqtt_topic_window,  ESP.getChipId());
  sprintf(g_points_mqtt_topic, mqtt_topic_points,  ESP.getChipId());

  pinMode(ledBluePin, OUTPUT);
  pinMode(ledRedPin, OUTPUT);
  pinMode(ledGreenPin, OUTPUT);
  pinMode(sensorPin, INPUT_PULLUP);

  Serial.begin(115200);

  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  Serial.println(F("Starting Adafruit TSL2591 Test!"));

  if (tsl.begin())
  {
    Serial.println(F("Found a TSL2591 sensor"));
  }
  else
  {
    Serial.println(F("No sensor found ... check your wiring?"));
    while (1);
  }

  /* Configure the sensor */
  configureSensor();

  // Now we're ready to get readings ... move on to loop()!

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

// WIFI SETUP

void setup_wifi() {
  delay(10);  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  //WiFi.config(ip, gateway, subnet);
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

  // Open/Close Door Notification
  if ((rx == "0") || (rx == "1") || (rx == "2")) {
    if (rx != rx_previous) {
      playedsound = false;
      newmessage = true;
      tpointcountstart = millis();
      points = "on";
    }
    rx_previous = rx;
  }

  // Amount of Sound Notification
  if (rx == "Full") {
    soundonoff_bad = "on";              
    soundonoff_good = "on";
  }
  
  if (rx == "Success") {
    soundonoff_bad = "off";              
    soundonoff_good = "on";
  }
  if (rx == "Off") {
    soundonoff_bad = "off";              
    soundonoff_good = "off";
  }

  // Volume Notification
  if (rx.toInt() >= 100){
    volume = map(rx.toInt(),100,200,0,25);
    myDFPlayer.volume(volume);
  }


    // Good / Bad Song Index Notification
  if (rx == "Classic_good") {
    goodMusicIndex = 1;
    nextMusicStop_good = 2000;
  }
  if (rx == "Spin_good") {
    goodMusicIndex = 2;
    nextMusicStop_good = 1000;
  }
  if (rx == "Robot_good") {
    goodMusicIndex = 3;
    nextMusicStop_good = 2000;
  }
  if (rx == "Vibrant_good") {
    goodMusicIndex = 4;
    nextMusicStop_good = 2500;
  }
  if (rx == "Double_good") {
    goodMusicIndex = 5;
    nextMusicStop_good = 2000;
  }
  if (rx == "Splash_good") {
    goodMusicIndex = 6;
    nextMusicStop_good = 3000;
  }
  if (rx == "SciFi_good") {
    goodMusicIndex = 7;
    nextMusicStop_good = 2000;
  }
  if (rx == "Twinkle_good") {
    goodMusicIndex = 8;
    nextMusicStop_good = 3000;
  }
  if (rx == "Bonk_good") {
    goodMusicIndex = 9;
    nextMusicStop_good = 2000;
  }
  if (rx == "Beep_good") {
    goodMusicIndex = 10;
    nextMusicStop_good = 2000;
  }
  if (rx == "Trumpet_good") {
    goodMusicIndex = 11;
    nextMusicStop_good = 3000;
  }
  if (rx == "Ta-Da_good") {
    goodMusicIndex = 12;
    nextMusicStop_good = 3000;
  }


  if (rx == "Classic_bad") {
    badMusicIndex = 1;
    nextMusicStop_bad = 2000;
  }
  if (rx == "Spin_bad") {
    badMusicIndex = 2;
    nextMusicStop_bad = 1000;
  }
  if (rx == "Robot_bad") {
    badMusicIndex = 3;
    nextMusicStop_bad = 2000;
  }
  if (rx == "Vibrant_bad") {
    badMusicIndex = 4;
    nextMusicStop_bad = 2500;
  }
  if (rx == "Double_bad") {
    badMusicIndex = 5;
    nextMusicStop_bad = 2000;
  }
  if (rx == "Splash_bad") {
    badMusicIndex = 6;
    nextMusicStop_bad = 3000;
  }
  if (rx == "SciFi_bad") {
    badMusicIndex = 7;
    nextMusicStop_bad = 2000;
  }
  if (rx == "Twinkle_bad") {
    badMusicIndex = 8;
    nextMusicStop_bad = 3000;
  }
  if (rx == "Bonk_bad") {
    badMusicIndex = 9;
    nextMusicStop_bad = 2000;
  }
  if (rx == "Beep_bad") {
    badMusicIndex = 10;
    nextMusicStop_bad = 2000;
  }
  if (rx == "Trumpet_bad") {
    badMusicIndex = 11;
    nextMusicStop_bad = 3000;
  }
  if (rx == "Ta-Da_bad") {
    badMusicIndex = 12;
    nextMusicStop_bad = 3000;
  }
  
}

// RECONNECT TO MQTT IF YOU LOSE CONNECTION

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(g_light1_mqtt_topic, mqtt_user, mqtt_password)) {
      //needs next line for proper connect to mqtt broker that doesn't allow anonymous
      //if (client.connect(mqtt_device_id, mqtt_user, mqtt_password, mqtt_topic, mqtt_qos, mqtt_retain, mqtt_lwt)) {
      Serial.println("connected");
      // ... and resubscribe
      client.subscribe(mqtt_subscribe);
      client.subscribe(mqtt_subscribe_sound);
      client.subscribe(mqtt_subscribe_volume);
      client.subscribe(mqtt_subscribe_goodsound);
      client.subscribe(mqtt_subscribe_badsound);
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
    Shows how to perform a basic read on visible, full spectrum or
    infrared light (returns raw 16-bit ADC values)
*/
/**************************************************************************/
void simpleRead(void)
{
  // Simple data read example. Just read the infrared, fullspecrtrum diode
  // or 'visible' (difference between the two) channels.
  // This can take 100-600 milliseconds! Uncomment whichever of the following you want to read
  uint16_t x = tsl.getLuminosity(TSL2591_VISIBLE);
  //uint16_t x = tsl.getLuminosity(TSL2591_FULLSPECTRUM);
  //uint16_t x = tsl.getLuminosity(TSL2591_INFRARED);

  Serial.print(F("[ ")); Serial.print(millis()); Serial.print(F(" ms ] "));
  Serial.print(F("Luminosity: "));
  Serial.println(x, DEC);
}

/**************************************************************************/
/*
    Show how to read IR and Full Spectrum at once and convert to lux
*/
/**************************************************************************/
void advancedRead(void)
{
  // More advanced data read example. Read 32 bits with top 16 bits IR, bottom 16 bits full spectrum
  // That way you can do whatever math and comparisons you want!
  uint32_t lum = tsl.getFullLuminosity();
  uint16_t ir, full;
  ir = lum >> 16;
  full = lum & 0xFFFF;
  float lux = tsl.calculateLux(full, ir);
  // Read and send light sensor data
  String message_string = String(lux);
  message_string.toCharArray(g_mqtt_message_buffer, message_string.length()+1);
  client.publish(g_light1_mqtt_topic, g_mqtt_message_buffer);
  
  Serial.print(F("[ ")); Serial.print(millis()); Serial.print(F(" ms ] "));
  Serial.print(F("IR: ")); Serial.print(ir);  Serial.print(F("  "));
  Serial.print(F("Full: ")); Serial.print(full); Serial.print(F("  "));
  Serial.print(F("Visible: ")); Serial.print(full - ir); Serial.print(F("  "));
  Serial.print(F("Lux: ")); Serial.println(lux, 6);
}

/**************************************************************************/
/*
    Performs a read using the Adafruit Unified Sensor API.
*/
/**************************************************************************/
void unifiedSensorAPIRead(void)
{
  /* Get a new sensor event */
  sensors_event_t event;
  tsl.getEvent(&event);

  /* Display the results (light is measured in lux) */
  Serial.print(F("[ ")); Serial.print(event.timestamp); Serial.print(F(" ms ] "));
  if ((event.light == 0) |
      (event.light > 4294966000.0) |
      (event.light < -4294966000.0))
  {
    /* If event.light = 0 lux the sensor is probably saturated */
    /* and no reliable data could be generated! */
    /* if event.light is +/- 4294967040 there was a float over/underflow */
    Serial.println(F("Invalid data (adjust gain or timing)"));
  }
  else
  {
    Serial.print(event.light); Serial.println(F(" lux"));
  }
}

/**************************************************************************/
/*
    Read the window sensor
*/
/**************************************************************************/

void windowRead(void)
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
  
  if ((tnow - tpointcountstart >= 0)&&(tnow - tpointcountstart <= nextPointLevelTime)&&(points=="on")) {
     client.publish(g_points_mqtt_topic,"100");
     points = "off";
  }
  if ((tnow - tpointcountstart >= nextPointLevelTime)&&(tnow - tpointcountstart <= 2*nextPointLevelTime)&&(points=="on")) {
     client.publish(g_points_mqtt_topic,"80");
     points = "off";
  }
  if ((tnow - tpointcountstart >= 2*nextPointLevelTime)&&(tnow - tpointcountstart <= 3*nextPointLevelTime)&&(points=="on")) {
     client.publish(g_points_mqtt_topic,"60");
     points = "off";
  }
  if ((tnow - tpointcountstart >= 3*nextPointLevelTime)&&(tnow - tpointcountstart <= 4*nextPointLevelTime)&&(points=="on")) {
     client.publish(g_points_mqtt_topic,"40");
     points = "off";
  }
  if ((tnow - tpointcountstart >= 4*nextPointLevelTime)&&(tnow - tpointcountstart <= 5*nextPointLevelTime)&&(points=="on")) {
     client.publish(g_points_mqtt_topic,"20");
     points = "off";
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
void loop(void)
{
  // reconnect to server if not connected
  if (!client.connected()) {
      reconnect();
    }

  tnow = millis();
  
  if (tnow >= tnextread) {
    // read and send data
    advancedRead();
    //simpleRead();
    //unifiedSensorAPIRead();
    tnextread = tnow + nextReading;
  }
  
  windowRead();
  
  client.loop();
  
}
