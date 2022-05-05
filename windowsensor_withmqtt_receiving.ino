// don't open serial monitor while running this pleaseee trust me

#include <ESP8266WiFi.h>
#include <PubSubClient.h>

const char* ssid = "AREC_WL";
const char* password = "AREC_WL_PASSWD";
const char* mqtt_server = "192.168.2.101";
char g_window1_mqtt_topic[50];        // MQTT topic for reporting window1
const char* mqtt_user = "eric";
const char* mqtt_password = "eric";
const char* mqtt_device_id = "Window_Sensor1";
const boolean mqtt_retain = 0; //set to 0 to resolve reconnect LOP issues
const uint8_t mqtt_qos = 0; //can only be a value of (0, 1, 2)
char vInp13 = 0;
String rx;         
int rxLength = 0;
//int relayPin = 5; 
int sensorPin = 5;
int ledGpioPin =4;

int ledState=0;


WiFiClient espClient;
PubSubClient client(espClient);

void setup() {   
  // Set up topic for publishing sensor readings
  sprintf(g_window1_mqtt_topic, "tele/%x/WINDOW1",  ESP.getChipId());
  
  pinMode(ledGpioPin, OUTPUT);
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

//Evaulate the recieved message to do stuff
    if ((rx == "0"))      //&& (vInp13 == HIGH)) //normal operations
    {
      digitalWrite(ledGpioPin, 0); //Turn the LED output on 
      delay(1000);                                     //Wait a second   
    }
    if ((rx == "1"))
    {
      digitalWrite(ledGpioPin, 1); //Turn the LED output off 
      delay(1000);                                     //Wait a second
    }

// State initial window position
  if (rx == "Status") //normal operation
  {
       vInp13 = digitalRead(sensorPin);
       if (vInp13 == LOW)
         {    
            client.publish(g_window1_mqtt_topic, "1");
            Serial.println("TX: WindowOpened");
         }
      else
         {
            client.publish(g_window1_mqtt_topic, "0");
            Serial.println("TX: WindowClosed");
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
      client.subscribe(g_window1_mqtt_topic);
      client.subscribe("command/window1");
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
  if (digitalRead(sensorPin) != vInp13)
    {
       vInp13 = digitalRead(sensorPin);
       if (vInp13 == LOW)
         {    
            if (!client.connected()) {
                reconnect();
            }
            client.publish(g_window1_mqtt_topic, "1");
            Serial.println("TX: WindowOpened");
            //client.disconnect();  // Disconnect from MQTT broker
         }
       else
        {
           if (!client.connected()) {
                reconnect();
            }
           client.publish(g_window1_mqtt_topic, "0");
           Serial.println("TX: WindowClosed");
           //client.disconnect();   // Disconnect from MQTT broker
        }
    }
  client.loop();
  delay(10);  
}
