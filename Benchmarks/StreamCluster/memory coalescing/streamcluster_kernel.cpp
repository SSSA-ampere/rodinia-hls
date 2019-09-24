
#include "constant.h"
#include "mars_wide_bus_1d.h"
#define BUF_SIZE ((OFFSET) << 7)
#define BUF_NUM (BATCH_SIZE / BUF_SIZE)
#define HALF_BUF ((BUF_SIZE) / 2)
#define UNROLL_FACTOR 16
void setup(ap_uint< 512 >* target, float* local_target, ap_uint< 512 >* center_table, int* local_center_table)
{
#pragma HLS INLINE OFF
    memcpy_wide_bus_read_float(local_target, (ap_uint< 512 >*) target, 0, DIM * sizeof(float));
    memcpy_wide_bus_read_int(local_center_table, (ap_uint< 512 >*) center_table, 0, BATCH_SIZE * sizeof(int));
    //memcpy(local_center_table, center_table, BATCH_SIZE * sizeof(int));
}

void stage_0(int flag, int i, ap_uint< 512 >* coord, float* local_coord, ap_uint< 512 >* weight, float* local_weight, ap_uint< 512 >* cost, float* local_cost, ap_uint< 512 >* assign, int* local_assign)
{
#pragma HLS INLINE OFF
    if(flag){
        int index = i * BUF_SIZE;
        //memcpy(local_weight, weight + index, BUF_SIZE * sizeof(float));
        memcpy_wide_bus_read_float(local_weight, (ap_uint< 512 >*) weight, index * sizeof(float), BUF_SIZE * sizeof(float));

        //memcpy(local_assign, assign + index, BUF_SIZE * sizeof(int));
        memcpy_wide_bus_read_int(local_assign, (ap_uint< 512 >*) assign, index * sizeof(int), BUF_SIZE * sizeof(int));

        //memcpy(local_cost, cost + index, BUF_SIZE * sizeof(float));
        memcpy_wide_bus_read_float(local_cost, (ap_uint< 512 >*) cost, index * sizeof(float), BUF_SIZE * sizeof(float));

        //memcpy(local_coord, coord + index * DIM, BUF_SIZE * DIM * sizeof(float));
        memcpy_wide_bus_read_float(local_coord, (ap_uint< 512 >*) coord, index * DIM * sizeof(float), BUF_SIZE * DIM * sizeof(float));

    }
}


void stage_1(int flag, float* local_coord, float* local_weight, float* local_target, float* x_cost)
{
#pragma HLS INLINE OFF

    if(flag){
        int i, j, k;
        for( i = 0; i < BUF_SIZE; i++ ) {
        #pragma HLS pipeline
            float sum = 0;
            for( j = 0; j < 16; j++ ) {
            #pragma HLS pipeline
                float sum_coord[16];
                #pragma HLS array_partition variable=sum_coord complete
                memset(sum_coord, 0, 16 * sizeof(float));
                for(k = 0; k <16; k++){
                #pragma HLS unroll
                #pragma HLS dependence variable=local_coord inter false  
                #pragma HLS dependence variable=local_target inter false 
                #pragma HLS dependence variable=sum_coord inter false                
                    float a = local_coord[i * DIM + j * 16 + k] - local_target[j * 16 + k];
                    sum_coord[k] += a * a;
                }
            
            
                for(k = 0; k < 8; k++){
                #pragma HLS unroll
                #pragma HLS dependence variable=sum_coord inter false 
                    sum_coord[k] += sum_coord[k + 8];   
                }
                for(k = 0; k < 4; k++){
                #pragma HLS unroll
                #pragma HLS dependence variable=sum_coord inter false 
                    sum_coord[k] += sum_coord[k + 4];         
                }
                for(k = 0; k < 4; k++){
                #pragma HLS pipeline
                    sum += sum_coord[k];
                }
            
            }
            x_cost[i] = sum * local_weight[i];
        }
    }
    
}


