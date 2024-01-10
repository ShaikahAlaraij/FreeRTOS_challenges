 #if CONFIG_FREERTOS_UNICORE
 static const BaseType_t app_cpu = 0;
 #else
 static const BaseType_t app_cpu = 1;
 #endif

static const uint8_t buf_len = 255; //buffer length

static char *msg_ptr = NULL; 
static volatile uint8_t msg_flag = 0; //0 before allocating new heap, set to 1 after done copying the msg to the heap 


// tasks


void serialRead(void *parameters){

  char c;
  char buf[buf_len];
  uint8_t idx = 0;

  memset(buf, 0, buf_len); //Clear buffer
  
  while (1){
  //Read String / characters from serial
  if (Serial.available()>0){
    c = Serial.read();
   
  // store the received characters in buffer *if not over its limit
    if (idx < buf_len-1){
      buf[idx]=c;
      idx++;
    }
   // Upon receiving a newline "\n" character, the task allocates a new section of heap memory
  //stores the string up to the new line in that section of heap
    
  if (c == '\n'){

    buf[idx-1]= '\0';
    
    if (msg_flag ==0){
      msg_ptr = (char *) pvPortMalloc(idx * sizeof(char)); 
       
      configASSERT(msg_ptr); // if msg_prt = 0 >> configASSERT() is triggered to trap user errors 
      
      memcpy(msg_ptr, buf, idx); //to copy msg from stack to heap?
      
      msg_flag = 1;//to notify task2 that the msg is ready
       }
      // Reset the receive buffer and index counter
      memset (buf, 0, buf_len);
      idx=0;
    }
    }
  }
  }

  void Printmsg ( void *parameters){
     while(1){
      
// print msg whenever flag is set "1"
      if (msg_flag == 1){
        Serial.println(msg_ptr);
        
     // give amount of free heap
       Serial.print("Free heap (bytes): ");
       Serial.println(xPortGetFreeHeapSize());
       
     //free buffer, set pointer to null, and clear flag
       vPortFree(msg_ptr);
       msg_ptr = NULL;
       msg_flag=0;

      }
     }
    
  }


  //********************************************************************************




void setup() {
  
 Serial.begin(115200);

 vTaskDelay(1000/ portTICK_PERIOD_MS);
 Serial.println();
 Serial.println("----FreeRTOS HEAP Demo----");
 Serial.println("Enter a string");

 xTaskCreatePinnedToCore( serialRead,
                          "serial Read",
                            1024,
                            NULL,
                              1, 
                            NULL,
                            app_cpu);


 xTaskCreatePinnedToCore (Printmsg,
                          "Print msg", 
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
