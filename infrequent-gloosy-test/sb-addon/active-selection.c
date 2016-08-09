#include "active-selection.h"

#include <stdio.h>

/*---------------------------------------------------------------------------*/
uint8_t  slot_allocation[MAX_BITMAP_LEN];
sensor_info_struct node_info[MAX_NODE_NUMBER];
/*---------------------------------------------------------------------------*/
static uint16_t highest_node_number;
static uint8_t  active_count;
/*---------------------------------------------------------------------------*/
#define TOTAL_CLUSTERS	15
/* clusters are defined as following - number of nodes in the cluster, then the node-ids */
static uint8_t cluster_members[] = {10,1,2,3,4,5,6,7,8,9,10,
	10,11,12,13,14,15,16,17,18,19,20,
	10,21,22,23,24,25,26,27,28,29,30,
	10,31,32,33,34,35,36,37,38,39,40,
	10,41,42,43,44,45,46,47,48,49,50,
	10,51,52,53,54,55,56,57,58,59,60,
	10,61,62,63,64,65,66,67,68,69,70,
	10,71,72,73,74,75,76,77,78,79,80,
	10,81,82,83,84,85,86,87,88,89,90,
	10,91,92,93,94,95,96,97,98,99,100,
	10,101,102,103,104,105,106,107,108,109,110,
	10,111,112,113,114,115,116,117,118,119,120,
	10,121,122,123,124,125,126,127,128,129,130,
	10,131,132,133,134,135,136,137,138,139,140,
	10,141,142,143,144,145,146,147,148,149,150};

static uint8_t cluster_cover[TOTAL_CLUSTERS+1];
/*---------------------------------------------------------------------------*/
void initialize_sink_node(uint16_t node_id) {
	node_info[node_id-1].parent[0] = node_id;
	highest_node_number = node_id;
	
	
	uint8_t i;
	uint16_t j, k=0;
    for(i=0; i<TOTAL_CLUSTERS; i++) {
		for(j=k+1; j<=k+cluster_members[k]; j++) {
			node_info[cluster_members[j]-1].cluster_number = i+1;
		}
		k = k+cluster_members[k]+1;
	}
	cluster_cover[0] = 0;
	printf("cluster number for node %u is %u\n", 32, node_info[32-1].cluster_number);
}
/*---------------------------------------------------------------------------*/
void cluster_member_notification(uint16_t node, uint16_t slot) {
	if(node_info[node-1].slot == 0) {
		node_info[node-1].active_time = 0;
		node_info[node-1].slot 		  = slot;
		
		if(highest_node_number < node) {
			highest_node_number = node;
		}
		
		uint8_t c_number = node_info[node-1].cluster_number;
		if(cluster_cover[c_number] == 0) {
			cluster_cover[c_number] = 1;
			cluster_cover[0]++;
		}
	}
}
/*---------------------------------------------------------------------------*/
uint16_t mark_node_active(uint16_t node, uint16_t clusters_covered)
{
    if(node_info[node].parent[0] == 0) {
		printf("Parent node not set for %d\n", node+1);
	} else {
		uint16_t parent = node_info[node].parent[0] - 1;
		if(node_info[parent].state != ACTIVE) {
		#if ALTERNATE_PARENT
			uint16_t pp = node_info[node].parent[1] - 1;
			if(node_info[pp].state != ACTIVE) {
				clusters_covered = mark_node_active(parent, clusters_covered);
			}
		#else
			clusters_covered = mark_node_active(parent, clusters_covered);
		#endif
		}
	}
	
	if(cluster_cover[node_info[node].cluster_number] == 1) {
		cluster_cover[node_info[node].cluster_number] = 2;
		clusters_covered++;
	}
    /* mark the selected node as active */
    node_info[node].state = ACTIVE;     
    node_info[node].active_time += 1;
    slot_allocation[node/8] = slot_allocation[node/8] | (0x01 << (node%8));
	active_count++;
	
    return clusters_covered;
}
/*---------------------------------------------------------------------------*/
void choose_active_nodes(uint16_t sink_node_id, uint8_t vector[])
{
    uint16_t node;
    uint16_t i;
    uint16_t clusters_covered;
    
    /* reset all cluster covers */
    for(i=1; i<=TOTAL_CLUSTERS; i++) {
		if(cluster_cover[i] != 0) {
			cluster_cover[i] = 1;
		}
	} 
	
	/* reset all bitmaps */
    for(i=0; i<MAX_BITMAP_LEN; i++) {
		slot_allocation[i] = 0x00;
	} 
	
    /* reset every node's state */
    for(i=0; i<highest_node_number; i++) {
        if(node_info[i].slot > 0) {
			node_info[i].state = NOT_ASSIGNED;
		} else {
			node_info[i].state = DEAD;
		}
    }    
    
    /* assign sink node as active */
    node_info[sink_node_id-1].state		= ACTIVE;
    node_info[sink_node_id-1].active_time += 1;
    clusters_covered = 0;
    active_count = 0;
    
    /* iterate until all node is assigned a state */ 
    while(clusters_covered < cluster_cover[0]) {
		node = sink_node_id-1;    
		for(i=0; i<highest_node_number; i++) {
			/* if a role is not assigned and the cluster is not covered and 
			 * its active time is less than the minimum active time  */
			if((node_info[i].state == NOT_ASSIGNED) && (cluster_cover[node_info[i].cluster_number] == 1) && 
								(node_info[i].active_time < node_info[node].active_time)) {
				node = i;
			}
		}
    	/* select a node as potential active from the remaining nodes that is not yet assigned */
        if(node == sink_node_id-1) {
            printf("Something is wrong: probably many dead nodes, %d\n", clusters_covered);
            break;
        }
		clusters_covered = mark_node_active(node, clusters_covered);
    }
    
    /* reset every node's state */
    for(node=0; node<highest_node_number; node++) {
        if(node_info[node].slot > 0) {
			i = node_info[node].slot-1;
			if(node_info[node].state == ACTIVE) {
				vector[i/8] |= 1 << (i%8);
			} else {
				vector[i/8] &= ~(1 << (i%8));
			}	
		}
    }
    
    printf("active count %d, vector %u, total %u\n", active_count, vector[0], cluster_cover[0]);//slot_allocation[0], slot_allocation[1]);
}
/*---------------------------------------------------------------------------*/
