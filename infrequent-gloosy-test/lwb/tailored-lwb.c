#include "slot-allocation.h"
#include "slot-participation.h"
#include "energy-stats.h"

#include "active-selection.h"
#include "node-stat.h"


/**
 * \defgroup custom-lwb-variables Application variables
 * @{
 */

static sync_data_struct sync_data;     /**< \brief Flooding data. */
static struct rtimer rt;                   /**< \brief Rtimer used to schedule Glossy. */
static struct pt pt;                       /**< \brief Protothread used to schedule Glossy. */
static rtimer_clock_t t_ref_l_old = 0;     /**< \brief Reference time computed from the Glossy
                                                phase before the last one. \sa get_t_ref_l */
static uint8_t skew_estimated = 0;         /**< \brief Not zero if the clock skew over a period of length
                                                \link GLOSSY_PERIOD \endlink has already been estimated. */
static uint8_t sync_missed = 0;            /**< \brief Current number of consecutive phases without
                                                synchronization (reference time not computed). */
static int period_skew = 0;                /**< \brief Current estimation of clock skew over a period
                                                of length \link GLOSSY_PERIOD \endlink. */

/** @} */

/**
 * \defgroup lwb-test-variables-stats Statistics variables
 * @{
 */

/*---------------------------------------------------------------------------*/
static rtimer_clock_t REF_TIME;
static rtimer_clock_t NEXT_SLOT;
/*---------------------------------------------------------------------------*/
static int16_t period = 0;
static int offset;
static int offset_err;
/*---------------------------------------------------------------------------*/
static uint16_t run_time, last_time;

static uint8_t  SYNC_SLOT = 1;

static uint8_t  SLEEP_SLOTS;
/*---------------------------------------------------------------------------*/
static int8_t FLOODING_ROLE;
static enum err_type errno;
/*---------------------------------------------------------------------------*/

char tailored_lwb_scheduler(struct rtimer *t, void *ptr);
/*---------------------------------------------------------------------------*/

/** @} */
/** @} */

/**
 * \defgroup lwb-test-processes Application processes and functions
 * @{
 */

static inline void estimate_period_skew(void) {
	// Estimate clock skew over a period only if the reference time has been updated.
	if (GLOSSY_IS_SYNCED()) {
		// Estimate clock skew based on previous reference time and the Glossy period.
		period_skew = get_t_ref_l() - (t_ref_l_old + (rtimer_clock_t)GLOSSY_PERIOD);
		// Update old reference time with the newer one.
		t_ref_l_old = get_t_ref_l();
		// If Glossy is still bootstrapping, count the number of consecutive updates of the reference time.
		if (GLOSSY_IS_BOOTSTRAPPING()) {
			// Increment number of consecutive updates of the reference time.
			skew_estimated++;
			// Check if Glossy has exited from bootstrapping.
			if (!GLOSSY_IS_BOOTSTRAPPING()) {
				// Glossy has exited from bootstrapping.
				leds_off(LEDS_RED);
				// Initialize Energest values.
				energest_init();
#if GLOSSY_DEBUG
				high_T_irq = 0;
				bad_crc = 0;
				bad_length = 0;
				bad_header = 0;
#endif /* GLOSSY_DEBUG */

			}
		}
	}
}

/**
 * 
 */
PROCESS(tailored_lwb_print_process, "Tailored LWB print process");

PROCESS_THREAD(tailored_lwb_print_process, ev, data)
{
	PROCESS_BEGIN();
	
	while(1) {
		PROCESS_YIELD_UNTIL(ev == PROCESS_EVENT_POLL);
		switch(errno) {
		case NO_SYNC_PACKET:
			period = 0;
			offset = 0;
			printf("Glossy NOT received\n");
			break;
		case DATA_SLOT_END:
			break;
		case NO_ERROR:
		case DATA_TX_SUCC:
		case DATA_TX_FAIL:
		case DATA_TX_IGNORE:
		default:
			if(!IS_SINK()) {
				if(run_time >= COOLOFF_PERIOD) {
					if(run_time <= TRAINING_PERIOD) {
						period++;
						offset = offset + period_skew;
					} else {
						period = sync_data.sleep_slots;
						offset = period_skew;
					}
					/* estimate clock offset */
					estimate_clock_offset(period, offset, period_skew, offset_err);
					//printf("period %d, offset %d, before and after correction %d, %d, alpha=%d, beta=%d\n", 
					//		period, offset, period_skew, offset_err, (int)(alpha[0]*100), (int)(alpha[1]*100));
				}
				energy_update((run_time-last_time)*1000);
				last_time = run_time;
			}
			break;
		}
		errno   = NO_ERROR;
	}

	PROCESS_END();
}

/**
 * 
 */
