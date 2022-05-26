/* WiFi */
const char* ssid      = "AREC_WL";          // WiFi SSID
const char* password  = "AREC_WL_PASSWD";   // WiFi Password

/* MQTT */
const char* mqtt_broker       = "192.168.2.110";    // IP address of your MQTT broker
#define REPORT_MQTT_SEPERATE  true              // Report each value to its own topic
#define REPORT_MQTT_JSON      false              // Report all value in a JSON message
const char* status_topic      = "events";       // MQTT topic to report startup

/* ESP */
uint32_t    esp_report_period   =   30;         // seconds between reports

/* Serial */
#define     SERIAL_BAUD_RATE    115200          // Speed for USB serial console


/* -------For ESP8266-------- */
#define   MODE_BUTTON_PIN   D3    // GPIO0 Pushbutton to GND
#define   ESP_WAKEUP_PIN    D0    // To reset ESP8266 after deep sleep
