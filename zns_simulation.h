/* 2020. 09. 22 - Date of initial creation
 * Creater : Gunhee Choi
 * Modifier : Hojin Shin
 * This file defines the Simulation Class Header
*/

#ifndef ZNS_simulation_H
#define ZNS_simulation_H

#include "m2controller.h"
#include "u3controller.h"
#include "controller.h"

#include "zns_simulation_datastructure.h"
#include "workload_creator.h"

#include <cstring>
#include <random>
#include <iostream>

//In Simulation, Set hardware zone spec
#define M2_ZONE_MAX_COUNT 530
#define U3_ZONE_MAX_COUNT 29172
#define FEMU_ZONE_MAX_COUNT 512


//In Simulation, Set default IO size : 4KB = 512 * 8
#define SIM_BLOCK_IO_DEFAULT 8

//In Simulation, Set default Block size, block count : 4KB, 0x35e500
#define ZNS_BLOCK_SIZE _4KB
#define ZNS_ZONE_BLOCK_COUNT 0x35e500

//In Simulation, Set M2 Zone spec (512 * 4K = 2MB)
#define M2_BLOCK_COUNT_IN_SEGMENT 512
#define M2_SEGMENT_COUNT_IN_ZONE 512           //1024MB zsze

//In Simulation, Set U3 Zone spec
#define U3_BLOCK_COUNT_IN_SEGMENT 512
#define U3_SEGMENT_COUNT_IN_ZONE 36

//In Simulation, Set FEMU Zone spec. this can be chagned by FEMU setting
#define FEMU_BLOCK_COUNT_IN_SEGMENT 512         //4KB blocks * 512 = 2MB segment
#define FEMU_SEGMENT_COUNT_IN_ZONE 256          //2MB segment * 64 = 128MB zone

//Control zone, segment, block state
#define VALID_BLOCK 1
#define INVALID_BLOCK 2
#define FREE_BLOCK 0

#define HOT_SEGMENT 0
#define COLD_SEGMENT 1
#define WARM_SEGMENT 2

using namespace std;

class Block_data {
    char data[4096];
};

class ZNS_Simulation {
    //ZNS SSD init variable
    struct m2_zns_share_info * m2zns_info_list;
    struct zns_share_info * zns_info_list;
    
    //Setting ZNS SSD Argument
    int Zone_count;
    float Zone_util;
    int Segment_count;
    int Block_count;
    int Dev_num;

    //Workload block number
    struct Workload_Creator * workload_creator;
    int * update_bitmap;
    int update_write_offset;
    int update_zone_count;

    //Simualtion bitmap
    SIM_Zone * Zone_bitmap;
    SIM_Segment * Segment_bitmap;
    SIM_Block * Block_bitmap;
    int total_segment_count;
    int total_block_count;

    int current_i_block_bitmap;

public :
    ZNS_Simulation(char * path, int zone_count, int dev_util, int update_count);
    //Init function
    void init_block_bitmap();
    void init_segment_bitmap();
    void init_zone_bitmap();
    
    //GENRENAL 
    int basic_zgc();
    int lsm_zgc();
    int read_valid_data(int i_block);
    int read_valid_data_lsm(int i_block);
    int init_zones_write(int numofzones);
    void init_zone_reset(int numofzones);
    void init_all_zones_reset();
    //Inho : all codes below needs refactoring

    //M2 ZNS SSD GC
    int m2_basic_zgc();
    int m2_lsm_zgc();
    int m2_read_valid_data(int i_block);
    int m2_read_valid_data_lsm(int i_block);

    //U3 ZNS SSD GC
    int u3_basic_zgc();
    int u3_lsm_zgc();
    int u3_read_valid_data(int i_block);
    int u3_read_valid_data_lsm(int i_block);

    //M2 ZNS SSD function
    int m2_init_zones_write(int numofzones);
    void m2_init_zone_reset(int numofzones);
    void m2_init_all_zones_reset();

    //U3 ZNS SSD function
    int u3_init_zones_write(int numofzones);
    void u3_init_zone_reset(int numofzones);
    void u3_init_all_zones_reset();

    //Print function
    void print_block_info(int offset);
    void print_segment_info(int offset);
    void print_zone_info(int offset);
    void print_segment_block_bitmap(int i_segment);
    void print_zone_block_bitmap(int i_zone);
    void print_zone_segment_bitmap(int i_zone);

    //Request workload function
    int *request_sequential_workload();
    int *request_random_workload();
    void request_update_workload();
};

#endif