static inline int8_t reset_parameters() {
		
	if(get_rx_cnt() == 0) {
		SLEEP_SLOTS		 = 1;
		skew_estimated--;
		return NO_SYNC_PACKET;
	}
	else {
		run_time = sync_data.run_time;
			
		SLEEP_SLOTS	= sync_data.sleep_slots;
		if(sync_data.sleep_slots>1) {
			t_ref_l_old = (rtimer_clock_t)(t_ref_l_old + ((SLEEP_SLOTS-1)%2)*RTIMER_SECOND);
		} else {
			SLEEP_SLOTS	= 1;
		}
		return NO_ERROR;
	}
}

/**
 * 
 */
void prepare_next_superframe() {
		
	sync_data.run_time = run_time;	

	if(run_time < TRAINING_PERIOD) {
		sync_data.rr_slots    = 0;
		sync_data.data_slots  = 0;
		sync_data.sleep_slots = 1;
	} 
	else {
		sync_data.sleep_slots = 60*IPI;
	}
	//printf("Sync received: run time %u\n", sync_data.run_time);
}

/**
 * 
 */
void next_radio_activity_schedule(struct rtimer *t, void *ptr) {
		
	rtimer_clock_t GRACE_PERIOD = 0;
	
	if(NEXT_SLOT < RTIMER_SECOND) { 
		NEXT_SLOT = RTIMER_SECOND;
	}
		
	/* if a second has been passed, add it to the RX_TIME */
	if(NEXT_SLOT >= RTIMER_SECOND) {
		NEXT_SLOT   = (rtimer_clock_t)(NEXT_SLOT - RTIMER_SECOND);
		REF_TIME    = (rtimer_clock_t)(REF_TIME + RTIMER_SECOND);
		run_time++;
		if(SLEEP_SLOTS > 0) {
			SLEEP_SLOTS--;
		}	
		
		if(SLEEP_SLOTS==0) {
			if(!IS_SINK()) {
				//GRACE_PERIOD = GLOSSY_GUARD_TIME * (1 + sync_missed) * (-1);
				GRACE_PERIOD = GLOSSY_GUARD_TIME * (-1);
			}
		}
	}
	
	/* schedule next radio activity */
	rtimer_set(t, (rtimer_clock_t)(REF_TIME + NEXT_SLOT + GRACE_PERIOD), 1, 
				(rtimer_callback_t)tailored_lwb_scheduler, ptr);
				
	if(SLEEP_SLOTS==0) {
		
		/* print stats after the LWB round */
		process_poll(&tailored_lwb_print_process);
		
		/* indicate there will be a sync slot */
		SYNC_SLOT = 1;
		
		/* sink node decides structure of the next superframe */
		if (IS_SINK()) {
			prepare_next_superframe();
		}
	}
	
}

/**
 * 
 */
void node_init() {
	
	run_time = 0;
	last_time = run_time;
		
	if (IS_SINK()) {
		initialize_sink_node(node_id);
		sync_data.run_time	  = 0;
		sync_data.rr_slots    = 0;
		sync_data.sleep_slots = 1;
	} else {
		initialize_node(node_id);
	}
}

/**
 * 
 */
rtimer_clock_t prepare_sync_packet(struct rtimer *t) {
	
	leds_on(LEDS_GREEN);
		
	rtimer_clock_t t_stop;
			
	if (IS_SINK()) {
		t_stop = RTIMER_TIME(t) + GLOSSY_DURATION;
		FLOODING_ROLE = GLOSSY_INITIATOR;
		REF_TIME = RTIMER_TIME(t);
	} else {
		if (GLOSSY_IS_BOOTSTRAPPING()) {
			t_stop = RTIMER_TIME(t) + GLOSSY_INIT_DURATION;
		} else {
			t_stop = RTIMER_TIME(t) + GLOSSY_DURATION;
		}
		FLOODING_ROLE = GLOSSY_RECEIVER;	
	}
	
	return t_stop;
}

/**
 * 
 */
