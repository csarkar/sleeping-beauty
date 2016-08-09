#include "tailored-lwb.h"
//#include "glossy-test.h"
	

PROCESS(sleeping_beauty_test, "Sleeping Beauty test");
AUTOSTART_PROCESSES(&sleeping_beauty_test);
PROCESS_THREAD(sleeping_beauty_test, ev, data)
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
