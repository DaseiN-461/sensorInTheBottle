#define uS_TO_S_FACTOR 1000000ULL
#define TIME_TO_SLEEP 60

void setup() {
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);

  esp_deep_sleep_start();
}

void loop() {
  // put your main code here, to run repeatedly:

}
