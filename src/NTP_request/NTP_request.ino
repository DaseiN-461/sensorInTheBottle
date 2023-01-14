#include <WiFi.h>
#include "time.h"
#include "sntp.h"

#define uS_TO_S_FACTOR 1000000ULL
#define TIME_TO_SLEEP 30 //1800 = 30 minutes

const char* ssid       = "spending_net";
const char* password   = "spending_pass";

const char* ntpServer1 = "pool.ntp.org";
const char* ntpServer2 = "time.nist.gov";
const long  gmtOffset_sec = -3*3600;
const int   daylightOffset_sec = 0;

const char* time_zone = "CET-1CEST,M3.5.0,M10.5.0/3";  // TimeZone rule for Europe/Rome including daylight adjustment rules (optional)

void printLocalTime()
{
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("No time available (yet)");
    return;
  }
  
  /*
  if(timeinfo.tm_hour < 4){
    int diff = abs(timeinfo.tm_hour-4);
    timeinfo.tm_hour = 24-diff;
  }else{
    timeinfo.tm_hour -= 4;
  }
  */
  int h = timeinfo.tm_hour;
  int m = timeinfo.tm_min;

  char buf_h[4];
  char buf_m[4];

  itoa(h,buf_h,10);
  itoa(m,buf_m,10);

  char buf[10];
  buf[0] = buf_h[0];
  buf[1] = buf_h[1];
  buf[2] = buf_h[2];
  buf[3] = buf_h[3];

  buf[4] = buf_m[0];
  buf[5] = buf_m[1];
  buf[6] = buf_m[2];
  buf[7] = buf_m[3];
  
  Serial.println(buf);
  //Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}

// Callback function (get's called when time adjusts via NTP)
void timeavailable(struct timeval *t)
{
  Serial.println("Got time adjustment from NTP!");
  printLocalTime();
}

void setup()
{
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);

  Serial.begin(115200);
  

  //sntp_set_time_sync_notification_cb( timeavailable );

  //sntp_servermode_dhcp(1);    // (optional)

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer1, ntpServer2);

  Serial.printf("Connecting to %s ", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
      delay(100);
      Serial.print(".");
  }
  Serial.println(" CONNECTED");

  printLocalTime(); 
  
  esp_deep_sleep_start();
}

void loop()
{
  Serial.println("XXXXXXX");

}
