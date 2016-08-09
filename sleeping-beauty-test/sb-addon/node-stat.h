#ifndef NODE_STAT_H_
#define NODE_STAT_H_

#include "contiki.h"

/*---------------------------------------------------------------------------*/

#define MAX_STAT_ENTRY				10

#define INITIAL_ETX					16000

#define DISCOVERY_BOOTSTRAPPING()   (discovery_cnt < GLOSSY_BOOTSTRAP_PERIODS)

/*---------------------------------------------------------------------------*/
typedef struct {
	uint16_t node;
	uint16_t etx;
	uint16_t slot;
} parent_stat_struct;

typedef struct {
	uint16_t etx;
	uint16_t parent[2];
} own_stat_struct;

/*---------------------------------------------------------------------------*/
extern double 			   alpha[2];
extern own_stat_struct	   own_data;
extern int8_t			   discovery_cnt;
extern parent_stat_struct  parent_stat[MAX_STAT_ENTRY];
/*---------------------------------------------------------------------------*/

void estimate_clock_offset(int16_t period, int16_t offset, int period_skew, int off);

void add_neighbor_stat(uint16_t probe_slot, uint16_t node, uint16_t etx, int8_t cnt, int8_t s_tx);

void update_neighbor_stat(uint8_t stat_index, uint16_t node, uint16_t etx, int8_t cnt, int8_t s_tx);

void choose_alternate_parent();

void initialize_node(int16_t node_id);

//uint16_t decide_flooding_participation(uint8_t *slots);




#endif /* NODE_STAT_H_ */
