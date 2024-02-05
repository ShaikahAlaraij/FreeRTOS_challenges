
#if CONFIG_FREERTOS_UNICORE
 static const BaseType_t app_cpu = 0;
 #else
 static const BaseType_t app_cpu = 1;
 #endif


//Settings

static const char command[] = "avg";   //command
static const uint16_t timer_divider = 8; //divide 80 MHz by this value
static const uint64_t timer_max_count = 1000000; //timer counts to this value
static const uint32_t cli_delay = 20;  //ms delay
enum {BUF_LEN = 10};  //Number of elements in sample buffer
enum {MSG_LEN = 100}; //Max characters in message body 
enum {MSG_QUEUE_LEN = 5}; //Number of slots in message queue
enum {CMD_BUF_LEN = 255}; //Number of characters in command buffer

//pins
static const int adc_pin = A0;

//Message struct to wrap strings for queue
typedef struct Message {
  char body[MSG_LEN];
}Message;

//Globals
static hw_timer_t *timer = NULL;
static TaskHandle_t processing_task = NULL;
static SemaphoreHandle_t sem_done_reading = NULL;
static portMUX_TYPE spinlock = portMUX_INITIALIZER_UNLOCKED;
static QueueHandle_t msg_queue;
static volatile uint16_t buf_0[BUF_LEN]; //one buffer in the pair
static volatile uint16_t buf_1[BUF_LEN]; //The other buffer in the pair 
static volatile uint16_t* write_to = buf_0; //Double buffer write pointer
static volatile uint16_t* read_from = buf_1; //double buffer read pointer
static volatile uint8_t buf_overrun = 0;  //double buffer overrun flag
static float adc_avg;
/**************************************************************/
// Functions that can be called from anywhere (in this file)

// Swap the write_to and read_from pointers in the double buffer
// Only ISR calls this at the moment, so no need to make it thread-safe
void IRAM_ATTR swap() {
  volatile uint16_t* temp_ptr = write_to;
  write_to = read_from;
  read_from = temp_ptr;
}
/*****************************************************************/
//Interrupt Service Routine ISR
// this function only executes when timer reaches max (and resets)

  void IRAM_ATTR onTimer(){

    static uint16_t idx = 0;
    BaseType_t task_woken = pdFALSE;

    //if buffer isn't overrun, read ADC to next buffer element. 
    //if buffer is overrun, drop the sample. 

    if ((idx < BUF_LEN) && (buf_overrun == 0)){
      write_to[idx] = analogRead (adc_pin);
      idx++;
    }

   
      if (xSemaphoreTakeFromISR (sem_done_reading, &task_woken) == pdFALSE){
        buf_overrun = 1;
      }

      //only swap buffers and notify task if overrun flag is cleared

      if (buf_overrun == 0){

        //reset index and swap buffer pointers
        idx = 0;
        swap();

        // a task notification works like a binary semaphore but is faster
        vTaskNotifyGiveFromISR( processing_task, &task_woken);
        
      }
    

    // exit from ISR (ESP-IDF)
    if (task_woken){
      portYIELD_FROM_ISR();
    }
  }
  
/****************************************************************/

//Tasks

//Serial terminal tasks
void doCLI (void *parameters){

  Message rcv_msg;
  char c; 
  char cmd_buf[CMD_BUF_LEN];
  uint8_t idx = 0;
  uint8_t cmd_len = strlen(command);

  // clear whole buffer
  memset(cmd_buf, 0, CMD_BUF_LEN);

  //loop forever

  while(1){

    //look for any error messages that need to be printed 

    if (xQueueReceive(msg_queue, (void*)&rcv_msg, 0) == pdTRUE){
      Serial.println(rcv_msg.body);
    }

    //read characters from serial 
    if (Serial.available() > 0){
    c = Serial.read();

    //store received character to buffer if not over buffer limit 
    if (idx < CMD_BUF_LEN - 1) {
      cmd_buf[idx] = c;
      idx++;
    }

    //print newline and check input on 'enter'

    if ((c == '\n') || (c == '\r')){

      //print newline to terminal
      Serial.print("\r\n");


      //print average value if command given is "avg"
      cmd_buf[idx - 1] = '\0';
      if (strcmp(cmd_buf, command) == 0){
        Serial.print(" Average: ");
        Serial.println(adc_avg);
      }

      //reset receive buffer and index counter
      memset(cmd_buf, 0, CMD_BUF_LEN);
      idx = 0;

      //otherwise, echo character back to serial terminal 
    } else {
      Serial.print(c);
    }
  }

  //don't hog the cpu. yield to other tasks for a while 
  vTaskDelay( cli_delay / portTICK_PERIOD_MS);
}
}

//wait for semaphore and calculate average of ADC values 
void calcAverage( void *parameters){

  Message msg;
  float avg;

  //loop forever, wait for semaphore, and print value 

  while(1){

  //wait for notification from ISR (similar to binary semaphore)
  ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

  // calculate average (as floating point value)
   avg = 0.0;
  for( int i= 0; i < BUF_LEN; i++){
    avg += (float)read_from[i];

    //vTaskDelay(105/ portTICK_PERIOD_MS); //to test overrun flag
  }
  avg /= BUF_LEN;

  //Updating the shared float may or may not take multiple instructions 
  //we protect it with a mutex or critical section. 
  //the ESP-IDF critical section is the easiest for this application 
  portENTER_CRITICAL(&spinlock);
  adc_avg = avg;
  portEXIT_CRITICAL(&spinlock);

  // if we took to long to process, buffer writing will have overrun. 
  // so, we need a message to be printed out to the serial terminal 

  if (buf_overrun == 1){
    strcpy(msg.body, "Error: Buffer overrun. Samples have been dropped.");
    xQueueSend(msg_queue, (void *)&msg, 10);
  }

  // clearing the overrun flag and giving the "done reading" semaphore must
  //be done together without being interrupted

  portENTER_CRITICAL(&spinlock);
  buf_overrun = 0;
  xSemaphoreGive(sem_done_reading);
  portEXIT_CRITICAL(&spinlock);
} 
}

/***********************************************************/

void setup() {
  
  Serial.begin(115200);

  vTaskDelay(1000/ portTICK_PERIOD_MS);
  Serial.println();
  Serial.println(" FreeRTOS Sample and process demo");

  //create semaphore before it's used (in task or isr
  sem_done_reading = xSemaphoreCreateBinary();

  //Force reboot if we can't create the semaphore
  if (sem_done_reading == NULL){
    Serial.println("Couldn't create one or more semaphores");
    ESP.restart();
  }
 // we want the done reading semaphore to initialize to 1
 xSemaphoreGive(sem_done_reading);

 //create message queue before it's used

 msg_queue = xQueueCreate(MSG_QUEUE_LEN, sizeof(Message));

 //start task to handle command line interface events. lets set it at a higher priority but only run it once every 20 ms

 xTaskCreatePinnedToCore( doCLI,
                          "Do CLI", 
                          1024,
                          NULL,
                          2,
                          NULL,
                          app_cpu);

  //start task to calculate average. save handle for use with notifications

  xTaskCreatePinnedToCore( calcAverage,
                            "Calculate average",
                            1024,
                            NULL,
                            1,
                            &processing_task,
                            app_cpu);




      // start a timer to run ISR every 100 ms

      timer = timerBegin(0, timer_divider, true);
      timerAttachInterrupt(timer, &onTimer, true);
      timerAlarmWrite(timer, timer_max_count, true);
      timerAlarmEnable(timer);

      //Delete "setup and loop" task
      vTaskDelete(NULL);
                         

}

void loop() {
  // put your main code here, to run repeatedly:

}
