#include <ESP32Time.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <EEPROM.h>

#include "time.h"
#include "sntp.h"


#define uS_TO_S_FACTOR 1000000ULL


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
const char* topic = "";

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
  Serial.print("######  ------------  TRY WIFI CONNECTION --------  ##############");
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
  if(WiFi.status() == WL_CONNECTED){
          Serial.println("");
          Serial.println(" #################### # WiFi connected # #####################");
          Serial.println("IP address: ");
          Serial.println(WiFi.localIP());
  }else if(count>=timeout_wifi){
          Serial.println(" #################### # WiFi ERROR !!!!!!! # #####################");
  }

  


  
 
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

}

// intenta reconectar el cliente mqtt
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
      delay(500);
    }
  }
}

struct tm time_to_wake_up;

void setup() {
        
        //Aquí debe tomar la muestra ////////////##################################//////////////////////////////
      

  
        configTime(gmtOffset_sec, daylightOffset_sec, ntpServer1, ntpServer2);
        if(!firstBoot){
          time_to_wake_up = rtc.getTimeStruct();

          ////////////////////////////// get time to tag the measures
        
          buf_data = rtc.getTime("%d/%m/%Y  %H:%M:%S");
          
        // contruir buf data con time tag y medicion

        }
  
        

        Serial.begin(115200);
        Serial.println("############$$$$$$$$$$$$$$$$$$$$############$$$$$$$$$$$$##########$$$$$$$$$$$$");
        Serial.println("############$$$$$$$$$$$$$$$$$$$$############$$$$$$$$$$$$##########$$$$$$$$$$$$");
        Serial.println("############$$$$$$$$$$$$$$$$$$$$############$$$$$$$$$$$$##########$$$$$$$$$$$$");
        Serial.print(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>Wake up Time: ");
          Serial.println(buf_data);
        
        
        
        
        
        
        
        

        
        if(firstBoot){
          
            Serial.println("#############################     FIRST BOOT      ##########################");
            firstBoot = false;
            /*Solo si es la primera vez intentará indefinidamente conectarse a 
             * Wifi para poder actualizar el RTC interno mediante NTP
             */
            while(WiFi.status() != WL_CONNECTED){
              try_wifi();
            }
            //Actualiza los relojes de acuerdo a los servicios NTP
            time_NTP_update();
            //Si no es la primera vez intenta conectarse a wifi, pero un timeout asegura no bloquear mas de eso
        }else{
          try_wifi();
        }

        
        if(WiFi.status() == WL_CONNECTED){
          Serial.println(" $$$$$$$$$$$$$$$$ TRY SEND DATA TO MQTT BROKER ==================== <<<<<<<<<<<");
          try_mqtt();
          
        }else{
            Serial.println(" %%%%%%%%%%%%%%%%%%%%%%% SAVE DATA IN NO VOLATILE MEMORY $$$$$$$$$$$$$$$$$$");
            EEPROM.begin(500);
            int cant = int(EEPROM.read(0));
            //Si no hay muestras, empieza a guardarlas desde la segunda posicion de la memoria
            // la primera guarda la cantidad de muestras guardadas
            EEPROM.write(1+cant,rtc.getMinute());
            EEPROM.write(0,cant+1);

            Serial.printf("queue size: [%d]\n\n", cant);
            
  
            EEPROM.commit();
        }

        client.disconnect();
        
        

        ////////////////////////////Calcula el tiempo restante para dormir
        Serial.print(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>Time at sleep:");
        Serial.println(rtc.getTime("%d/%m/%Y  %H:%M:%S"));
        
        struct tm time_at_sleep = rtc.getTimeStruct();
       
        
        //goto sleep
        

        int seco = 60 - time_at_sleep.tm_sec;
        
        int t = seco* uS_TO_S_FACTOR;

        Serial.println("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX");
        
        Serial.print("\n\n Durmiendo por: ");
        Serial.println(t/1000000);

        Serial.println("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX");

        
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
        client.setServer(mqtt_server, 1883);
        client.setCallback(callback);
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
                Serial.println("\n------------------ MQTT BROKER CONNECTION SUCCESSFULLY !!!!!!!");
                //conection is ready to get and send data
                ////////////////////////////Aquí debería vaciar el buffer FIFO////////////////////////////////////
                
                client.loop();

                

                //vaciando el buffer
                uint8_t cant = EEPROM.read(0);
                if(cant>0){
                        Serial.println("\n Vaciando buffer");
                        Serial.printf("Cantidad: [%i]\n",cant);

                        
                        for(int i=0; i<cant; i++){
                              //Si la cantidad de tiempo restante 
                          
                              Serial.println(int(EEPROM.read(cant-i)));
                              char buf[2];
                              itoa(int(EEPROM.read(i+1)), buf, 10);
                              client.publish(topic,buf);
   
                        }
                        cant = 0;
                        EEPROM.write(0,cant);
                        EEPROM.commit();

                }

                //Deberia publicar ahora la última muestra/////////////////////////////
                
                //publica la fecha
                client.publish("esp32/ttgo", buf_data.c_str());
                
          }else{
            Serial.println("\n ---------------  ERROR ENVIANDO POR MQTT   !!!!");
  
            //Añadir  la muestra a la cola FIFO
            
  
            int cant = int(EEPROM.read(0));
            //Si no hay muestras, empieza a guardarlas desde la segunda posicion de la memoria
            // la primera guarda la cantidad de muestras guardadas
            EEPROM.write(1+cant,rtc.getMinute());
            EEPROM.write(0,cant+1);
  
            EEPROM.commit();
  
          }
}
