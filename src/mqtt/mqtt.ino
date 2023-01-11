#include <ESP32Time.h>
#include <WiFi.h>
#include <PubSubClient.h>

#include "time.h"
#include "sntp.h"

#define uS_TO_S_FACTOR 1000000ULL
#define TIME_TO_SLEEP 60 //in seconds,      1800 = 30 minutes

RTC_DATA_ATTR bool firstBoot = true;
RTC_DATA_ATTR bool NPT_update = false;

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


char dataString[6];

WiFiClient espClient;
PubSubClient client(espClient);
ESP32Time rtc;



void try_wifi() {
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  
  WiFi.begin(ssid, password);
  int count = 0;
  while (WiFi.status() != WL_CONNECTED) {

    //////////////////////////////////////////////////////////////////////////////
    // debugging, for no conection loop
    //////////////////////////////////////////////////////////////////////////////
    count += 1;
    delay(500);
    Serial.print(".");
    if(count == 10){
      bool flag = false;
      for(int i=0; i<12; i++){
        flag = !flag;
        digitalWrite(4,flag);
        delay(50);
      }
      esp_restart();
    }
    //////////////////////////////////////////////////////////////////////////////
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
      Serial.println(" try again in 1 seconds");
      // Wait 1 seconds before retrying
      delay(1000);
    }
  }
}



void setup() {
        pinMode(4,OUTPUT);
        digitalWrite(4,HIGH);
        
        configTime(gmtOffset_sec, daylightOffset_sec, ntpServer1, ntpServer2);
        
        Serial.begin(115200);
        
        client.setServer(mqtt_server, 1883);
        client.setCallback(callback);

        
      
        try_wifi();

        if(WiFi.status() == WL_CONNECTED){
                  if(firstBoot or !NPT_update){
                            time_NTP_update();
                            firstBoot = false;
                  }
        }
        
        
      
        
       
     
        
        

        
        
        
 
        ////////////////////////////// get time to tag the measures
        char bufToSend[20];
        String cadena = rtc.getTime("%d/%m/%Y  %H:%M:%S");
        strcpy(bufToSend, cadena.c_str());


        //////////////////////////// Intenta connectarse a broker mqtt  ////////////////////////////////////
        //publish data by mqtt
        int count = 0;
        int timeout_mqtt = 5;
        while(count<timeout_mqtt and !client.connected()){
          reconnect();
          count += 1;
        }
        if(client.connected()){
                //conection is ready to get and send data
                ////////////////////////////Aquí debería vaciar el buffer FIFO////////////////////////////////////
                digitalWrite(4,LOW);
                client.loop();
                client.publish("esp32/temperature", cadena.c_str());
        }else{
          Serial.println("\n ERROR ENVIANDO POR MQTT");

          //Añadir  la muestra a la cola FIFO
        }
        


        // SUCCESSFULLY ADVERTISMENT
        bool flag = false;
        for(int i=0; i<6; i++){
              flag = !flag;
              digitalWrite(4,flag);
              delay(500);
        }
        digitalWrite(4,HIGH);

        ////////////////////////////Calcula el tiempo restante para dormir
        int t = TIME_TO_SLEEP - rtc.getSecond();
        //goto sleep
        esp_sleep_enable_timer_wakeup(t * uS_TO_S_FACTOR);
        esp_deep_sleep_start();

}

void loop() {
    
}

void time_NTP_update(){              
        struct tm timeinfo;
      
        if(!getLocalTime(&timeinfo)){
                Serial.println("No time available (yet)");
        }else{
                rtc.setTimeStruct(timeinfo);
                Serial.println("NTP update successfully");
                NPT_update = true;
        }
}