void stage_2(int flag, int ii, int num, float* x_cost, float* local_cost, char* local_switch_membership, float* local_cost_of_opening_x, int* local_assign, int* local_center_table, float* local_work_mem)
{
#pragma HLS INLINE OFF
    if(flag){
/*
        int bound = (num - ii) > BUF_SIZE ? BUF_SIZE : num - ii;
        int bound1 = bound > HALF_BUF ? HALF_BUF : bound;
        int bound2 = bound - bound1;
        int local_bound[2] = {bound1, bound2};
        int i, j;
        float temp_work_mem[2][MAX_WORK_MEM_SIZE];
        #pragma HLS array_partition variable=temp_work_mem complete dim=1
        for(i = 0; i < 2; i++){
        #pragma HLS unroll
            for(j = 0; j < MAX_WORK_MEM_SIZE; j++){
            #pragma HLS pipeline
                temp_work_mem[i][j] = 0;
            }
        }

        float temp_cost[2] = {0, 0};
        #pragma HLS array_partition variable=temp_cost complete

        for (j = 0; j < 2; j++){
        #pragma HLS unroll
        #pragma HLS dependence variable=local_assign inter false  
        #pragma HLS dependence variable=local_cost inter false  
        #pragma HLS dependence variable=x_cost inter false  
        #pragma HLS dependence variable=local_switch_membership inter false  


            for (i = 0; i < local_bound[j]; i++) {
                int assign_index = local_assign[HALF_BUF * j + i];
                int local_center_index = local_center_table[assign_index];
                float current_cost = x_cost[i + HALF_BUF * j] - local_cost[HALF_BUF * j + i];

                if (current_cost < 0) {

                    // point i would save cost just by switching to x
                    // (note that i cannot be a median, 
                    // or else dist(p[i], p[x]) would be 0)
      
                    local_switch_membership[ii + HALF_BUF * j + i] = 1;
                    temp_cost[j] += current_cost;

                } 
                else {

                    // cost of assigning i to x is at least current assignment cost of i

                    // consider the savings that i's **current** median would realize
                    // if we reassigned that median and all its members to x;
                    // note we've already accounted for the fact that the median
                    // would save z by closing; now we have to subtract from the savings
                    // the extra cost of reassigning that median and its members 
                    temp_work_mem[j][local_center_index] -= current_cost;
                }
            }
        }
        *local_cost_of_opening_x = temp_cost[0] + temp_cost[1];
        for(i = 0; i <MAX_WORK_MEM_SIZE; i++){
        #pragma HLS unroll
            local_work_mem[i] = temp_work_mem[0][i] + temp_work_mem[1][i];
        }
*/
        int index = ii * BUF_SIZE;
        int bound = (num - index) > BUF_SIZE ? BUF_SIZE : num - index;
        int i;
        for (i = 0; i < bound; i++) {
	#pragma HLS pipeline
            int assign_index = local_assign[i];
            int local_center_index = local_center_table[assign_index];
            float current_cost = x_cost[i] - local_cost[i];

            if (current_cost < 0) {

                // point i would save cost just by switching to x
                // (note that i cannot be a median, 
                // or else dist(p[i], p[x]) would be 0)
      
                local_switch_membership[index + i] = 1;
                *local_cost_of_opening_x += current_cost;

            } 
            else {

                // cost of assigning i to x is at least current assignment cost of i

                // consider the savings that i's **current** median would realize
                // if we reassigned that median and all its members to x;
                // note we've already accounted for the fact that the median
                // would save z by closing; now we have to subtract from the savings
                // the extra cost of reassigning that median and its members 
                local_work_mem[local_center_index] -= current_cost;
            }
        }
    }
}

void result(float* work_mem, float* local_work_mem, int numcenter, ap_uint< 512 >* switch_membership, char* local_switch_membership, float* cost_of_opening_x, float local_cost_of_opening_x)
{
#pragma HLS INLINE OFF
    memcpy(work_mem, local_work_mem, numcenter * sizeof(float));
    memcpy_wide_bus_write_char((ap_uint< 512 >*)switch_membership, local_switch_membership, 0, BATCH_SIZE * sizeof(char));
    cost_of_opening_x[0] = local_cost_of_opening_x;
}
    
void pipeline_stage(int num, ap_uint< 512 >* coord, ap_uint< 512 >* weight, ap_uint< 512 >* assign, ap_uint< 512 >* cost, 
            int i, float* local_target, int* local_center_table, float* local_cost_of_opening_x, float* local_work_mem, char* local_switch_membership, 
            float* local_coord_stage_0, int* local_assign_stage_0, float* local_cost_stage_0, float* local_weight_stage_0,
            float* local_coord_stage_1, float* local_weight_stage_1, float* x_cost_stage_1,
            float* x_cost_stage_2, int* local_assign_stage_2, float* local_cost_stage_2
)
{
#pragma HLS INLINE OFF
    stage_0(i < BUF_NUM, i, coord, local_coord_stage_0, weight, local_weight_stage_0, cost, local_cost_stage_0, assign, local_assign_stage_0);
    stage_1(0 < i && i < BUF_NUM + 1, local_coord_stage_1, local_weight_stage_1, local_target, x_cost_stage_1);
    stage_2(1 < i, i - 2, num, x_cost_stage_2, local_cost_stage_2, local_switch_membership, local_cost_of_opening_x, local_assign_stage_2, local_center_table, local_work_mem);
}

