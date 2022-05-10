// don't open serial monitor while running this pleaseee trust me

#include"pitches.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

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
int ledBluePin = 4;
int ledGreenPin = 5;
int ledRedPin = 16;
//int ledGpioPin =4;
const char* soundonoff = "on";
boolean playedsound = false;
unsigned long tnow, tnextoff;
const int nextTimeEvent = 5000;
char* color = "off";

// window sensor D5 (14), Sound D8 (15), Blue D2 (4), Green D1 (5), Red D0 (16)

int ledState=0;

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
  sprintf(g_window1_mqtt_topic, "tele/%x/WINDOW1",  ESP.getChipId());                                  // CHANGE THIS FOR NEW SENSOR!!!!!!!!!
  
  pinMode(ledBluePin, OUTPUT);
  pinMode(ledRedPin, OUTPUT);
  pinMode(ledGreenPin, OUTPUT);
  pinMode(soundPin, OUTPUT);
  pinMode(sensorPin, INPUT_PULLUP);
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

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
                                                                                                  // CHANGE THIS FOR NEW SENSOR!!!!!!!!!
void callback(char* window1, byte* payload, unsigned int length) {\                                                          
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
  }
  rx_previous = rx;

// State initial window position
  if (rx == "Status") //normal operation
  {
       vInp13 = digitalRead(sensorPin);
       if (vInp13 == LOW)
         {    
            client.publish(g_window1_mqtt_topic, "1");
            Serial.println("TX: Opened");
         }
      else
         {
            client.publish(g_window1_mqtt_topic, "0");
            Serial.println("TX: Closed");
        }        
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
      client.subscribe("command/window1");                                                            // CHANGE THIS FOR NEW SENSOR!!!!!!!!!
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
    if (playedsound == false) {
        //Evaulate the recieved message to do stuff
        if ((rx == "0") && (vInp13 == HIGH)) 
        {
          RGB_color(255,0,0);
          lowMusic();
          playedsound = true;
          color = "red";                                
        }
        if ((rx == "1") && (vInp13 == LOW))
        {
          RGB_color(255,0,0);
          lowMusic();
          playedsound = true;
          color = "red";
        }
    }
    
  
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
            tnow = millis();
            tnextoff = tnow + nextTimeEvent;
            color = "green";
            
          if ((rx == "0"))
          {
            RGB_color(0,255,0);
            highMusic();
          }
            
            //RGB_color(255,0,0);
            //lowMusic();
            
            //client.disconnect();  // Disconnect from MQTT broker
         }
       else
        {
           if (!client.connected()) {
                reconnect();
            }
           client.publish(g_window1_mqtt_topic, "1");
           Serial.println("TX: Opened");
           playedsound = false;
           tnow = millis();
           tnextoff = tnow + nextTimeEvent;
           color = "green";

          if ((rx == "1"))
          {
            RGB_color(0,255,0);
            highMusic();
          }
           
           //RGB_color(0,255,0);
           //highMusic();
           //client.disconnect();   // Disconnect from MQTT broker
        }
    }

  tnow = millis();
  
  if ((tnow >= tnextoff) && (color == "green")) {
  RGB_color(0,0,0);
  color = "off";
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
