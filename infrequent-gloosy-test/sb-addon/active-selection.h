#ifndef ACTIVE_SELECTION_H_
#define ACTIVE_SELECTION_H_

#include "contiki.h"


#define MAX_NODE_NUMBER				150

#define MAX_BITMAP_LEN				40//(MAX_NODE_NUMBER / 8 + 1)

#define ACTIVE_SELECTION			32768

enum {
	NOT_ASSIGNED=0, ACTIVE=1, DORMANT=2, DEAD=3
};

typedef struct {
	uint8_t  state;
	uint16_t parent[2];
	uint32_t active_time;
	uint16_t slot;
	uint8_t  cluster_number;
} sensor_info_struct;


/*---------------------------------------------------------------------------*/
extern uint8_t  slot_allocation[MAX_BITMAP_LEN];
extern sensor_info_struct node_info[MAX_NODE_NUMBER];
/*---------------------------------------------------------------------------*/

void initialize_sink_node(uint16_t node_id);

void cluster_member_notification(uint16_t node, uint16_t slot);

void choose_active_nodes(uint16_t sink_node_id, uint8_t vector[]);

#endif /* ACTIVE_SELECTION_H_ */
