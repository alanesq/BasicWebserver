//
//    RTOS tasks demo  - 21oct21
//
//    see: https://freertos.org/Documentation/RTOS_book.html
//         https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/freertos.html
//
//    To communicate between tasks use:   
//      Queue - this adds values to a stack which can then be read by other tasks
//              see:  https://www.youtube.com/watch?v=pHJ3lxOoWeI
//      Mutex - Protects variable from other tasks whilst being changed (mutually exclusive access)    
//              see:  https://www.youtube.com/watch?v=I55auRpbiTs
//      Semaphore - sort of shared counter?   
//              e.g. signal that there are x bytes of data ready to be read
//              see: https://www.youtube.com/watch?v=5JcMtbA9QEE


// counters used to demonstrate the various tasks running
  int counter1 = 0;
  int counter2 = 0;
  int counter3 = 0;

// this is a shared variable accessed by both a task and LOOP with access controlled via a mutex
  static int sharedVariable = 5; 
  static SemaphoreHandle_t mutex;

TaskHandle_t task1_handle = NULL;       // used to control task1


// ----------------------------------------------------------------


// show some info about a RTOS task
void taskInfo(void * parameter, int task) {
      Serial.print("Task");
      Serial.print(task);
      Serial.println(" starting");
      Serial.print("  Core=");
      Serial.println(xPortGetCoreID());
      Serial.print("  Priority=");
      Serial.println(uxTaskPriorityGet(NULL));  
      Serial.print("  Free stack=");
      Serial.println(uxTaskGetStackHighWaterMark(NULL));
      Serial.print("  Free heap=");
      Serial.println(xPortGetFreeHeapSize());
}



// ----------------------------------------------------------------
//                            -Task1
// ---------------------------------------------------------------- 
void taskOne( void * parameter ) { 

    taskInfo(parameter, 1);    // display some info about the task
  
    for(;;) {
 
        Serial.print("Task1 counter:");  
        Serial.println(counter1++);
        vTaskDelay(2000 / portTICK_PERIOD_MS);   // delay 2 seconds
        
    }
 
    Serial.println("Ending task1");
    vTaskDelete( NULL );
 
}


// ----------------------------------------------------------------
//                            -Task2
// ---------------------------------------------------------------- 
void taskTwo( void * parameter ) { 

    taskInfo(parameter, 2);    // display some info about the task

    for(;;) {
 
        Serial.print("Task2 counter:");  
        Serial.println(counter2++);

        // update shared variable using a mutex to protect it
        if (counter2 < 4 ) {
          Serial.println("Task2 decrementing shared variable");
            if (xSemaphoreTake(mutex, 5) == pdTRUE) {         // take mutex waiting max of 5 ticks
              sharedVariable--;
              xSemaphoreGive(mutex);                          // return the mutex            
            } else {
              // failed to get mutex
            }
        }

        // delete this task
        if (counter2 > 5) {
          Serial.println("Task2 is deleting its self");
          vTaskDelete(NULL);       
        }

        //if (task1_handle != NULL) vTaskSuspend(task1_handle);       // suspend task1
        vTaskDelay(3000 / portTICK_PERIOD_MS);   // delay 3 seconds
        
    }
 
    Serial.println("Ending task2");
    vTaskDelete( NULL );
 
}


// ----------------------------------------------------------------
//                            -Setup
// ---------------------------------------------------------------- 

void setup() {
  Serial.begin(115200); while (!Serial); delay(50);       // start serial comms   
  delay(300);
  Serial.println("\n\n\n\Starting\n");

  // create the mutex to control access of the shared Variable (sharedVariable)
     mutex = xSemaphoreCreateMutex();

  // create task1
  xTaskCreate(      taskOne,          // Task function 
                    "TaskOne",        // String with name of task
                    2000,             // Stack size in bytes (if too small esp will restart with "stack canary" error), 768bytes min.
                    NULL,             // Parameter to pass to the function
                    1,                // Priority of the task (1=low)
                    &task1_handle);   // Task handle

  delay(200);    // give task1 chance to write its stuff to serial port 

  // create task2
  xTaskCreate(      taskTwo, 
                    "TaskTwo", 
                    2000, 
                    NULL, 
                    1, 
                    NULL);   

  delay(200);    // give task2 chance to write its stuff to serial port                     
       
  // see xTaskCreatePinnedToCore to specify which core to run it on otherwise best one is automatically chosen
  // vTaskDele(NULL);   Putting this here would delete the SETUP and LOOP tasks
}


// ----------------------------------------------------------------
//                            -Loop
// ---------------------------------------------------------------- 

void loop() {

  delay(4000);
  Serial.print("Loop, shared variable = ");
  Serial.println(sharedVariable);     // the mutex controlled shared variable (not sure if you need to protect reads?)

  counter3++;
  
  if (counter3 == 2) {
    Serial.println("Loop is suspending task1");
    if (task1_handle != NULL) vTaskSuspend(task1_handle);       // suspend task1
  }

  if (counter3 == 5) {
    Serial.println("Loop is restarting task1");
    if (task1_handle != NULL) vTaskResume(task1_handle);        // restart task1  
  }

  if (counter3 == 10) {
    Serial.println("Loop is deleting task1");
    if (task1_handle != NULL) {
      vTaskDelete(task1_handle);     
      task1_handle = NULL;
    }
  }  
 
}

// ---------------------------------------------------------------- 


//// ----------------------------------------------------------------
////                  -suspend all other tasks demo
//// ---------------------------------------------------------------- 
//// Note: this doesn't work - it causes the esp32 to restart!
//
//void important() {
//  
//  vTaskSuspendAll();                           // suspend all tasks (including loop)
//
//    Serial.println("Stopping all other tasks");
//    // do stuff here
//    Serial.println("Resuming other tasks");
//  
//  xTaskResumeAll();                            // resume
//
//}
