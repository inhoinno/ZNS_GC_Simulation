/* 2020. 12. 15 - Date of initial creation
 * Creater : Hojin Shin
 * Modifier : -
 * This file is the ZNS SSD Simulation Workload Creator
*/

#include "workload_creator.h"

#include <string.h>
#include <stdio.h>
#include <iostream>
#include <time.h>
#include <math.h>


Workload_Creator::Workload_Creator(m2_zns_share_info * zonelist, int zone_count, int dev_num, int zone_util) {
    this->m2zns_info_list = zonelist;
    this->zone_util = zone_util;
    this->Zone_count = zone_count;
    this->Dev_num = dev_num;

    if (Dev_num == 1) 
        cal_util_block = Zone_count * (int)ceil(M2_SEGMENT_COUNT_IN_ZONE * M2_BLOCK_COUNT_IN_SEGMENT * (zone_util * 0.01));
    else if (Dev_num == 2)
        cal_util_block = Zone_count * (int)ceil(U3_SEGMENT_COUNT_IN_ZONE * U3_BLOCK_COUNT_IN_SEGMENT * (zone_util * 0.01));
    else if (Dev_num == 3)
        cal_util_block = Zone_count * (int)ceil(FEMU_SEGMENT_COUNT_IN_ZONE * FEMU_BLOCK_COUNT_IN_SEGMENT * (zone_util * 0.01));

}

Workload_Creator::Workload_Creator(int zone_count, int dev_num, int zone_util) {
    this->zone_util = zone_util;
    this->Zone_count = zone_count;
    this->Dev_num = dev_num;
    
    if (Dev_num == 1) 
        cal_util_block = Zone_count * (int)ceil(M2_SEGMENT_COUNT_IN_ZONE * M2_BLOCK_COUNT_IN_SEGMENT * (zone_util * 0.01));
    else if (Dev_num == 2)
        cal_util_block = Zone_count * (int)ceil(U3_SEGMENT_COUNT_IN_ZONE * U3_BLOCK_COUNT_IN_SEGMENT * (zone_util * 0.01));
    else if (Dev_num == 3)
        cal_util_block = Zone_count * (int)ceil(FEMU_SEGMENT_COUNT_IN_ZONE * FEMU_BLOCK_COUNT_IN_SEGMENT * (zone_util * 0.01));


}
Workload_Creator::Workload_Creator(int zone_count, int zone_util) {
    this->zone_util = zone_util;
    this->Zone_count = zone_count;
    //this->Dev_num = dev_num;

    cal_util_block = Zone_count * (int)ceil(FEMU_SEGMENT_COUNT_IN_ZONE * FEMU_BLOCK_COUNT_IN_SEGMENT * (zone_util * 0.01));
    cout << " cal_util_block " << cal_util_block << endl;
    cout << "Zone_count " << Zone_count << "ceil " <<(int)ceil(FEMU_SEGMENT_COUNT_IN_ZONE * FEMU_BLOCK_COUNT_IN_SEGMENT * (zone_util * 0.01)) <<" cal_util_block " << cal_util_block << endl;

}

int *Workload_Creator::create_sequential_workload(SIM_Zone * Zone_bitmap, SIM_Segment * Segment_bitmap, SIM_Block * Block_bitmap) {
    update_block = new int[cal_util_block];
    //int total_block_num = Zone_count * 512 * 512 - 1;
    int total_block_num = (Zone_count * FEMU_SEGMENT_COUNT_IN_ZONE * FEMU_BLOCK_COUNT_IN_SEGMENT) - 1;
    int i_block = 0, n_zone_block;
    int block_off, cond_off = 0;

    if (Dev_num == 1)
            n_zone_block = (int)ceil(M2_SEGMENT_COUNT_IN_ZONE * M2_BLOCK_COUNT_IN_SEGMENT * (zone_util * 0.01));
    else if (Dev_num == 2)
            n_zone_block = (int)ceil(U3_SEGMENT_COUNT_IN_ZONE * U3_BLOCK_COUNT_IN_SEGMENT * (zone_util * 0.01));
    else if (Dev_num == 3)
            n_zone_block = (int)ceil(FEMU_SEGMENT_COUNT_IN_ZONE * FEMU_BLOCK_COUNT_IN_SEGMENT * (zone_util * 0.01));

    for (int i_zone = 0; i_zone < Zone_count; i_zone++) {
        block_off = Zone_bitmap[i_zone].get_i_start_block();
        
        cond_off = block_off + n_zone_block;
        cout << " cond_off "<< cond_off <<"= block_off "<< block_off <<" + n_zone_block; "<< n_zone_block << endl;
        for(;block_off < cond_off; block_off++) {
            update_block[i_block] = block_off;
            i_block++;
        }
        if (i_block > cal_util_block) {
            cout << "Too many update blocks ...stop update" << endl;
            exit(0);
        }
    }

    return update_block;    
}

