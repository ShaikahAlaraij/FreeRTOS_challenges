


#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

static const int rate1 = 500;
static const int rate2 = 200;

// Pins
#define LED_BUILTIN 2
static const int led_pin = LED_BUILTIN;

//Tasks
void toggleLED(void *parameter) {
  while(1) {
    digitalWrite(led_pin, HIGH);
    vTaskDelay(rate1 / portTICK_PERIOD_MS);
    digitalWrite(led_pin, LOW);
    vTaskDelay(rate1 / portTICK_PERIOD_MS);
  }
}

void toggleLED2(void *parameter) {
  while(1) {
    digitalWrite(led_pin, HIGH);
    vTaskDelay(rate2 / portTICK_PERIOD_MS );
    digitalWrite(led_pin, LOW);
    vTaskDelay(rate2 / portTICK_PERIOD_MS);
  }
}


void setup() {

  // Configure pin
  pinMode(led_pin, OUTPUT);

  // Task to run forever
  xTaskCreatePinnedToCore(  
              toggleLED,    // Function to be called
              "Toggle LED", // Name of task
              1024,         // Stack size 
              NULL,         // Parameter to pass to function
              1,            // Task priority 
              NULL,         // Task handle
              app_cpu);     // Core

  xTaskCreatePinnedToCore(  
              toggleLED2,    // Function to be called
              "Toggle LED2", // Name of task
              1024,         // Stack size 
              NULL,         // Parameter to pass to function
              1,            // Task priority 
              NULL,         // Task handle
              app_cpu);     // Core

  
}

void loop() {

}
