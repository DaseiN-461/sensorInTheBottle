#include <ESP32Time.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <EEPROM.h>

#include "time.h"
#include "sntp.h"

#define MINU_TO_SLEEP 5
#define uS_TO_S_FACTOR 1000000ULL
#define TIME_TO_SLEEP MINU_TO_SLEEP*60 //in seconds,      1800 = 30 minutes

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


String buf_data;

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
  int timeout_wifi = 10;
  while (WiFi.status() != WL_CONNECTED and count<timeout_wifi) {

    //////////////////////////////////////////////////////////////////////////////
    // debugging, for no conection loop
    //////////////////////////////////////////////////////////////////////////////
    
    delay(100);
    Serial.print(".");
    
    count += 1;

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

struct tm time_to_wake_up;

void setup() {
        configTime(gmtOffset_sec, daylightOffset_sec, ntpServer1, ntpServer2);
        if(!firstBoot){
          time_to_wake_up = rtc.getTimeStruct();

          ////////////////////////////// get time to tag the measures
        
          buf_data = rtc.getTime("%d/%m/%Y  %H:%M:%S");

        // contruir buf data con time tag y medicion

        }
  
        

        Serial.begin(115200);
        Serial.print("Wake up time: ");
        Serial.println(buf_data);
        
        pinMode(4,OUTPUT);
        digitalWrite(4,HIGH);
        
        
        
        
        
        client.setServer(mqtt_server, 1883);
        client.setCallback(callback);

        
        if(firstBoot){
          
            Serial.println("FIRST BOOT...");
            firstBoot = false;
            while(WiFi.status() != WL_CONNECTED){
              try_wifi();
            }
            
            time_NTP_update();
        }else{
          try_wifi();
        }

      
        

     
        
        
      
        
       
     
        
        

        
        
        
 
        


        if(WiFi.status() == WL_CONNECTED){
          try_mqtt();
          
        }else{
          EEPROM.begin(20);
          int cant = int(EEPROM.read(0));
            //Si no hay muestras, empieza a guardarlas desde la segunda posicion de la memoria
            // la primera guarda la cantidad de muestras guardadas
            EEPROM.write(1+cant,rtc.getMinute());
            EEPROM.write(0,cant+1);
  
            EEPROM.commit();
        }

        
        
        

        ////////////////////////////Calcula el tiempo restante para dormir
        Serial.print("Time at sleep: ");
        Serial.println(rtc.getTime("%d/%m/%Y  %H:%M:%S"));
        
        struct tm time_at_sleep = rtc.getTimeStruct();
       
        
        //goto sleep
        int t = 0;
        int minu = 0;
        if(time_at_sleep.tm_min%10 != 0){
          minu = 10 - (time_at_sleep.tm_min%10) - 1;
        }else{
          minu = 9;
        }

        int seco = 60 - time_at_sleep.tm_sec;
        
        t = (seco* uS_TO_S_FACTOR)+(minu*60*uS_TO_S_FACTOR);
        Serial.print("\n\n Durmiendo por: ");
        Serial.println(t/1000000);
        esp_sleep_enable_timer_wakeup(t);
        esp_deep_sleep_start();

}

void loop() {
    
}

void time_NTP_update(){       
               
        struct tm timeinfo;
      
        while(!getLocalTime(&timeinfo)){
                Serial.println("No time available (yet)");
        }
        rtc.setTimeStruct(timeinfo);
        Serial.println("NTP update successfully");
        NPT_update = true;
        
}

void try_mqtt(){
  //////////////////////////// Intenta connectarse a broker mqtt  ////////////////////////////////////
        //publish data by mqtt
        int count = 0;
        int timeout_mqtt = 5;
        while(count<timeout_mqtt and !client.connected()){
          reconnect();
          count += 1;
        }

        
        EEPROM.begin(20);
          if(client.connected()){
                //conection is ready to get and send data
                ////////////////////////////Aquí debería vaciar el buffer FIFO////////////////////////////////////
                digitalWrite(4,LOW);
                client.loop();

                

                //vaciando el buffer
                uint8_t cant = EEPROM.read(0);
                if(cant>0){
                        Serial.println("\n vaciar buffer");
                        Serial.print("Cantidad: ");
                        Serial.print(cant);
                        Serial.println();

                        for(int i=0; i<cant; i++){
                          Serial.println(int(EEPROM.read(cant-i)));
                              char buf[2];
                              itoa(int(EEPROM.read(i+1)), buf, 10);
                              client.publish("esp32/temperature",buf);
                              
                          
                        }
                        cant = 0;
                        EEPROM.write(0,cant);
                        EEPROM.commit();

                }

                //Deberia publicar ahora la última muestra/////////////////////////////
                
                //publica la fecha
                client.publish("esp32/temperature", buf_data.c_str());
                
          }else{
            Serial.println("\n ERROR ENVIANDO POR MQTT");
  
            //Añadir  la muestra a la cola FIFO
            
  
            int cant = int(EEPROM.read(0));
            //Si no hay muestras, empieza a guardarlas desde la segunda posicion de la memoria
            // la primera guarda la cantidad de muestras guardadas
            EEPROM.write(1+cant,rtc.getMinute());
            EEPROM.write(0,cant+1);
  
            EEPROM.commit();
  
          }
        
        
        


        // SUCCESSFULLY ADVERTISMENT
        bool flag = false;
        for(int i=0; i<6; i++){
              flag = !flag;
              digitalWrite(4,flag);
              delay(500);
        }
        digitalWrite(4,HIGH);
}
