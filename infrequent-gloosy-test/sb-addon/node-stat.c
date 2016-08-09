#include "node-stat.h"

#include <stdio.h>


static int8_t	stat_entry_count;
/*---------------------------------------------------------------------------*/
/* parameters related to clock offset estimation */
static int32_t	oldOffset;
static int8_t   hasInitialLS = 0;
static double	iG[2][2];
static double	ab[2];
/*---------------------------------------------------------------------------*/
/* global variables */
double 				alpha[2];
own_stat_struct 	own_data;
int8_t				discovery_cnt = 0;
parent_stat_struct  parent_stat[MAX_STAT_ENTRY];
/*---------------------------------------------------------------------------*/
void matMultiply(double a[2][2], double b[2][2], double g[2][2]) {
	int i,j,k;
	
	for(i=0; i<2; i++) {
		for(j=0; j<2; j++) {
			g[i][j] = 0;
			for(k=0; k<2; k++) {
				g[i][j] += a[i][k]*b[k][j];
			}
		}
	}
}
/*---------------------------------------------------------------------------*/
int8_t initialLS(int32_t a1, int32_t a2, int32_t b1, int32_t b2) {

	double g[2][2];
	
	g[0][0] = (double) (a1*a1 + a2*a2);
	g[0][1] = (double) (a1 + a2);
	g[1][0] = (double) (a1 + a2);
	g[1][1] = 2.0;
	
	// determinant of g 
	double det = g[0][0]*g[1][1] - g[0][1]*g[1][0];
	if(det == 0 ) {
		return 0;
	}

	// get inverse of g
	iG[0][0] = (g[1][1])/det;
	iG[0][1] = (-g[0][1])/det;
	iG[1][0] = (-g[1][0])/det;
	iG[1][1] = (g[0][0])/det;
	
	ab[0] = (double) (a1*b1 + a2*b2);
	ab[1] = (double) (b1 + b2);
	
	alpha[0] = (iG[0][0]*ab[0] + iG[0][1]*ab[1]);
	alpha[1] = (iG[1][0]*ab[0] + iG[1][1]*ab[1]);
	
	return 1;
}
/*---------------------------------------------------------------------------*/
void iterativeLS(int32_t a, int32_t b) {

	double p;
	double c[2][2];
	double c1[2][2];

	p = 1 + (a*a*iG[0][0] + a*iG[0][1] + a*iG[1][0] +iG[1][1]);

	c[0][0] = a*a;
	c[0][1] = a;
	c[1][0] = a;
	c[1][1] = 1;
	
	matMultiply(iG, c, c1);
	matMultiply(c1, iG, c);

	iG[0][0] = iG[0][0] - c[0][0]/p;
	iG[0][1] = iG[0][1] - c[0][1]/p;
	iG[1][0] = iG[1][0] - c[1][0]/p;
	iG[1][1] = iG[1][1] - c[1][1]/p;
	
	ab[0] += a*b;
	ab[1] += b;

	alpha[0] = (iG[0][0]*ab[0] + iG[0][1]*ab[1]);
	alpha[1] = (iG[1][0]*ab[0] + iG[1][1]*ab[1]);
}
/*---------------------------------------------------------------------------*/
void estimate_clock_offset(int16_t period, int16_t offset, int period_skew, int offset_err) {
	if(hasInitialLS >= 2) {
		iterativeLS(period, offset);
	} else if(hasInitialLS == 1) {
		hasInitialLS = initialLS(period-1, period, oldOffset, offset);
		oldOffset = offset;
		hasInitialLS++;
	} else {
		oldOffset = offset;
		hasInitialLS = 1;
	}
	//printf("period %d, offset %d, before and after correction %d, %d, alpha=%d, beta=%d\n", 
	//						period, offset, period_skew, offset_err, (int)(alpha[0]*100), (int)(alpha[1]*100));
}
/*---------------------------------------------------------------------------*/
void add_neighbor_stat(uint16_t probe_slot, uint16_t node, uint16_t etx, int8_t cnt, int8_t S_TX) {
	int8_t i=0;
	
	for(i=0; i<stat_entry_count; i++) {
		if(parent_stat[i].node == node) {
			update_neighbor_stat(i, node, etx, cnt, S_TX);
			return;
		}
	}
	
	if(stat_entry_count < MAX_STAT_ENTRY) {
		uint16_t new_etx = etx + (100*S_TX)/cnt;
		while(i>1 && parent_stat[i-1].slot > probe_slot) {
			parent_stat[i].slot = parent_stat[i-1].slot;
			parent_stat[i].node = parent_stat[i-1].node;
			parent_stat[i].etx  = parent_stat[i-1].etx;
			i--;
		}
		
		parent_stat[i].slot = probe_slot;
		parent_stat[i].node = node;
		if(etx < INITIAL_ETX) {
			parent_stat[i].etx 	= new_etx;
		} else {
			parent_stat[i].etx 	= INITIAL_ETX;
		}
			
		if(new_etx < own_data.etx) {
			//printf("parent %d, new etx %d, own_etx %d\n", node, new_etx, own_data.etx);
			own_data.etx 	   = new_etx;
			own_data.parent[0] = node;
		}
		stat_entry_count++;
	}
}
/*---------------------------------------------------------------------------*/
void update_neighbor_stat(uint8_t stat_index, uint16_t node, uint16_t etx, int8_t cnt, int8_t S_TX) {
	
	uint16_t new_etx;
	if(cnt==0) {
		new_etx = etx + 100*(S_TX+1);
	} else {
		new_etx = etx + (100*S_TX)/cnt;
	}
	
	if(parent_stat[stat_index].etx < INITIAL_ETX) {
		new_etx = (new_etx + parent_stat[stat_index].etx)/2;
	} else if(etx >= INITIAL_ETX) {
		new_etx	= INITIAL_ETX;
	}
	parent_stat[stat_index].etx = new_etx;
	
	if(new_etx < own_data.etx) {
		//printf("update parent %d, new etx %d, own_etx %d\n", node, new_etx, own_data.etx);
		own_data.etx 	   = new_etx;
		own_data.parent[0] = node;
	}
}
/*---------------------------------------------------------------------------*/
void choose_alternate_parent() {
#if ALTERNATE_PARENT
	uint16_t min = INITIAL_ETX;
	int8_t i;
	
	own_data.parent[1] = own_data.parent[0];
	for(i=0; i<stat_entry_count; i++) {
		if((parent_stat[i].etx < min) && (parent_stat[i].node != own_data.parent[0]) && 
						parent_stat[i].etx-own_data.etx < 100) {
			min = parent_stat[i].etx;
			own_data.parent[1] = parent_stat[i].node;
		}
	}
#endif
}
/*---------------------------------------------------------------------------*/
/*uint8_t count_set_bits(uint8_t byte, uint8_t bit_pos) {
	uint8_t i;
	uint8_t count = 0;
	
	for(i=0; i<=bit_pos; i++) {
		if((byte & (0x01 <<i)) > 0) {
			count++;
		}
	}
	return count;
}
uint16_t decide_flooding_participation(uint8_t *slots) {
	uint8_t  i = 0;
	uint16_t my_slot = 0;					

	while(i < (node_id-1)/8) {
		my_slot = my_slot + count_set_bits(slots[i], 7);
		i++;
	}
	my_slot = my_slot + count_set_bits(slots[i], (node_id-1)%8);

	return my_slot;
}*/
/*---------------------------------------------------------------------------*/
void initialize_node(int16_t node_id)
{
	own_data.etx  = INITIAL_ETX;
	
	stat_entry_count = 0;
}
/*---------------------------------------------------------------------------*/