void process_sync_packet(struct rtimer *t, void *ptr) {
	
	leds_off(LEDS_GREEN);
	
	if(IS_SINK()) {
		if (!GLOSSY_IS_BOOTSTRAPPING()) {
			// Glossy has already successfully bootstrapped.
			if (!GLOSSY_IS_SYNCED()) {
				// The reference time was not updated: increment reference time by GLOSSY_PERIOD.
				set_t_ref_l(GLOSSY_REFERENCE_TIME + GLOSSY_PERIOD);
				set_t_ref_l_updated(1);
			}
		}
		
		// Estimate the clock skew over the last period.
		estimate_period_skew();
		
		if (GLOSSY_IS_BOOTSTRAPPING()) {
			rtimer_set(t, REF_TIME + GLOSSY_PERIOD, 1, (rtimer_callback_t)tailored_lwb_scheduler, ptr);
			SYNC_SLOT = 1;
		} else {
			NEXT_SLOT = GLOSSY_SYNC_GUARD;
			reset_parameters();				
			rtimer_set(t, REF_TIME + NEXT_SLOT, 1, (rtimer_callback_t)tailored_lwb_scheduler, ptr);
			SYNC_SLOT = 0;
		}
	}
	else {
		if (GLOSSY_IS_BOOTSTRAPPING()) {
			// Glossy is still bootstrapping.
			if (!GLOSSY_IS_SYNCED()) {
				// The reference time was not updated: reset skew_estimated to zero.
				skew_estimated = 0;
			}
		} else {
			// Glossy has already successfully bootstrapped.
			if (!GLOSSY_IS_SYNCED()) {
				// The reference time was not updated:
				// increment reference time by GLOSSY_PERIOD + period_skew.
				set_t_ref_l(GLOSSY_REFERENCE_TIME + GLOSSY_PERIOD + period_skew);
				set_t_ref_l_updated(1);
				// Increment sync_missed.
				sync_missed++;
			} else {
				// The reference time was not updated: reset sync_missed to zero.
				sync_missed = 0;
			}
		}
		// Estimate the clock skew over the last period.
		estimate_period_skew();
		if (GLOSSY_IS_BOOTSTRAPPING()) {
			// Glossy is still bootstrapping.
			if (skew_estimated == 0) {
				rtimer_set(t, RTIMER_TIME(t) + GLOSSY_INIT_PERIOD, 1,
						(rtimer_callback_t)tailored_lwb_scheduler, ptr);
				SYNC_SLOT = 1;
			} else {
				REF_TIME = GLOSSY_REFERENCE_TIME + GLOSSY_PERIOD;
				rtimer_set(t, REF_TIME - GLOSSY_INIT_GUARD_TIME, 1,
							(rtimer_callback_t)tailored_lwb_scheduler, ptr);
				SYNC_SLOT = 0;
			}
		} else {
			offset_err = GLOSSY_REFERENCE_TIME - (rtimer_clock_t)REF_TIME;
			REF_TIME   = GLOSSY_REFERENCE_TIME;
			NEXT_SLOT  = GLOSSY_SYNC_GUARD;
			rtimer_set(t, REF_TIME + NEXT_SLOT, 1, (rtimer_callback_t)tailored_lwb_scheduler, ptr);
			
			errno = reset_parameters();
			SYNC_SLOT = 0;
			
			if(errno == NO_SYNC_PACKET) {
				process_poll(&tailored_lwb_print_process);
			}
		}
	}
}


/** @} */

/**
 * \defgroup lwb-test-scheduler Periodic scheduling
 * @{
 */

char tailored_lwb_scheduler(struct rtimer *t, void *ptr) {
	
	PT_BEGIN(&pt);

	node_init();

	uint8_t src, dst;
	
	while (1) {
		if(SYNC_SLOT) {
			rtimer_clock_t t_stop = prepare_sync_packet(t);
			
			/* every node participate (using flooding) in every sync slot */
			tailored_glossy_start((uint8_t *)&sync_data, SYNC_LEN, FLOODING_ROLE, FLOODING, GLOSSY_SYNC, N_TX,
						APPLICATION_HEADER, t_stop, (rtimer_callback_t)tailored_lwb_scheduler, t, ptr);
			PT_YIELD(&pt);
			tailored_glossy_stop(&src, &dst);
			
			process_sync_packet(t, ptr);				
		}
		else {
			if(SLEEP_SLOTS%IPI == 1 && sync_data.sleep_slots > 5) {
				// rectify clock offset
				REF_TIME = REF_TIME + (rtimer_clock_t)(alpha[0]*IPI + alpha[1]);
			}
			
			/* in the sleep slot nothing to be done */
			/* schedule the next radio activity */
			next_radio_activity_schedule(t, ptr);
		} 
		
		
		/* Yield the protothread. */
		PT_YIELD(&pt);
	}
	
	PT_END(&pt);
}
	

PROCESS(tailored_lwb_process, "LWB Process");
PROCESS_THREAD(tailored_lwb_process, ev, data)
{
	PROCESS_BEGIN();
	
	initiate_energy_accounting();
	
	leds_on(LEDS_RED);
	// Start print stats processes.
	process_start(&tailored_lwb_print_process, NULL);
	// Start Glossy busy-waiting process.
	process_start(&tailored_glossy_process, NULL);
	// Start Glossy experiment in one second.
	rtimer_set(&rt, RTIMER_NOW() + RTIMER_SECOND, 1, (rtimer_callback_t)tailored_lwb_scheduler, NULL);
	
	PROCESS_END();
}

/** @} */
/** @} */
/** @} */