int *Workload_Creator::create_random_workload(SIM_Zone * Zone_bitmap, SIM_Segment * Segment_bitmap, SIM_Block * Block_bitmap) {
    update_block = new int[cal_util_block];
    srand(time(NULL));

    int total_block_num = (Zone_count * FEMU_SEGMENT_COUNT_IN_ZONE * FEMU_BLOCK_COUNT_IN_SEGMENT) - 1;
    int i_block = 0, n_zone_block, start_block;
    int block_off, cond_off = 0;
    int zone_in_block;

    if (Dev_num == 1)
        zone_in_block = M2_SEGMENT_COUNT_IN_ZONE * M2_BLOCK_COUNT_IN_SEGMENT;
    else if (Dev_num == 2)
        zone_in_block = U3_SEGMENT_COUNT_IN_ZONE * U3_BLOCK_COUNT_IN_SEGMENT;
    else if (Dev_num == 3)
        zone_in_block = FEMU_SEGMENT_COUNT_IN_ZONE * FEMU_BLOCK_COUNT_IN_SEGMENT;
    //zone_in_block  = 32768
    for (int i_zone = 0; i_zone < Zone_count; i_zone++) {
        start_block = Zone_bitmap[i_zone].get_i_start_block();
        block_off   = Zone_bitmap[i_zone].get_i_start_block();

        if (Dev_num == 1){
            n_zone_block = (int)ceil(M2_SEGMENT_COUNT_IN_ZONE * M2_BLOCK_COUNT_IN_SEGMENT       * (zone_util * 0.01));
        }
        else if (Dev_num == 2){
            n_zone_block = (int)ceil(U3_SEGMENT_COUNT_IN_ZONE * U3_BLOCK_COUNT_IN_SEGMENT       * (zone_util * 0.01));
        }
        else if (Dev_num == 3){
            n_zone_block = FEMU_SEGMENT_COUNT_IN_ZONE * FEMU_BLOCK_COUNT_IN_SEGMENT;
            n_zone_block = (int)(n_zone_block * zone_util / 100);
        }

        //cond_off = start block offset + number zone blocks to make util () 
        cond_off = block_off + n_zone_block;
        //block off ~ cond off

        for(;block_off < cond_off; block_off++) {
            update_block[i_block] = start_block + (rand() % zone_in_block);
            i_block++;
        }
        if (i_block > (cal_util_block) ) {
            cout << " cond_off "<< cond_off << "  block_off "<< block_off <<" n_zone_block " << n_zone_block <<endl;
            cout << " i_block  "<< i_block << " zone_uitl "<< (zone_util * 0.01) << "  cal_util_block  "<< cal_util_block <<endl;
            cout << "Too many update blocks ...stop update "<< endl;
            exit(0);
        }
    }

    return update_block;
}

pair<int,int>Workload_Creator::m2_update_block_in_memory(SIM_Zone * Zone_bitmap, SIM_Segment * Segment_bitmap, SIM_Block * Block_bitmap, int * _update_bitmap) {
    int start_block = 0;
    int end_block;
    int update_cnt = 0;
    int io_result = 0;
    int sel_zone = Zone_count;
    int zone_update_count = 0;
    int write_offset = 0;
    
    void * dummy_data = new char[ZNS_BLOCK_SIZE * 32];  //4K * 32 = 128KB
    memset(dummy_data, 66, ZNS_BLOCK_SIZE * 32);

    end_block = Zone_count * M2_SEGMENT_COUNT_IN_ZONE * M2_BLOCK_COUNT_IN_SEGMENT;

    cout << "Update in memory ...ing" << endl;
    for (int i_block = start_block; i_block < end_block; i_block++) {
        for (int j_block = 0; j_block < cal_util_block; j_block++) {
            if (i_block == _update_bitmap[j_block]) {
                Block_bitmap[i_block].set_state(INVALID_BLOCK);
                update_cnt++;
                break;
            }
        }
        if (update_cnt == 32) {
            io_result = m2_zns_write(m2zns_info_list, dummy_data, (512 * 8 * update_cnt), sel_zone, write_offset * 8);
            if (io_result == 0) {
                write_offset += update_cnt;
                update_cnt = 0;
            } else {
                cout << "ZNS SSD Write Fail" << endl;
                exit(0);
            }
        }
        if (write_offset == (512 * 512)) {
            m2_zns_zone_finish(m2zns_info_list, sel_zone);
            sel_zone++;
            zone_update_count++;
            write_offset = 0;
            cout << "Up to Max size of Zone, Move to next Zone!!" << endl;
            cout << "write zone : " << sel_zone << endl;
        }
    }
    if (update_cnt < 32) {
        io_result = m2_zns_write(m2zns_info_list, dummy_data, (512 * 8 * 32), sel_zone, write_offset * 8);
        if (io_result == 0) {
            write_offset += 32;
            update_cnt = 0;
        } else {
            cout << "ZNS SSD Write Fail" << endl;
            exit(0);
        }
    }
    if (write_offset == (512 * 512)) {
        m2_zns_zone_finish(m2zns_info_list, sel_zone);
        sel_zone++;
        zone_update_count++;
        write_offset = 0;
        cout << "Up to Max size of Zone, Move to next Zone!!" << endl;
        cout << "Write zone : " << sel_zone << endl;
    }
    cout << "Finish Update in memory" << endl;

    return {write_offset, zone_update_count};
}

