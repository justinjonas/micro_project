#include <asf.h>
#include "demotasks.h"
//#include "trcUser.h"


int main (void)
{
	system_init();
	//vTraceInitTraceData();
	//vTraceStart();
	//gfx_mono_init();

	// Initialize the demo..
	tasks_init();

	// ..and let FreeRTOS run tasks!
	vTaskStartScheduler();

	do {
		// Intentionally left empty
	} while (true);
}
