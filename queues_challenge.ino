 #if CONFIG_FREERTOS_UNICORE
 static const BaseType_t app_cpu = 0;
 #else
 static const BaseType_t app_cpu = 1;
 #endif



 #define LED_BUILTIN 2
 static const int led_pin = LED_BUILTIN;
 
 static  QueueHandle_t xQueue1;
 static  QueueHandle_t xQueue2;

 static const uint8_t buf_len = 255;
 static uint8_t idx = 0; 
 static const char CM[] = "delay ";
 static const uint8_t Blinks = 100;
 
//struct to store elements of different data types under one data type
 typedef struct MSG{ 
  char Mcharacters [10];
  int count;
  } MSG;
  

  
 
//tasks

void TaskA (void *parameters){

  MSG RECEIVED;
  char c;
  char buf[buf_len]; 
  uint8_t CM_len = strlen(CM); //strlen() to return the length of the string "delay "
  int t; // led delay variable
  
  memset(buf, 0, buf_len); // reset buffer

  while(1){
 // to check if there is a message in xQueue2
   if( xQueueReceive ( xQueue2, (void*) &RECEIVED, 0) == pdTRUE){

    Serial.print (RECEIVED.Mcharacters);
    Serial.println(RECEIVED.count);
    
   }
 // read characters from serial
   if(Serial.available()>0){ 
    c = Serial.read();

 // store the received characters *if it doesn't exceed the buffer limit
  
    if(idx < buf_len - 1){
      buf[idx]=c;
      idx++;
    }
    
// print newline and check input after pressing 'enter'

    if (c == '\n'){ 
      
      Serial.print("\n");
      // check if the first 6 characters are "delay "
      if( memcmp( buf, CM, CM_len)== 0){
        
        char* tail = buf + CM_len;
        //used atoi() to convert from char to int
        t = atoi(tail);
        // take the absolute value to get a +ve integer
        t = abs (t);
        
        // send t (led delay) to task B
        xQueueSend(xQueue1, (void*) &t, 10);
      }
     // reset buffer and index counter
      memset(buf, 0, buf_len);
      idx = 0;
    } 
    //else, echo character back to serial terminal
    else {
      Serial.print(c);
    }
   }
  }
}

void TaskB (void *parameters){
 MSG SENT; 
 int t = 500;
 uint8_t counter = 0;

 pinMode(led_pin, OUTPUT);
 
  while(1){
     xQueueReceive(xQueue1,(void*)  &t,10);
     
     digitalWrite(led_pin, HIGH);
     vTaskDelay( t/ portTICK_PERIOD_MS);
     digitalWrite(led_pin, LOW);
     vTaskDelay( t/ portTICK_PERIOD_MS);
  counter++;
  // when blinked 100 times, send message to task A
 if (counter>= Blinks){
  
  strcpy(SENT.Mcharacters, "BLINKED");
  SENT.count = counter;
  xQueueSend(xQueue2, (void*)&SENT, 10);
  // reset counter
  counter = 0;
    }
  } 
}





void setup() {
  

  Serial.begin(115200);

  
  vTaskDelay(1000/ portTICK_PERIOD_MS);
  Serial.println();
  Serial.println("QUEUE CHALLENGE");
  Serial.println("Enter your desired blinking rate as follows: ");
  Serial.println(" * 'delay xxx' where 'xxx' is your desired blinking rate in MS");
  
// queues creation
  xQueue1 = xQueueCreate(10, sizeof(int));
  xQueue2 = xQueueCreate(10, sizeof(MSG));
  
  xTaskCreatePinnedToCore(
    TaskA,
    "Task A",
    1024,
    NULL,
    1,
    NULL,
    app_cpu);


  xTaskCreatePinnedToCore(
    TaskB,
    "Task B",
    1024,
    NULL,
    1,
    NULL,
    app_cpu);


    vTaskDelete(NULL);
  
  
}

void loop() {
  // put your main code here, to run repeatedly:

}