pair<int, int>Workload_Creator::u3_update_block_in_memory(SIM_Zone * Zone_bitmap, SIM_Segment * Segment_bitmap, SIM_Block * Block_bitmap, int * _update_bitmap) {
    int start_block = 0;
    int end_block;
    int update_cnt = 0;
    int io_result = 0;
    int sel_zone = Zone_count;
    int zone_update_count = 0;
    int write_offset = 0;
    
    void * dummy_data = new char[ZNS_BLOCK_SIZE * 48];
    memset(dummy_data, 66, ZNS_BLOCK_SIZE * 48);

    end_block = Zone_count * U3_SEGMENT_COUNT_IN_ZONE * U3_BLOCK_COUNT_IN_SEGMENT;
    cout << "Update in memory ...ing" << endl;
    for (int i_block = start_block; i_block < end_block; i_block++) {
        for (int j_block = 0; j_block < cal_util_block; j_block++) {
            if (i_block == _update_bitmap[j_block]) {
                Block_bitmap[i_block].set_state(INVALID_BLOCK);
                update_cnt++;
                break;
            }
        }
        if (update_cnt == 48) {
            io_result = u3_zns_write(dummy_data, _192KB, sel_zone);
            if (io_result == 0) {
                write_offset += update_cnt;
                update_cnt = 0;
            } else {
                cout << "ZNS SSD Write Fail" << endl;
                exit(0);
            }
        }
        if (write_offset == (512 * 36)) {
            u3_zns_set_zone(sel_zone, MAN_FINISH);
            sel_zone++;
            zone_update_count++;
            write_offset = 0;
            cout << "Up to Max size of Zone, Move to next Zone!!" << endl;
            cout << "write zone : " << sel_zone << endl;
        }
    }
    if (update_cnt < 36) {
        io_result = u3_zns_write(dummy_data, _192KB, sel_zone);
        if (io_result == 0) {
            write_offset += 48;
            update_cnt = 0;
        } else {
            cout << "ZNS SSD Write Fail" << endl;
            exit(0);
        }
    }
    if (write_offset == (512 * 36)) {
        u3_zns_set_zone(sel_zone, MAN_FINISH);
        sel_zone++;
        zone_update_count++;
        write_offset = 0;
        cout << "Up to Max size of Zone, Move to next Zone!!" << endl;
        cout << "Write zone : " << sel_zone << endl;
    }
    cout << "Finish Update in memory" << endl;

    return {write_offset, zone_update_count};
}

