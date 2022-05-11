// don't open serial monitor while running this pleaseee trust me

#include"pitches.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

const char* mqtt_subscribe = "command/curtain1";
const char* mqtt_topic = "tele/%x/CURTAIN1";

const char* ssid = "AREC_WL";
const char* password = "AREC_WL_PASSWD";
const char* mqtt_server = "192.168.2.102";
char g_window1_mqtt_topic[50];        // MQTT topic for reporting window1          
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
int soundPin = 15;
int ledBluePin = 0;
int ledGreenPin = 4;
int ledRedPin = 5;
const char* soundonoff = "on";
boolean playedsound = false;
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

// window sensor D5 (14), Sound D8 (15), Blue D3 (0), Green D2 (4), Red D1 (5)


//Music
//DescendingLow
int melodyLow[] = {
  NOTE_A2, NOTE_C2, END
};

int noteDurationsLow[] = {
  16, 32
};

//AscendingHigh
int melodyHigh[] = {
  NOTE_C5, NOTE_E5, NOTE_G5, NOTE_B5, END
};

int noteDurationsHigh[] = {
  8, 8, 8, 16
};

int speedHigh = 10; //Ascending high speed
int speedLow = 45; //Ascending low speed

void highMusic(){
  if (soundonoff == "on") {
      for (int thisNote = 0; melodyHigh[thisNote]!=-1; thisNote++) {
      int noteDuration = speedHigh*noteDurationsHigh[thisNote];
      tone(soundPin, melodyHigh[thisNote],noteDuration*.95);
      Serial.println(melodyHigh[thisNote]);
      /*
      if(digitalRead(sensorPin) == LOW){
        noTone(soundPin);
        return;
      }
      */
      delay(noteDuration);
      noTone(soundPin);
      }
  }
}

void lowMusic(){
  if (soundonoff == "on") {
      for (int thisNote = 0; melodyLow[thisNote]!=-1; thisNote++) {
      int noteDuration = speedLow*noteDurationsLow[thisNote];
      tone(soundPin, melodyLow[thisNote],noteDuration*.95);
      Serial.println(melodyLow[thisNote]);
      /*
      if(digitalRead(sensorPin) == HIGH){
        noTone(soundPin);
        return;
      }
      */
      delay(noteDuration);
      noTone(soundPin);
      }
  } 
}


WiFiClient espClient;
PubSubClient client(espClient);

void setup() {   
  // Set up topic for publishing sensor readings
  sprintf(g_window1_mqtt_topic, mqtt_topic,  ESP.getChipId());                                  
  
  pinMode(ledBluePin, OUTPUT);
  pinMode(ledRedPin, OUTPUT);
  pinMode(ledGreenPin, OUTPUT);
  pinMode(soundPin, OUTPUT);
  pinMode(sensorPin, INPUT_PULLUP);
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  pinMode(16, OUTPUT);
  pinMode(2, OUTPUT);
  digitalWrite(16, HIGH);            // turn off on-board blue LED
  digitalWrite(2,HIGH);
  
}


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

  if (rx != rx_previous) {
    playedsound = false;
    newmessage = true;
  }
  rx_previous = rx;

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
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


void loop() {
  if (!client.connected()) {
    reconnect();
  }

  // Perform an action when rx changes value
  if (newmessage == true) {
      //Evaulate the recieved message to do stuff
      if (((rx == "0") && (vInp13 == LOW)) || ((rx == "1") && (vInp13 == HIGH)))
      {
        RGB_color(255,0,0);
        color = "red";
        ledState=1;             // LED is on
        if (playedsound == false) {
          lowMusic();
          playedsound = true;
        }
        tnow = millis();
        tredchange = tnow + nextRedTimeEvent;
        tredoff = tnow + nextRedOff;
        tnextblink = tnow + nextRedBlink_fast;                                     
      }

      if ((rx == "2")) {
        RGB_color(0,0,0);
        color = "off";
        ledState=0;             // LED is off
      }

      newmessage = false;
  }
      
    
  // Read sensor pin and publish when it changes state
  if (digitalRead(sensorPin) != vInp13)
  {
       vInp13 = digitalRead(sensorPin);
       if (vInp13 == HIGH)
         {    
            if (!client.connected()) {
                reconnect();
            }
            client.publish(g_window1_mqtt_topic, "0");
            Serial.println("TX: Closed");
            playedsound = false;
            newmessage = true;
            
            if ((rx == "0"))
            {
              RGB_color(0,255,0);
              highMusic();
              
              tnow = millis();
              tnextoff = tnow + nextTimeEvent;
              color = "green";
              ledState=1;             // LED is on
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
           
            if ((rx == "1"))
            {
              RGB_color(0,255,0);
              highMusic();
              
              tnow = millis();
              tnextoff = tnow + nextTimeEvent;
              color = "green";
              ledState=1;             // LED is on
            } 
        }
    }
  
  // Update LED state based upon timestamp (red blinks or green off)
  tnow = millis();


  if ((tnow >= tnextblink) && (color == "red")) {
      if (tnow <= tredchange) {
          if (ledState == 1) {
              RGB_color(0,0,0);
              ledState=0;             // LED is off
          } else {
              RGB_color(255,0,0);
              ledState=1;             // LED is on
          }    
          tnextblink = tnow + nextRedBlink_fast;
      } 
      if ((tnow >= tredchange) && (tnow <= tredoff)) {
          if (ledState == 1) {
              RGB_color(0,0,0);
              ledState=0;             // LED is off
              tnextblink = tnow + nextRedBlink_slow_off;
          } else {
              RGB_color(255,0,0);
              ledState=1;             // LED is on
              tnextblink = tnow + nextRedBlink_slow_on;
          }    
      } 
      if (tnow >= tredoff) {
         RGB_color(0,0,0);
         color = "off";
         ledState=0;             // LED is off
      }
  }

  if ((tnow >= tnextoff) && (color == "green")) {
    RGB_color(0,0,0);
    color = "off";
    ledState=0;             // LED is off
  }
      
  client.loop();
  delay(10);  

}



void RGB_color(int red_light_value, int green_light_value, int blue_light_value)
{
  analogWrite(ledRedPin, red_light_value);
  analogWrite(ledGreenPin, green_light_value);
  analogWrite(ledBluePin, blue_light_value);
}
