#include <asf.h>
#include "demotasks.h"


int main (void)
{
	system_init();
	//gfx_mono_init();

	// Initialize the demo..
	tasks_init();

	// ..and let FreeRTOS run tasks!
	vTaskStartScheduler();

	do {
		// Intentionally left empty
	} while (true);
}