extern "C" {
void workload(    
    ap_uint< 512 >* coord,                      
    ap_uint< 512 >* weight,                      
    ap_uint< 512 >* cost, 
    ap_uint< 512 >* target,
    ap_uint< 512 >* assign,
    ap_uint< 512 >* center_table,
    ap_uint< 512 >* switch_membership,
    float* work_mem,
    int num,
    float* cost_of_opening_x,
    int numcenter            
)
{
#pragma HLS INTERFACE m_axi port=coord offset=slave bundle=gmema
#pragma HLS INTERFACE m_axi port=weight offset=slave bundle=gmema
#pragma HLS INTERFACE m_axi port=cost offset=slave bundle=gmema
#pragma HLS INTERFACE m_axi port=target offset=slave bundle=gmema
#pragma HLS INTERFACE m_axi port=assign offset=slave bundle=gmema
#pragma HLS INTERFACE m_axi port=center_table offset=slave bundle=gmema
#pragma HLS INTERFACE m_axi port=switch_membership offset=slave bundle=gmema
#pragma HLS INTERFACE m_axi port=work_mem offset=slave bundle=gmemf
#pragma HLS INTERFACE m_axi port=cost_of_opening_x offset=slave bundle=gmemf
#pragma HLS INTERFACE s_axilite port=coord bundle=control
#pragma HLS INTERFACE s_axilite port=weight bundle=control
#pragma HLS INTERFACE s_axilite port=cost bundle=control
#pragma HLS INTERFACE s_axilite port=target bundle=control
#pragma HLS INTERFACE s_axilite port=assign bundle=control
#pragma HLS INTERFACE s_axilite port=center_table bundle=control
#pragma HLS INTERFACE s_axilite port=switch_membership bundle=control
#pragma HLS INTERFACE s_axilite port=work_mem bundle=control
#pragma HLS INTERFACE s_axilite port=num bundle=control
#pragma HLS INTERFACE s_axilite port=cost_of_opening_x bundle=control
#pragma HLS INTERFACE s_axilite port=numcenter bundle=control
#pragma HLS INTERFACE s_axilite port=return bundle=control
    

    float local_target[DIM];
    #pragma HLS array_partition variable=local_target cyclic factor=16

    int local_center_table[BATCH_SIZE];
    #pragma HLS array_partition variable=local_center_table cyclic factor=16

    float local_cost_of_opening_x = 0;
    float local_work_mem[MAX_WORK_MEM_SIZE];
    memset(local_work_mem, 0, MAX_WORK_MEM_SIZE * sizeof(float));
    char local_switch_membership[BATCH_SIZE];    
    #pragma HLS array_partition variable=local_switch_membership cyclic factor=64

    memset(local_switch_membership, 0, BATCH_SIZE * sizeof(char));


    setup(target, local_target, center_table, local_center_table);


    int i;
    int count = 0;
    for (i = 0; i < BUF_NUM + 2; i++) {
        static int local_assign_0[BUF_SIZE];
        #pragma HLS array_partition variable=local_assign_0 cyclic factor=16

        static float local_cost_0[BUF_SIZE];
        #pragma HLS array_partition variable=local_cost_0 cyclic factor=16

        static float local_weight_0[BUF_SIZE];
        #pragma HLS array_partition variable=local_weight_0 cyclic factor=16

        static float local_coord_0[BUF_SIZE * DIM];
        #pragma HLS array_partition variable=local_coord_0 cyclic factor=16
        static float x_cost_0[BUF_SIZE];

        static int local_assign_1[BUF_SIZE];
        #pragma HLS array_partition variable=local_assign_1 cyclic factor=16

        static float local_cost_1[BUF_SIZE];
        #pragma HLS array_partition variable=local_cost_1 cyclic factor=16

        static float local_weight_1[BUF_SIZE];
        #pragma HLS array_partition variable=local_weight_1 cyclic factor=16

        static float local_coord_1[BUF_SIZE * DIM];
        #pragma HLS array_partition variable=local_coord_1 cyclic factor=16
        static float x_cost_1[BUF_SIZE];

        static int local_assign_2[BUF_SIZE];
        #pragma HLS array_partition variable=local_assign_2 cyclic factor=16

        static float local_cost_2[BUF_SIZE];
        #pragma HLS array_partition variable=local_cost_2 cyclic factor=16

        static float local_weight_2[BUF_SIZE];
        #pragma HLS array_partition variable=local_weight_2 cyclic factor=16

        static float local_coord_2[BUF_SIZE * DIM];
        #pragma HLS array_partition variable=local_coord_2 cyclic factor=16
        static float x_cost_2[BUF_SIZE];

        if (count == 0)
            pipeline_stage(num, coord, weight, assign, cost, 
                            i, local_target, local_center_table, &local_cost_of_opening_x, local_work_mem, local_switch_membership, 
                            local_coord_0, local_assign_0, local_cost_0, local_weight_0,
                            local_coord_1, local_weight_1, x_cost_1,
                            x_cost_2, local_assign_2, local_cost_2
            );

        else if (count == 1) 
            pipeline_stage(num, coord, weight, assign, cost,
                            i, local_target, local_center_table, &local_cost_of_opening_x, local_work_mem, local_switch_membership, 
                            local_coord_2, local_assign_2, local_cost_2, local_weight_2,
                            local_coord_0, local_weight_0, x_cost_0,
                            x_cost_1, local_assign_1, local_cost_1
            );
    
        else 
            pipeline_stage(num, coord, weight, assign, cost,
                            i, local_target, local_center_table, &local_cost_of_opening_x, local_work_mem, local_switch_membership, 
                            local_coord_1, local_assign_1, local_cost_1, local_weight_1,
                            local_coord_2, local_weight_2, x_cost_2,
                            x_cost_0, local_assign_0, local_cost_0
            );    
        
        count++;
        if (count == 3) 
            count = 0;
    }

    result(work_mem, local_work_mem, numcenter, switch_membership, local_switch_membership, cost_of_opening_x, local_cost_of_opening_x);
    return;
}
}