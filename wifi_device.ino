/* MQTT */
char g_mqtt_message_buffer[150];    // General purpose buffer for MQTT messages
char g_command_topic[50];           // MQTT topic for receiving commands

char g_device1_mqtt_topic[50];        // MQTT topic for reporting temp1

/****************************************************************************/
// WIFI
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

const char* ssid = "AREC_WL";
const char* password = "AREC_WL_PASSWD";
const char* mqtt_server = "192.168.2.100";
const char* mqtt_user = "eric";
const char* mqtt_password = "eric";
const char* mqtt_device_id = "Light_Sensor1";
//const char* mqtt_lwt = "LAST_WILL_IF_DESIRED";
const boolean mqtt_retain = 0; //set to 0 to resolve reconnect LOP issues
const uint8_t mqtt_qos = 0; //can only be a value of (0, 1, 2)
String rx;
int rxLength = 0;

WiFiClient espClient;
PubSubClient client(espClient);

void setup(void)
{
  // Set up topic for publishing sensor readings
  sprintf(g_device1_mqtt_topic, "tele/%x/DEVICE1",  ESP.getChipId());

  Serial.begin(9600);

  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

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
}

void reconnect() {
  char mqtt_client_id[30];
  sprintf(mqtt_client_id, "esp8266-%x", ESP.getChipId());

  // Loop until we're reconnected
  Serial.print("Connecting to MQTT Broker");
  while (!client.connected()){
    Serial.print(".");
    if (client.connect(mqtt_client_id, mqtt_user, mqtt_password)){
      Serial.println("Connected");
      sprintf(g_mqtt_message_buffer, "Device %s starting up", mqtt_client_id);
    } else{
      delay(5000);
    }
  }
}

void advancedRead(void)
{
  float lux = 1;
  String message_string = String(lux);
  message_string.toCharArray(g_mqtt_message_buffer, message_string.length()+1);
  client.publish(g_device1_mqtt_topic, g_mqtt_message_buffer);
}

void loop(void)
{
  // reconnect to server if not connected
  if (!client.connected()) {
      reconnect();
    }

  // read and send data
  advancedRead();
  //simpleRead();
  //unifiedSensorAPIRead();
  
  client.loop();
  
  // sampling rate
  delay(5000); // 1 minute
}
