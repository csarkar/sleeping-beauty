#ifndef SLOT_DEF_H_
#define SLOT_DEF_H_


/****** Application defined parameters that controls LWB process *****/

/**
 * Inter packet interval, default is 10 seconds
 */
#ifndef IPI
#define IPI							10
#endif

/**
 * Initial network cooloff period, default is 10 seconds
 */
#define COOLOFF_PERIOD				3*IPI

/**
 * Initial network stabilization period, default is 20 seconds
 */
#define STABILIZATION_PERIOD		COOLOFF_PERIOD+2*IPI

/** 
 * minimum training period to estimate clock offset parameters 
 */
#define TRAINING_PERIOD				STABILIZATION_PERIOD + 10*IPI

/**
 * Number of round the set of active nodes will remain unchanged
 */
#define ACTIVE_SELECTION_ROUND		10

/**
 * LWB reset period, default is 24 hours
 */
#define LWB_RESET_PERIOD			STABILIZATION_PERIOD + 24*360*IPI

/**
 * Maximum payload length of a LWB data packet
 */
#define MAX_PAYLOAD_LEN				40

/**
 * NodeId of the initiator.
 * Default value: 1
 */
#define SINK_NODE_ID       			1

/**
 * MAX number of nodes in the network.
 * Default value: 150
 */
#define MAX_NODE_NUMBER		       	150

/**
 * This indicates whether forwarder selection will be used or a LWB-like 
 * all node participation in all flooding.
 */
#ifndef	FORWARDER_SELECTION
#define FORWARDER_SELECTION			0
#endif


/**************** SYSTEM parameters, change with care *****************/

/**
 * required by FS-LWB, a different value can change FS-LWB's behavior 
 * and performance
 */
#define RSSI_THESHOLD				-75

/**
 * associated with FS-LWB
 */
#define RSSI_BUFFER					-5

/**
 * \brief MAX number of req/reply slots per second.
 *        Default value: 48
 */
#define MAX_RR_SLOTS_P_SECOND		48

#if FORWARDER_SELECTION
#define MIN_RR_SLOTS				3
#else 
#define MIN_RR_SLOTS				2
#endif

/**
 * \brief MAX number of global data slots per second.
 *        Default value: 38
 */
//#define MAX_SLOTS_P_SECOND			52			

/**
 * \brief number of times data packets will be sent 
 */
#define D_TX 						2

/**
 * \brief number of times strobe will be sent 
 */
#define S_TX 						5

/**
 * \brief Maximum number of transmissions N.
 *        Default value: 5.
 */
#define N_TX                    	3


/** 
 * how many (maximum, minimum is 1) parent nodes will be listed for each node 
 */
#define ALTERNATE_PARENT			1


/**
 * \brief Period with which a Glossy phase is scheduled.
 *        Default value: 1000 ms.
 */
#define GLOSSY_PERIOD           (RTIMER_SECOND)         // 1000 ms

/**
 * \brief Duration of each Glossy phase.
 *        Default value: 15 ms.
 */
#define GLOSSY_DURATION      	(RTIMER_SECOND / 66)    // 15 ms

#define GLOSSY_SYNC_GUARD		(RTIMER_SECOND / 40)	// 25 ms

#define RR_SLOT_LEN		 		(RTIMER_SECOND / 55)    // 18 ms

#define RR_SLOT_DURATION 		(RTIMER_SECOND / 66)    // 15 ms

#define DATA_SLOT_LEN	  		(RTIMER_SECOND / 55)    // 18 ms

#define DATA_SLOT_DURATION 		(RTIMER_SECOND / 66)    // 15 ms

#define STROBE_SLOT_LEN			(RTIMER_SECOND / 66)    // 15 ms

#define STROBE_SLOT_DURATION	(RTIMER_SECOND / 100)   // 10 ms

/**
 * \brief Guard-time at receivers for the sync packet.
 *        Default value: 2000 us.
 */
#if COOJA
#define GLOSSY_GUARD_TIME       (RTIMER_SECOND / 1000)
#else
#define GLOSSY_GUARD_TIME       (RTIMER_SECOND / 1000)   // 526 us
#endif /* COOJA */

/**
 * \brief Guard-time at receivers for the rest of packets except the sync packet.
 *        Default value: 1000 us.
 */
#define TX_GUARD_TIME       	GLOSSY_GUARD_TIME //(RTIMER_SECOND / 1000)	// 1 ms

/**
 * \brief Number of consecutive Glossy phases with successful computation of reference time required to exit from bootstrapping.
 *        Default value: 3.
 */
#define GLOSSY_BOOTSTRAP_PERIODS 3

/**
 * \brief Period during bootstrapping at receivers.
 *        It should not be an exact fraction of \link GLOSSY_PERIOD \endlink.
 *        Default value: 69.474 ms.
 */
#define GLOSSY_INIT_PERIOD      (GLOSSY_INIT_DURATION + RTIMER_SECOND / 100)                   //  69.474 ms

/**
 * \brief Duration during bootstrapping at receivers.
 *        Default value: 59.474 ms.
 */
#define GLOSSY_INIT_DURATION    (GLOSSY_DURATION - GLOSSY_GUARD_TIME + GLOSSY_INIT_GUARD_TIME) //  59.474 ms

/**
 * \brief Guard-time during bootstrapping at receivers.
 *        Default value: 50 ms.
 */
#define GLOSSY_INIT_GUARD_TIME  (RTIMER_SECOND / 20)                                           //  50 ms

/**
 * \brief Application-specific header.
 *        Default value: 0x0
 */
#define APPLICATION_HEADER      	0


/********** Do not chnage these until absolutely necessary ***********/

/** @} */

/**
 * \defgroup glossy-test-defines Application internal defines
 * @{
 */


/**
 * \brief Check if the nodeId matches the one of the initiator.
 */
#define IS_SINK()              (node_id == SINK_NODE_ID)

/**
 * \brief Check if Glossy is still bootstrapping.
 * \sa \link GLOSSY_BOOTSTRAP_PERIODS \endlink.
 */
#define GLOSSY_IS_BOOTSTRAPPING()   (skew_estimated < GLOSSY_BOOTSTRAP_PERIODS)

/**
 * \brief Check if Glossy is synchronized.
 *
 * The application assumes that a node is synchronized if it updated the reference time
 * during the last Glossy phase.
 * \sa \link is_t_ref_l_updated \endlink
 */
#define GLOSSY_IS_SYNCED()          (is_t_ref_l_updated())

/**
 * \brief Get Glossy reference time.
 * \sa \link get_t_ref_l \endlink
 */
#define GLOSSY_REFERENCE_TIME       (get_t_ref_l())

/** @} */

/** @} */

#endif /* SLOT_DEF_H_ */