pair<int, int>Workload_Creator::update_block_in_memory(SIM_Zone * Zone_bitmap, SIM_Segment * Segment_bitmap, SIM_Block * Block_bitmap, int * _update_bitmap) {
    int start_block = 0;
    int end_block;
    int update_cnt = 0;
    int io_result = 0;
    int sel_zone = Zone_count;
    int zone_update_count = 0;
    int write_offset = 0;
    
    void * dummy_data = new char[_128KB];
    memset(dummy_data, 66, _128KB);

    end_block = Zone_count * FEMU_SEGMENT_COUNT_IN_ZONE * FEMU_BLOCK_COUNT_IN_SEGMENT;
    cout << "Update in memory ...ing" << endl;
    for (int i_block = start_block; i_block < end_block; i_block++) {
        for (int j_block = 0; j_block < cal_util_block; j_block++) {
            if (i_block == _update_bitmap[j_block]) {
                Block_bitmap[i_block].set_state(INVALID_BLOCK);
                update_cnt++;
                break;
            }
        }
        if (update_cnt == (_128KB / ZNS_BLOCK_SIZE )) {
            io_result = zns_write(dummy_data, _128KB, sel_zone);
            if (io_result == 0) {
                write_offset += update_cnt;
                update_cnt = 0;
            } else {
                cout << "ZNS SSD Write Fail" << endl;
                exit(0);
            }
        }
        if (write_offset == ((FEMU_BLOCK_COUNT_IN_SEGMENT * FEMU_SEGMENT_COUNT_IN_ZONE))) {   
            zns_set_zone(sel_zone, MAN_FINISH);
            sel_zone++;
            zone_update_count++;
            write_offset = 0;
            cout << "Up to Max size of Zone, Move to next Zone!!" << endl;
            cout << "write zone : " << sel_zone << endl;
        }
    }
    if (update_cnt < FEMU_SEGMENT_COUNT_IN_ZONE) {
        io_result = zns_write(dummy_data, _128KB, sel_zone);
        if (io_result == 0) {
            write_offset += (_128KB / ZNS_BLOCK_SIZE );
            update_cnt = 0;
        } else {
            cout << "ZNS SSD Write Fail" << endl;
            exit(0);
        }
    }
    if (write_offset == (FEMU_BLOCK_COUNT_IN_SEGMENT* FEMU_SEGMENT_COUNT_IN_ZONE)) {
        zns_set_zone(sel_zone, MAN_FINISH);
        sel_zone++;
        zone_update_count++;
        write_offset = 0;
        cout << "Up to Max size of Zone, Move to next Zone!!" << endl;
        cout << "Write zone : " << sel_zone << endl;
    }
    cout << "Finish Update in memory" << endl;

    return {write_offset, zone_update_count};
}

pair<int, int>Workload_Creator::inter_update_block_in_memory(SIM_Zone * Zone_bitmap, SIM_Segment * Segment_bitmap, SIM_Block * Block_bitmap, int * _update_bitmap) {
    int start_block = 0;
    int end_block;
    int update_cnt = 0;
    int io_result = 0;
    int chnlway = 32;
    int sel_zone = Zone_count;
    int zone_update_count = 0;
    int write_offset = 0;
    
    void * dummy_data = new char[_128KB];
    memset(dummy_data, 66, _128KB);

    end_block = Zone_count * FEMU_SEGMENT_COUNT_IN_ZONE * FEMU_BLOCK_COUNT_IN_SEGMENT;
    cout << "Update in memory ...ing" << endl;
    for (int i_block = start_block; i_block < end_block; i_block++) {
        for (int j_block = 0; j_block < cal_util_block; j_block++) {
            if (i_block == _update_bitmap[j_block]) {
                Block_bitmap[i_block].set_state(INVALID_BLOCK);
                update_cnt++;
                break;
            }
        }
        if (update_cnt == (_128KB / ZNS_BLOCK_SIZE )) {
            io_result = zns_write(dummy_data, _128KB, sel_zone);
            if (io_result == 0) {
                write_offset += update_cnt;
                update_cnt = 0;
            } else {
                cout << "ZNS SSD Write Fail" << endl;
                exit(0);
            }
        }
        if (write_offset == ((FEMU_BLOCK_COUNT_IN_SEGMENT* FEMU_SEGMENT_COUNT_IN_ZONE))) {   
            zns_set_zone(sel_zone, MAN_FINISH);
            sel_zone++;
            zone_update_count++;
            write_offset = 0;
            cout << "Up to Max size of Zone, Move to next Zone!!" << endl;
            cout << "write zone : " << sel_zone << endl;
        }
    }
    if (update_cnt < FEMU_SEGMENT_COUNT_IN_ZONE) {
        io_result = zns_write(dummy_data, _128KB, sel_zone);
        if (io_result == 0) {
            write_offset += (_128KB / ZNS_BLOCK_SIZE );
            update_cnt = 0;
        } else {
            cout << "ZNS SSD Write Fail" << endl;
            exit(0);
        }
    }
    if (write_offset == (FEMU_BLOCK_COUNT_IN_SEGMENT* FEMU_SEGMENT_COUNT_IN_ZONE)) {
        zns_set_zone(sel_zone, MAN_FINISH);
        sel_zone++;
        zone_update_count++;
        write_offset = 0;
        cout << "Up to Max size of Zone, Move to next Zone!!" << endl;
        cout << "Write zone : " << sel_zone << endl;
    }
    cout << "Finish Update in memory" << endl;

    return {write_offset, zone_update_count};
}