/* Configuration for the code are here */
#include "config.h"

/* Libraries */
#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "DHT.h"
#include <Adafruit_Sensor.h>
#include "Adafruit_CCS811.h"

/* Global Variables */
#define   ESP_STATE_ASLEEP    0
#define   ESP_STATE_READY     1
uint8_t   esp_state         = 0;
uint32_t  esp_state_start   = 0;    // timestamp when esp last changed
uint8_t   dht_reading_taken = 0;    // 0/1, whether any reading has been taken
float     temp_value         = 0;    // temperature value from dht22 reading
float     hum_value         = 0; // humidty vlaue from dht22 reading


/* -------For ESP8266-------- */
#define   SERIAL_BAUD_RATE    115200          // Speed for USB serial console
#define   MODE_BUTTON_PIN   D3    // GPIO0 Pushbutton to GND
#define   ESP_WAKEUP_PIN    D0    // To reset ESP8266 after deep sleep

/* MQTT */
char g_mqtt_message_buffer[150];    // General purpose buffer for MQTT messages
char g_command_topic[50];           // MQTT topic for receiving commands

#if REPORT_MQTT_SEPERATE
char g_temp1_mqtt_topic[50];        // MQTT topic for reporting temp1
char g_hum1_mqtt_topic[50];         // MQTT topic for reporting hum1
#endif
#if REPORT_MQTT_JSON
char g_mqtt_json_topic[50];         // MQTT topic for reporting all values using JSON
#endif
char g_motion1_mqtt_topic[50];        // MQTT topic for reporting motion1
char g_CO2_mqtt_topic[50];            // MQTT topic for reporting CO2
char g_TVOC_mqtt_topic[50];           // MQTT topic for reporting TVOC

// WiFi
#define WIFI_CONNECT_INTERVAL 500   // Wait 500ms intervals for wifi connection
#define WIFI_CONNECT_MAX_ATTEMPTS 10// Number of attempts/intervals to wait

// DHT Setup 
#define DHTPIN 2                    // actually D4 pin on ESP8266
#define DHTTYPE DHT22               // DHT 22  (AM2302)
DHT dht(DHTPIN, DHTTYPE);

// PIR Setup
int StatusPin = 12;  // Digital pin D6
int sensorPin = 13;  // Digital pin D7

// CCS811 Setup (Air Quality)
Adafruit_CCS811 ccs;
int CO2_value;
int TVOC_value;

// General
uint32_t g_device_id;

/* Function Signatures */
void mqttCallback(char* topic, byte* payload, uint8_t length);
bool initWifi();
void reconnectMqtt();
void updateDHTReadings();
void reportToMqtt();
void updatePIRReadings();
void updateCCS811Readings();

/* Instantiate Global Objects */
// MQTT
WiFiClient esp_client;
PubSubClient client(esp_client);


/*----------SETUP------------*/
void setup()  {
  Serial.begin(SERIAL_BAUD_RATE);
  Serial.println();
  Serial.println("ESP Starting Up");

  pinMode(2, OUTPUT);
  digitalWrite(2,HIGH);
  dht.begin();

  // Device ID for MQTT Client
  g_device_id = ESP.getChipId();
  Serial.print("Device ID: ");
  Serial.println(g_device_id, HEX);

  // Set up topics for publishing sensor readings
  sprintf(g_motion1_mqtt_topic, "tele/%x/MOTION1",  ESP.getChipId());
  sprintf(g_CO2_mqtt_topic, "tele/%x/CO2",  ESP.getChipId());
  sprintf(g_TVOC_mqtt_topic, "tele/%x/TVOC",  ESP.getChipId());
  sprintf(g_command_topic,  "cmnd/%x/COMMAND",  ESP.getChipId());   // For receiving commands
  #if REPORT_MQTT_SEPERATE
  sprintf(g_temp1_mqtt_topic, "tele/%x/TEMP1",  ESP.getChipId());   // Data from DHT
  sprintf(g_hum1_mqtt_topic, "tele/%x/HUM1",  ESP.getChipId());   // Data from DHT
  #endif
  #if REPORT_MQTT_JSON
  sprintf(g_mqtt_json_topic,  "tele/%x/temp_sensors", ESP.getChipId());   // Data from DHT
  #endif
  
  pinMode(sensorPin, INPUT);   // declare sensor as input
  pinMode(StatusPin, OUTPUT);  // declare LED as output

  // Begin WiFi
  WiFi.begin(ssid, password);
  WiFi.mode(WIFI_STA);
  
  // Connecting to WiFi...
  Serial.print("Connecting to ");
  Serial.print(ssid);
  // Loop continuously while WiFi is not connected
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(100);
    Serial.print(".");
  }
  
  // Connected to WiFi
  Serial.println();
  Serial.print("Connected! IP address: ");
  Serial.println(WiFi.localIP());
  
  /* Set up the MQTT Client */
  client.setServer(mqtt_broker, 1883);
  client.setCallback(mqttCallback);
  Serial.println("MQTT Client Initiated");
  client.setBufferSize(255);

  // AQ sensor setup
  if(!ccs.begin()){
    //Serial.println("Failed to start Air Quality sensor! Please check your wiring.");
    while(1);
  }
  
}


