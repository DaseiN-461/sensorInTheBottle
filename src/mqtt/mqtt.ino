#include <ESP32Time.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <EEPROM.h>
#include <DHT.h>

//#include "time.h"
//#include "sntp.h"

#define DHTTYPE DHT22   
#define DHTPIN 2 

DHT dht(DHTPIN, DHTTYPE);

#define uS_TO_S_FACTOR 1000000ULL
#define timeout_wifi 10 // 10 intentos cada 100 ms
#define timeout_mqtt 5 // 5 intentos cada 500 ms


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
const char* topic = "esp32/temperature";




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
  while (WiFi.status() != WL_CONNECTED and count<timeout_wifi) {
    delay(100);
    Serial.print(".");
    
    count += 1;
  }
  
  if(WiFi.status() == WL_CONNECTED){
          Serial.println("");
          Serial.println(" #################### # WiFi connected # #####################");
          Serial.println("IP address: ");
          Serial.println(WiFi.localIP());
  }else{
          Serial.println(" #################### # WiFi ERROR !!!!!!! # #####################");
  }

  


  
 
}

void callback(char* topic, byte* message, unsigned int length) {
  /*
   * Para implementar una función que gestione los mensajes de entrada por mqtt
   * 
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");

  //crea el messageTemp con los datos entrantes
  String messageTemp;
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();
*/
}

// intenta reconectar el cliente mqtt debería demorar unos 500 ms
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

String get_measures(){
        dht.begin();
        delay(10);

        float hum = dht.readHumidity();
        float temp = dht.readTemperature();
      
        if (isnan(hum) || isnan(temp)) {
          Serial.println("Error al leer el sensor DHT22");
        } else {
          Serial.print("Humedad: ");
          Serial.print(hum);
          Serial.print(" %, Temperatura: ");
          Serial.print(temp);
          Serial.println(" °C ");
        }

    
        
        String measures;
        measures = "temp: " + String(temp,1) + ", hum: " + String(hum,1);
        
        return measures;

        
}



void setup() {
        
        //Aquí debe tomar la muestra ////////////##################################//////////////////////////////

        

        struct tm time_to_wake_up;

        
        String str_wakeup_time;
        struct tm tm_wakeup_time;


        String frame;
        
        if(!firstBoot){
          
          
          String measures = get_measures();
          
          
          tm_wakeup_time = rtc.getTimeStruct();

          ////////////////////////////// get time to tag the measures
        
          str_wakeup_time = rtc.getTime("%d/%m/%Y  %H:%M:%S");
          
        // contruir buf data con time tag y medicion
          frame = "["+str_wakeup_time+"][" + measures+"]";
        }

      
  
        

        Serial.begin(115200);
        Serial.println("############$$$$$$$$$$$$$$$$$$$$############$$$$$$$$$$$$##########$$$$$$$$$$$$");
        Serial.println("############$$$$$$$$$$$$$$$$$$$$############$$$$$$$$$$$$##########$$$$$$$$$$$$");
        Serial.println("############$$$$$$$$$$$$$$$$$$$$############$$$$$$$$$$$$##########$$$$$$$$$$$$");
        Serial.print(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>Wake up Time: ");
        Serial.println(str_wakeup_time);     

        
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
          try_mqtt(frame);
          
        }else{

            //Esto es solo por mientras, guarda el minuto en la eeprom para simular que guarda la
            // estructura en una memoria no voltatil
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
        

        int t = 0;
        int minu = 0;
        
        if(time_at_sleep.tm_min%10 != 0){
          minu = 10 - (time_at_sleep.tm_min%10) - 1;
        }else{
          minu = 9;
        }


        int seco = 60 - time_at_sleep.tm_sec;

        t = (seco* uS_TO_S_FACTOR)+(minu*60*uS_TO_S_FACTOR);
   

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
        configTime(gmtOffset_sec, daylightOffset_sec, ntpServer1, ntpServer2);
        
        struct tm timeinfo;
      
        while(!getLocalTime(&timeinfo)){
                Serial.println("No time available (yet)");
        }
        rtc.setTimeStruct(timeinfo);
        Serial.println("NTP update successfully");
        NPT_update = true;
        
}

void try_mqtt(String bufferToSendMQTT){
        client.setServer(mqtt_server, 1883);
        client.setCallback(callback);
  //////////////////////////// Intenta connectarse a broker mqtt  ////////////////////////////////////
        //publish data by mqtt
        int count = 0;
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
                client.publish(topic, bufferToSendMQTT.c_str());
                
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
