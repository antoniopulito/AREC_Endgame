// don't open serial monitor while running this pleaseee trust me

#include <ESP8266WiFi.h>
#include <PubSubClient.h>

const char* ssid = "AREC_WL";
const char* password = "AREC_WL_PASSWD";
// Update these with values suitable for your network.
//IPAddress ip(XXX.XXX.XXX.XXX);  //Node static IP
//IPAddress gateway(XXX.XXX.XXX.XXX);
//IPAddress subnet(XXX.XXX.XXX.XXX);
const char* mqtt_server = "192.168.2.101";
char g_door1_mqtt_topic[50];        // MQTT topic for reporting door1
const char* mqtt_user = "eric";
const char* mqtt_password = "eric";
const char* mqtt_device_id = "Door_Sensor1";
//const char* mqtt_lwt = "LAST_WILL_IF_DESIRED";
const boolean mqtt_retain = 0; //set to 0 to resolve reconnect LOP issues
const uint8_t mqtt_qos = 0; //can only be a value of (0, 1, 2)
char vInp13 = 0;
String rx;         
int rxLength = 0;
//int relayPin = 5; 
int sensorPin = 5;
int ledGpioPin =4;

// Not sure if needed
//int doorState=0;
int ledState=0;
//#define closed 0
//#define opened 1

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {   
  // Set up topic for publishing sensor readings
  sprintf(g_door1_mqtt_topic, "tele/%x/DOOR1",  ESP.getChipId());
  
  pinMode(ledGpioPin, OUTPUT);
  pinMode(sensorPin, INPUT_PULLUP);
  //digitalWrite(relayPin, 1);
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

/*
    //Evaulate the recieved message to do stuff
    if ((rx == "OpenGarageDoor") && (vInp13 == HIGH)) //normal operations
//  if ((rx == "True") && (vInp13 == HIGH)) //difference in home-assistant, prevent error in log
  {digitalWrite(relayPin, 1);} //Turn the output on / open the door 
  delay(1000);                                     //Wait a second
  digitalWrite(relayPin, 0);                              //Turn the output back off   
  delay(1000);                                     //Let Voltage settle before resuming.  

  if ((rx == "CloseGarageDoor") && (vInp13 == LOW))  //normal operations
//  if ((rx == "False") && (vInp13 == LOW))  //difference in home-assistant, prevent error in log
  {digitalWrite(relayPin, 1);} //Turn the output on / close the door 
  delay(1000);                                     //Wait a second
  digitalWrite(relayPin, 0);                              //Turn the output back off   
  delay(1000);                                     //Let Voltage settle before resuming.  
*/

// State initial door position
  if (rx == "Status") //normal operation
//  if (rx == "100")  //difference in home-assistant, prevent error in log
  {
       vInp13 = digitalRead(sensorPin);
       if (vInp13 == LOW)
         {    
            client.publish(g_door1_mqtt_topic, "1");
            Serial.println("TX: DoorOpened");
            ledState=0; //off
            digitalWrite(ledGpioPin, ledState);
         }
      else
         {
            client.publish(g_door1_mqtt_topic, "0");
            Serial.println("TX: DoorClosed");
            ledState=1; //on
            digitalWrite(ledGpioPin, ledState);
        }        
  }

}



void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
  Serial.print("Attempting MQTT connection...");
  // Attempt to connect
  if (client.connect(g_door1_mqtt_topic,mqtt_user,mqtt_password)) {
  //needs next line for proper connect to mqtt broker that doesn't allow anonymous
  //if (client.connect(mqtt_device_id, mqtt_user, mqtt_password, mqtt_topic, mqtt_qos, mqtt_retain, mqtt_lwt)) {
    Serial.println("connected");
 /*   
  if (digitalRead(sensorPin) != vInp13)
    {
       vInp13 = digitalRead(sensorPin);
       if (vInp13 == LOW)
         {    
            client.publish(mqtt_topic, "Open");
            Serial.println("TX: DoorOpened");
         }
      else
         {
            client.publish(mqtt_topic, "Closed");
            Serial.println("TX: DoorClosed");
        }
  }
  */
      // ... and resubscribe
      client.subscribe(g_door1_mqtt_topic);
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
/*
  if (!client.connected()) {
    reconnect();
  }
  */
  if (digitalRead(sensorPin) != vInp13)
    {
       vInp13 = digitalRead(sensorPin);
       if (vInp13 == LOW)
         {    
            if (!client.connected()) {
                reconnect();
            }
            client.publish(g_door1_mqtt_topic, "1");
            Serial.println("TX: DoorOpened");
            //doorState=opened;
            ledState=0; //off
            digitalWrite(ledGpioPin, ledState);
            client.disconnect();  // Disconnect from MQTT broker
         }
       else
        {
           if (!client.connected()) {
                reconnect();
            }
           client.publish(g_door1_mqtt_topic, "0");
           Serial.println("TX: DoorClosed");
           //doorState=closed;
           ledState=1; //on
           digitalWrite(ledGpioPin, ledState);
           client.disconnect();   // Disconnect from MQTT broker
        }
    }
  client.loop();
  delay(10);  
}