void loop() {
  
  if (WiFi.status() == WL_CONNECTED){
    if (!client.connected()){
      reconnectMqtt();
    }
  }
  
  client.loop();    // Process any outstanding MQTT messages

  updatePIRReadings();
  delay(5000);
  updateCCS811Readings();
  delay(5000);
  updateDHTReadings();
  delay(5000);
  
}


/* Update DHT reading */
void updateDHTReadings(){
  
  uint32_t time_now = millis();

  if (ESP_STATE_ASLEEP == esp_state){
    if (time_now - esp_state_start >= esp_report_period*1000){
      esp_state_start = time_now;
      esp_state = ESP_STATE_READY;
    }
  }

  if (ESP_STATE_READY == esp_state){
    temp_value = dht.readTemperature();
    hum_value = dht.readHumidity();
    dht_reading_taken = true;
  

    // Report the new values
    reportToMqtt();
    reportToSerial();

    esp_state_start = time_now;
    esp_state = ESP_STATE_ASLEEP;
    dht_reading_taken = false;
  }
  
}


/* Update PIR reading */
void updatePIRReadings(){
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
}

/* Update CCS811 reading */
void updateCCS811Readings() {
  if(!ccs.readData()){
      // define strings
      String message_string_CO2;
      String message_string_TVOC;
      // read values from sensor
      CO2_value = ccs.geteCO2();
      TVOC_value = ccs.getTVOC();

      message_string_CO2 = String(CO2_value);
      message_string_CO2.toCharArray(g_mqtt_message_buffer, message_string_CO2.length()+1);
      client.publish(g_CO2_mqtt_topic, g_mqtt_message_buffer);

      message_string_TVOC = String(TVOC_value);
      message_string_TVOC.toCharArray(g_mqtt_message_buffer, message_string_CO2.length()+1);
      client.publish(g_TVOC_mqtt_topic, g_mqtt_message_buffer);
    }
    else{
      while(1);
    }
}


/* Report the latest values to MQTT */
void reportToMqtt(){
  String message_string_temp1;
  String message_string_hum1;

  Serial.println("Reporting to MQTT...");

  #if REPORT_MQTT_SEPERATE
  if (dht_reading_taken){
    message_string_temp1 = String(temp_value);
    message_string_temp1.toCharArray(g_mqtt_message_buffer, message_string_temp1.length()+1);
    client.publish(g_temp1_mqtt_topic,g_mqtt_message_buffer);

    message_string_hum1 = String(hum_value);
    message_string_hum1.toCharArray(g_mqtt_message_buffer, message_string_hum1.length()+1);
    client.publish(g_hum1_mqtt_topic,g_mqtt_message_buffer);
    
    Serial.println("Done");
  }
  #endif

  #if REPORT_MQTT_JSON
  if (dht_reading_taken){
    sprintf(g_mqtt_message_buffer, "{\"ESP1\":{\"DHT1\":%i}}", temp_value);
    client.publish(g_mqtt_json_topic,g_mqtt_message_buffer);
  }
  #endif

}

/* Report the latest values to serial */
void reportToSerial(){
  if (true == dht_reading_taken){
    Serial.print("DHT1:");
    Serial.println(String(temp_value));
  }
}

bool initWifi(){
  if (WiFi.status() == WL_CONNECTED){
    WiFi.disconnect();
  }
  WiFi.setAutoConnect(false);
}


void reconnectMqtt() {
  char mqtt_client_id[30];
  sprintf(mqtt_client_id, "esp8266-%x", ESP.getChipId());

  // Loop until we're reconnected
  Serial.print("Connecting to MQTT Broker");
  while (!client.connected()){
    Serial.print(".");
    if (client.connect(mqtt_client_id, "eric", "eric")){
      Serial.println("Connected");
      sprintf(g_mqtt_message_buffer, "Device %s starting up", mqtt_client_id);
      client.publish(status_topic, g_mqtt_message_buffer);
    } else{
      delay(5000);
    }
  }
}

void mqttCallback(char* topic, byte* payload, uint8_t length){
  
  /*
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("]");
  for (int i=0; i<length; i++){
    Serial.print((char)payload[i]);
  }
  Serial.println();
  */
}
