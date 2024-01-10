
// Use only core 1 for demo purposes
 #if CONFIG_FREERTOS_UNICORE
 static const BaseType_t app_cpu = 0;
 #else
 static const BaseType_t app_cpu = 1;
 #endif


// Pins
#define LED_BUILTIN 2
static const int led_pin = LED_BUILTIN;

//globals
static int led_delay = 500;   //ms

//Tasks
void READ(void  *parameter){
 while(1){


  // Loop forever
  while (Serial.available()>0) {

 long datain = Serial.parseInt();
  led_delay = datain;
  Serial.print("Updated LED delay to: ");
        Serial.println(led_delay);

        Serial.setTimeout(2000);
 
   
  }}}

 void ToggleLED (void *parameter ){
    while(1){
     digitalWrite(led_pin, HIGH);
     vTaskDelay(led_delay /portTICK_PERIOD_MS);
   digitalWrite(led_pin, LOW);
    vTaskDelay(led_delay /portTICK_PERIOD_MS);
 }}
      
    
void setup() {

    // Configure pin
  pinMode(led_pin, OUTPUT);

  // Configure serial and wait a second
  Serial.begin(115200);
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  Serial.println("Multi-task LED Demo");   // not printed?
  Serial.println("Enter a number in milliseconds to change the LED delay.");  // not printed?


   xTaskCreatePinnedToCore(  // Use xTaskCreate() in vanilla FreeRTOS
            ToggleLED,    // Function to be called
          "ToggleLED", // Name of task
            1024,         // Stack size (bytes in ESP32, words in FreeRTOS)
           NULL,         // Parameter to pass to function
             1,            // Task priority (0 to configMAX_PRIORITIES - 1)
          NULL,         // Task handle
          app_cpu);     // Run on one core for demo purposes (ESP32 only)

 xTaskCreatePinnedToCore(  // Use xTaskCreate() in vanilla FreeRTOS
              READ,    // Function to be called
             "READ", // Name of task
             1024,         // Stack size (bytes in ESP32, words in FreeRTOS)
             NULL,         // Parameter to pass to function
             1,            // Task priority (0 to configMAX_PRIORITIES - 1)
            NULL,         // Task handle
          app_cpu);     // Run on one core for demo purposes (ESP32 only)




          vTaskDelete(NULL);

}

void loop() {


  
}
