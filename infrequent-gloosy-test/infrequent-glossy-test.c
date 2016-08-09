#include "tailored-lwb.h"
	

PROCESS(infrequent_glossy_test, "Infrequent glossy test");
AUTOSTART_PROCESSES(&infrequent_glossy_test);
PROCESS_THREAD(infrequent_glossy_test, ev, data)
{
	static struct etimer et;
	
	PROCESS_BEGIN();
	
	/* Allow some time for the network to settle. */
	etimer_set(&et, 30 * CLOCK_SECOND);
	PROCESS_WAIT_UNTIL(etimer_expired(&et));
	printf("node_id %d\n", node_id);
	
	process_start(&tailored_lwb_process, NULL);
	
	PROCESS_END();
}


/** @} */
/** @} */
/** @} */
