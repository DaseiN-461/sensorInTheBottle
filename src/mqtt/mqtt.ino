#include <WiFi.h>
#include <PubSubClient.h>

#include "time.h"
#include "sntp.h"

#define uS_TO_S_FACTOR 1000000ULL
#define TIME_TO_SLEEP 30 //1800 = 30 minutes

const char* ntpServer1 = "pool.ntp.org";
const char* ntpServer2 = "time.nist.gov";
const long  gmtOffset_sec = -3*3600;
const int   daylightOffset_sec = 0;

const char* time_zone = "CET-1CEST,M3.5.0,M10.5.0/3";  // TimeZone rule for Europe/Rome including daylight adjustment rules (optional)


const char* ssid = "spending_net";
const char* password = "spending_pass";

const char* mqtt_server = "mqtt.eclipseprojects.io";

long lastMsg = 0;
char msg[100];
int value = 0;
float temperature = 0;
float humidity = 0;

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  Serial.begin(115200);

  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer1, ntpServer2);

}

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
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

void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();
/*
  // Feel free to add more if statements to control more GPIOs with MQTT

  // If a message is received on the topic esp32/output, you check if the message is either "on" or "off". 
  // Changes the output state according to the message
  if (String(topic) == "esp32/output") {
    Serial.print("Changing output to ");
    if(messageTemp == "on"){
      Serial.println("on");
    }
    else if(messageTemp == "off"){
      Serial.println("off");
    }
  }
*/
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266Client")) {
      Serial.println("connected");
      // Subscribe
      client.subscribe("esp32/output");
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
  client.loop();

  long now = millis();
  if (now - lastMsg > 5000) {
    lastMsg = now;

    struct tm timeinfo;
    if(!getLocalTime(&timeinfo)){
      Serial.println("No time available (yet)");
      return;
    }
    
    char hora[4];
    itoa(timeinfo.tm_hour,hora,10);
    char minu[4];
    itoa(timeinfo.tm_min,hora,10);

    char dataString[20];
    dataString[0] = hora[0];
    dataString[1] = hora[1];
    dataString[2] = hora[2];
    dataString[3] = hora[3];
    dataString[4] = ':';
    dataString[5] = minu[0];
    dataString[6] = minu[1];
    dataString[7] = minu[2];
    dataString[8] = minu[3];
    
 

    client.publish("esp32/temperature", dataString);

  
  }
}
