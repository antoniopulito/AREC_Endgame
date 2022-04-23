/*********
  Rui Santos
  Complete project details at https://randomnerdtutorials.com  
*********/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// Set pins
int StatusPin = 12;  // Digital pin D6
int sensorPin = 13;  // Digital pin D7

// WiFi & MQTT setup
const char* ssid = "AREC_WL";
const char* password = "AREC_WL_PASSWD";
const char* mqtt_server = "192.168.2.101";
char g_motion1_mqtt_topic[50];        // MQTT topic for reporting door1
const char* mqtt_user = "eric";
const char* mqtt_password = "eric";
const char* mqtt_device_id = "Door_Sensor1";
const boolean mqtt_retain = 0; //set to 0 to resolve reconnect LOP issues
const uint8_t mqtt_qos = 0; //can only be a value of (0, 1, 2)
String rx;         
int rxLength = 0;

WiFiClient espClient;
PubSubClient client(espClient);

// Wifi Setup Function
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

// MQTT Communication Setup
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
}

// Reconnect to MQTT if disconnected
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(g_motion1_mqtt_topic,mqtt_user,mqtt_password)) {
    //needs next line for proper connect to mqtt broker that doesn't allow anonymous
    //if (client.connect(mqtt_device_id, mqtt_user, mqtt_password, mqtt_topic, mqtt_qos, mqtt_retain, mqtt_lwt)) {
      Serial.println("connected");
        // ... and resubscribe
        client.subscribe(g_motion1_mqtt_topic);
     } else {
        Serial.print("failed, rc=");
        Serial.print(client.state());
        Serial.println(" try again in 5 seconds");
        // Wait 5 seconds before retrying
        delay(5000);
     }
  }
}

void setup() {
  // Set up topic for publishing sensor readings
  sprintf(g_motion1_mqtt_topic, "tele/%x/MOTION1",  ESP.getChipId());

  pinMode(sensorPin, INPUT);   // declare sensor as input
  pinMode(StatusPin, OUTPUT);  // declare LED as output
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  long state = digitalRead(sensorPin);
    if(state == HIGH) {
      client.publish(g_motion1_mqtt_topic, "1");
      digitalWrite (StatusPin, HIGH);
      Serial.println("Motion detected!");  
    } else {
      client.publish(g_motion1_mqtt_topic, "0");
      digitalWrite (StatusPin, LOW);
      Serial.println("No Motion detected.");
    }
    client.loop();  // Allows client to process incoming messages to send publish data and refreshes the connection
    delay(10);
    //client.disconnect();   // Disconnect from MQTT broker
    delay(10000);   // time delay before next reading
}
