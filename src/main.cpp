#include<Arduino.h>
#include <FreeRTOS_SAMD51.h>
#include <Adafruit_NeoPixel.h>

//**************************************************************************
// Type Defines and Constants
//**************************************************************************

//#define  ERROR_LED_PIN  13 //Led Pin: Typical Arduino Board
//#define  ERROR_LED_PIN  2 //Led Pin: samd21 xplained board
#define ERROR_LED_PIN 76

#define ERROR_LED_LIGHTUP_STATE  HIGH // the state that makes the led light up on your board, either low or high

// Select the serial port the project should use and communicate over
// Some boards use SerialUSB, some use Serial
//#define SERIAL          SerialUSB //Sparkfun Samd51 Boards
#define SERIAL          Serial //Adafruit, other Samd51 Boards

#define LED_PIN    88

// How many NeoPixels are attached to the Arduino?
#define LED_COUNT  1

// NeoPixel brightness, 0 (min) to 255 (max)
#define BRIGHTNESS 50 // Set BRIGHTNESS to about 1/5 (max = 255)

//**************************************************************************
// global variables
//**************************************************************************
TaskHandle_t Handle_aTask;
TaskHandle_t Handle_bTask;
TaskHandle_t Handle_cTask;
TaskHandle_t Handle_monitorTask;
Adafruit_NeoPixel pixel(LED_COUNT, LED_PIN, NEO_GRBW + NEO_KHZ800);

//**************************************************************************
// Can use these function for RTOS delays
// Takes into account processor speed
// Use these instead of delay(...) in rtos tasks
//**************************************************************************
void myDelayUs(int us)
{
    vTaskDelay( us / portTICK_PERIOD_US );
}

void myDelayMs(int ms)
{
    vTaskDelay( (ms * 1000) / portTICK_PERIOD_US );
}

void myDelayMsUntil(TickType_t *previousWakeTime, int ms)
{
    vTaskDelayUntil( previousWakeTime, (ms * 1000) / portTICK_PERIOD_US );
}

//*****************************************************************
// Create a thread that prints out A to the screen every two seconds
// this task will delete its self after printing out afew messages
//*****************************************************************
static void threadA( void *pvParameters )
{

    SERIAL.println("Thread A: Started");
    for(int x=0; x<100; ++x)
    {
        SERIAL.print("A");
        SERIAL.flush();
        myDelayMs(500);
    }

    // delete ourselves.
    // Have to call this or the system crashes when you reach the end bracket and then get scheduled.
    SERIAL.println("Thread A: Deleting");
    vTaskDelete( NULL );
}

//*****************************************************************
// Create a thread that prints out B to the screen every second
// this task will run forever
//*****************************************************************
static void threadB( void *pvParameters )
{
    SERIAL.println("Thread B: Started");

    while(1) {
        SERIAL.println("B");
        SERIAL.flush();
        myDelayMs(2000);
    }

}

static void threadC( void *pvParameters ) {

    SERIAL.println("Thread C: Started");
    while(1) {
        pixel.setPixelColor(0, pixel.Color(255,0,0));
        pixel.show();
        myDelayMs(1000);
        pixel.setPixelColor(0, pixel.Color(0,255,0));
        pixel.show();
        myDelayMs(1000);
        pixel.setPixelColor(0, pixel.Color(0,0,255));
        pixel.show();
        myDelayMs(1000);
    }
}

//*****************************************************************
// Task will periodically print out useful information about the tasks running
// Is a useful tool to help figure out stack sizes being used
// Run time stats are generated from all task timing collected since startup
// No easy way yet to clear the run time stats yet
//*****************************************************************
static char ptrTaskList[400]; //temporary string buffer for task stats

