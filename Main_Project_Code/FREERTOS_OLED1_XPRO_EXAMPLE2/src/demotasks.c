#include <asf.h>
#include <conf_demo.h>
#include "demotasks.h"
#include "lcd.h"

//Some defines for our tasks

#define LCD_TASK_PRIORITY				(tskIDLE_PRIORITY + 20)
#define LCD_TASK_DELAY					(50 / portTICK_RATE_MS)

#define NEW_SENSOR_TASK_PRIORITY		(tskIDLE_PRIORITY + 1)
#define NEW_SENSOR_TASK_DELAY			(1000 / portTICK_RATE_MS)

#define ANALYZE_DATA_TASK_PRIORITY		(tskIDLE_PRIORITY + 1)
#define ANALYZE_DATA_TASK_DELAY			(33 / portTICK_RATE_MS)

#define SAMPLE_TEMP_TASK_PRIORITY		(tskIDLE_PRIORITY + 1)
#define SAMPLE_TEMP_TASK_DELAY			(33 / portTICK_RATE_MS)

#define OPEN_CLOSE_TASK_PRIORITY		(tskIDLE_PRIORITY + 1)
#define OPEN_CLOSE_TASK_DELAY			(33 / portTICK_RATE_MS)

#define SYSTASK_HANDLER_PRIORITY		(tskIDLE_PRIORITY + 1)
#define SYSTASK_HANDLER_DELAY			(33 / portTICK_RATE_MS)

//! Semaphore to signal busy display
static xSemaphoreHandle lcd_mutex;

//! Semaphore to signal busy terminal buffer
static xSemaphoreHandle wireless_mutex;

//! The event group used.
static EventGroupHandle_t event_group = NULL;

//display data
char mode_type[4] = "cool";
char selected_room[2] = "1";
char target_temp[2] = "70";
char selected_room_temp[2] = "NA";

//name the tasks
static void lcd_task(void *params);
static void new_sensor_task(void *params);
static void analyze_data_task(void *params);
static void sample_temp_task(void *params);
static void open_close_task(void *params);
static void systask_handler(void *params);

//initialize the tasks
void tasks_init(void)
{
	//mutexes
	lcd_mutex  = xSemaphoreCreateMutex();
	wireless_mutex = xSemaphoreCreateMutex();
	
	configure_console();
	
	event_group = xEventGroupCreate();

	xTaskCreate(lcd_task,
			(const char *)"LCD",
			configMINIMAL_STACK_SIZE,
			NULL,
			LCD_TASK_PRIORITY,
			NULL);

	xTaskCreate(new_sensor_task,
			(const char *)"New Sensor",
			configMINIMAL_STACK_SIZE,
			NULL,
			NEW_SENSOR_TASK_PRIORITY,
			NULL);

	xTaskCreate(analyze_data_task,
			(const char *) "Analyze Data",
			configMINIMAL_STACK_SIZE,
			NULL,
			ANALYZE_DATA_TASK_PRIORITY,
			NULL);

	xTaskCreate(sample_temp_task,
			(const char *)"Sample Temp",
			configMINIMAL_STACK_SIZE,
			NULL,
			SAMPLE_TEMP_TASK_PRIORITY,
			NULL);

	xTaskCreate(open_close_task,
			(const char *) "Open/Close",
			configMINIMAL_STACK_SIZE,
			NULL,
			OPEN_CLOSE_TASK_PRIORITY,
			NULL);
			
	xTaskCreate(systask_handler,
			(const char *) "Systask Handler",
			configMINIMAL_STACK_SIZE,
			NULL,
			SYSTASK_HANDLER_PRIORITY,
			NULL);
}

static void lcd_task(void *params)
{
<<<<<<< HEAD
	putchar(254);
	putchar(128);
	
	printf("Mode:");
	printf(mode_type);
	printf("  ");
	printf("Rm:");
	printf(selected_room);
	printf("Target:");
	printf(target_temp);
	printf("  ");
	printf(selected_room_temp);
	
=======
	//putchar(128);
	//putchar(256);
	printf("lcd_task");
>>>>>>> JJ's_Branch
	vTaskDelay(LCD_TASK_DELAY);
}

static void new_sensor_task(void *params)
{
	vTaskDelay(NEW_SENSOR_TASK_DELAY);
}

static void analyze_data_task(void *params)
{
	vTaskDelay(ANALYZE_DATA_TASK_DELAY);
}

static void sample_temp_task(void *params)
{
	vTaskDelay(SAMPLE_TEMP_TASK_DELAY);
}

static void open_close_task(void *params)
{
	vTaskDelay(OPEN_CLOSE_TASK_DELAY);
}
static void systask_handler(void *params)
{
	vTaskDelay(SYSTASK_HANDLER_DELAY);
}