void taskMonitor(void *pvParameters)
{
    int x;
    int measurement;

    SERIAL.println("Task Monitor: Started");

    // run this task afew times before exiting forever
    while(1)
    {
        myDelayMs(10000); // print every 10 seconds

        SERIAL.flush();
        SERIAL.println("");
        SERIAL.println("****************************************************");
        SERIAL.print("Free Heap: ");
        SERIAL.print(xPortGetFreeHeapSize());
        SERIAL.println(" bytes");

        SERIAL.print("Min Heap: ");
        SERIAL.print(xPortGetMinimumEverFreeHeapSize());
        SERIAL.println(" bytes");
        SERIAL.flush();

        SERIAL.println("****************************************************");
        SERIAL.println("Task            ABS             %Util");
        SERIAL.println("****************************************************");

        vTaskGetRunTimeStats(ptrTaskList); //save stats to char array
        SERIAL.println(ptrTaskList); //prints out already formatted stats
        SERIAL.flush();

        SERIAL.println("****************************************************");
        SERIAL.println("Task            State   Prio    Stack   Num     Core" );
        SERIAL.println("****************************************************");

        vTaskList(ptrTaskList); //save stats to char array
        SERIAL.println(ptrTaskList); //prints out already formatted stats
        SERIAL.flush();

        SERIAL.println("****************************************************");
        SERIAL.println("[Stacks Free Bytes Remaining] ");

        measurement = uxTaskGetStackHighWaterMark( Handle_aTask );
        SERIAL.print("Thread A: ");
        SERIAL.println(measurement);

        measurement = uxTaskGetStackHighWaterMark( Handle_bTask );
        SERIAL.print("Thread B: ");
        SERIAL.println(measurement);

        measurement = uxTaskGetStackHighWaterMark( Handle_monitorTask );
        SERIAL.print("Monitor Stack: ");
        SERIAL.println(measurement);

        SERIAL.println("****************************************************");
        SERIAL.flush();

    }

    // delete ourselves.
    // Have to call this or the system crashes when you reach the end bracket and then get scheduled.
    SERIAL.println("Task Monitor: Deleting");
    vTaskDelete( NULL );

}


//*****************************************************************

void setup()
{

    SERIAL.begin(9600);
    pixel.begin();

    delay(1000); // prevents usb driver crash on startup, do not omit this
    while (!SERIAL) ;  // Wait for serial terminal to open port before starting program

    SERIAL.println("");
    SERIAL.println("******************************");
    SERIAL.println("        Program start         ");
    SERIAL.println("******************************");
    SERIAL.flush();

    // Set the led the rtos will blink when we have a fatal rtos error
    // RTOS also Needs to know if high/low is the state that turns on the led.
    // Error Blink Codes:
    //    3 blinks - Fatal Rtos Error, something bad happened. Think really hard about what you just changed.
    //    2 blinks - Malloc Failed, Happens when you couldn't create a rtos object.
    //               Probably ran out of heap.
    //    1 blink  - Stack overflow, Task needs more bytes defined for its stack!
    //               Use the taskMonitor thread to help gauge how much more you need
    //vSetErrorLed(ERROR_LED_PIN, ERROR_LED_LIGHTUP_STATE);

    // sets the serial port to print errors to when the rtos crashes
    // if this is not set, serial information is not printed by default
    //vSetErrorSerial(&SERIAL);

    SERIAL.println("Setup LED and SERIAL error");
    SERIAL.flush();

    // Create the threads that will be managed by the rtos
    // Sets the stack size and priority of each task
    // Also initializes a handler pointer to each task, which are important to communicate with and retrieve info from tasks
    xTaskCreate(threadA,     "Task A",       256, NULL, tskIDLE_PRIORITY + 3, &Handle_aTask);
    xTaskCreate(threadB,     "Task B",       256, NULL, tskIDLE_PRIORITY + 2, &Handle_bTask);
    xTaskCreate(threadC,     "Task C",       256, NULL, tskIDLE_PRIORITY + 4, &Handle_cTask);
    xTaskCreate(taskMonitor, "Task Monitor", 256, NULL, tskIDLE_PRIORITY + 1, &Handle_monitorTask);

    SERIAL.println("Created tasks");
    SERIAL.flush();

    // Start the RTOS, this function will never return and will schedule the tasks.
    vTaskStartScheduler();

    // error scheduler failed to start
    // should never get here
    while(1)
    {
        SERIAL.println("Scheduler Failed! \n");
        SERIAL.flush();
        delay(1000);
    }

}

//*****************************************************************
// This is now the rtos idle loop
// No rtos blocking functions allowed!
//*****************************************************************
void loop()
{
    // Optional commands, can comment/uncomment below
    SERIAL.print("."); //print out dots in terminal, we only do this when the RTOS is in the idle state
    SERIAL.flush();
    delay(100); //delay is interrupt friendly, unlike vNopDelayMS
}

//*****************************************************************