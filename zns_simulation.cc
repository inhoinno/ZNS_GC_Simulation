/* 2020. 09. 22
 * Creater : Gunhee Choi
 * Modifier : Hojin Shin
 * This file implements the functions of Simulation Class
*/

#include <time.h>
#include "zns_simulation.h"
#include "zns_simulation_datastructure.h"
#include <cmath>

ZNS_Simulation::ZNS_Simulation(char * path, int zone_count, float zone_util) {
        zns_info_list = (struct zns_share_info *) malloc(sizeof(struct zns_share_info));
        m2_zns_init(path, zns_info_list);

        //ZNS SSD status print
        //m2_zns_init_print(zns_info_list);
        
        //init Argument
        Zone_count = zone_count;
        Zone_util = zone_util;
        Segment_count = SEGMENT_COUNT_IN_ZONE;
        Block_count = BLOCK_COUNT_IN_SEGMENT;
        total_segment_count = Zone_count * Segment_count;
        total_block_count = total_segment_count * Block_count;
        
        //init bitmap
        init_zone_bitmap();

        //init ZNS SSD
        init_all_zones_reset();
        //init_zones_write(Zone_count);
        
        current_i_block_bitmap = 0;
}

//***************** init function *****************//

void ZNS_Simulation::init_block_bitmap() {
    Block_bitmap = new SIM_Block[total_block_count];
    int valid_cnt = 0; float zone_util;
    srand(time(NULL));

    cout<<endl<<"Init Block_bitmap ...ing"<<endl;
    
    // set data valid by using utilization
    for(int i=0; i<Zone_count; i++) {
        int start_block = Zone_bitmap[i].get_i_start_block();
        int end_block = start_block + (SEGMENT_COUNT_IN_ZONE * BLOCK_COUNT_IN_SEGMENT);
        for(int j = start_block; j <= end_block; j++) {
            int seed = rand() % 100 + 1;
            //cout << "2" << endl;
            Block_bitmap[j].set_block_info(j);
            if (seed >=1 && seed <=Zone_util) {
                //cout << "3" << endl;
                Block_bitmap[j].set_state(VALID_BLOCK);
                valid_cnt++;
            } else Block_bitmap[j].set_state(INVALID_BLOCK);
        }
        zone_util = (valid_cnt / (float)(SEGMENT_COUNT_IN_ZONE * BLOCK_COUNT_IN_SEGMENT)) * 100;
        Zone_bitmap[i].set_utilization(zone_util);
        valid_cnt = 0;
    }

    cout<<"Finish Init Block_bitmap"<<endl;
    cout<< "#############################" <<endl;

    cout<<endl<<"Block Info First & Last"<<endl;
    cout<< "#############################" <<endl;
    cout<< "[First Block Info]" << endl;
    print_block_info(0);
    cout<< "[Last Block Info]" << endl;
    print_block_info(total_block_count-1);
    cout<<endl;
}

void ZNS_Simulation::init_segment_bitmap() {
        Segment_bitmap = new SIM_Segment[total_segment_count];
        
        cout<<endl<<"Init Segment_bitmap ...ing"<<endl;
        for(int i=0; i<total_segment_count; i++) {
            Segment_bitmap[i].set_segment_info(i, i * BLOCK_COUNT_IN_SEGMENT, WARM_SEGMENT);
        }
        cout<<"Finish Init Segment_bitmap"<<endl;
        cout<< "#############################" <<endl;
        
        cout<<endl<<"Segment Info First & Last"<<endl;
        cout<< "#############################" <<endl;
        cout<< "[First Segment Info]" << endl;
        print_segment_info(0);
        cout<< "[Last Segment Info]" << endl;
        print_segment_info(total_segment_count-1);
        cout<<endl;

        init_block_bitmap();
}

void ZNS_Simulation::init_zone_bitmap() {
        float z_util;

        Zone_bitmap = new SIM_Zone[Zone_count];

        cout<<endl<<"Init Zone_bitmap ...ing"<<endl;
        for(int i=0; i<Zone_count; i++) {
            Zone_bitmap[i].set_zone_info(i, i * SEGMENT_COUNT_IN_ZONE, i * SEGMENT_COUNT_IN_ZONE * SEGMENT_COUNT_IN_ZONE);
        }
        cout<<"Finish Init Zone_bitmap"<<endl;
        cout<< "#############################" <<endl;

        cout<<endl<<"Zone Info First & Last"<<endl;
        cout<< "#############################" <<endl;
        cout<< "Total number of Zones : ";
        cout << zns_info_list->totalzones << endl;
        cout<< "[First Zone Info]" << endl;
        print_zone_info(0);
        cout<< "[Last Zone Info]" << endl;
        print_zone_info(Zone_count-1);
        cout<<endl;

        init_segment_bitmap();
}

//***************** End init function *****************//

int ZNS_Simulation::write_data() {

    /*
    void * buffer_128KB_write = malloc(512 * 8 * 32);
    memset(buffer_128KB_write, 99, 512 * 8 * 32);

    zns_zone_reset_request(zns_info_list->fd, 0);
    zns_write(zns_info_list, buffer_128KB_write, 512 * 8 * 32, 0, 0);

    return 0;
    */
}

int ZNS_Simulation::read_valid_data(int i_block_offset) { 
    int index = i_block_offset;
    int read_count = 0;

    while(1) {
        //cout << Block_bitmap[index].get_data() << endl;
        if (Block_bitmap[index].get_state() == VALID_BLOCK) {
            read_count++;
            index++;
        } else break;
 
        if (read_count == 32) break; // Max IO size 128KB (4KB * 32)
    } 

    //cout<< "read data in func" <<endl;

    return read_count;
}

int ZNS_Simulation::basic_zgc() {
    cout<< "Start Basic ZGC" <<endl;
    int sel_zone = Zone_count;
    int max_zone_num = zns_info_list->totalzones;
    //cout << sel_zone << endl;
    int i_zone, i_segment, i_block;

    int collection_invalid_count = 0;
    int i_bitmap_current;
    int read_valid_count;
    int io_result = 0;

    int i_zone_start_block, i_zone_end_block;

    Block_data * buffer_128KB_read = (Block_data *)malloc(512 * 8 * 32);
    Block_data * buffer_128KB_write = (Block_data *)malloc(512 * 8 * 32);

    memset(buffer_128KB_read, 0, 512 * 8 * 32);
    memset(buffer_128KB_write, 0, 512 * 8 * 32);

    int offset_write_zone = 0;
    int i_current_write_buffer = 0; //Max 32

    int buffer_write_temp_offset;
    int remain_read_offset;

    //cout << "through here 1" << endl;

    for(i_zone = 0; i_zone < Zone_count; i_zone++) {
        //cout << "through here 2" << endl;

        i_zone_start_block = Zone_bitmap[i_zone].get_i_start_block();
        i_zone_end_block = Zone_bitmap[i_zone].get_i_start_block() + SEGMENT_COUNT_IN_ZONE * BLOCK_COUNT_IN_SEGMENT;

        if(Zone_bitmap[i_zone].get_valid_blocks(Segment_bitmap, Block_bitmap) == 0) continue;
        
        //cout << "through here 3" << endl;

        for(i_bitmap_current = i_zone_start_block; i_bitmap_current <= i_zone_end_block; ) {
            //cout << "through here loop" << endl;
            if (Block_bitmap[i_bitmap_current].get_state() == INVALID_BLOCK) {
                collection_invalid_count++;
                i_bitmap_current++;
                //cout << collection_invalid_count << " " << i_bitmap_current << endl;
                continue;
            }
        
            read_valid_count = read_valid_data(i_bitmap_current);
            //cout << read_valid_count << endl;
            
            if (read_valid_count == 0) {
                i_bitmap_current++;
                continue;
            } 

            // How to pick ZNS SSD Zone number ??
            io_result = m2_zns_read(zns_info_list, buffer_128KB_read, 512 * 8 * read_valid_count, 
                i_zone, (i_bitmap_current - 512 * 512 * i_zone) * 8);
            
            if(io_result == 0) {
                //cout << "through here read success" << endl;
                i_bitmap_current += read_valid_count;
            } else {
                cout << "goto Basic_GC_end" << endl;
                cout << "read_count : " << read_valid_count << endl;
                cout << "IO result : " << io_result << endl;
                goto Basic_GC_end;
            }

            // Buffer write (write before fill 128KB )
            // How to pick ZNS SSD Zone write
            if( (i_current_write_buffer + read_valid_count) < 32 ) { //check 128KB
                //cout << "through here 4" << endl;
                memcpy(&buffer_128KB_write[i_current_write_buffer], buffer_128KB_read, 512 * 8 * read_valid_count);
                i_current_write_buffer += read_valid_count;
            } else {
                if((i_current_write_buffer + read_valid_count) == 32) { //128KB write
                    //cout << "through here 5" << endl;
                    memcpy(&buffer_128KB_write[i_current_write_buffer], buffer_128KB_read, 512 * 8 * read_valid_count);
                    if (sel_zone > max_zone_num) sel_zone = 0;
                    io_result = m2_zns_write(zns_info_list, buffer_128KB_write, 512 * 8 * 32, sel_zone, offset_write_zone * 8);
                    if(io_result == 0) {
                        offset_write_zone += 32;
                        i_current_write_buffer = 0;
                    } else {
                        cout << "write IO result : " << io_result << endl;
                    }
                } else {
                    remain_read_offset = i_current_write_buffer + read_valid_count - 32;
                    memcpy(&buffer_128KB_write[i_current_write_buffer], buffer_128KB_read, 512 * 8 * (read_valid_count-remain_read_offset));
                    if (sel_zone > max_zone_num) sel_zone = 0;
                    io_result = m2_zns_write(zns_info_list, buffer_128KB_write, 512 * 8 * 32, sel_zone, offset_write_zone * 8);
                    if(io_result == 0) {
                        offset_write_zone += 32;
                        i_current_write_buffer = 0;
                    } else {
                        cout << "write IO result : " << io_result << endl;
                    }

                    memcpy(&buffer_128KB_write[i_current_write_buffer], &buffer_128KB_read[read_valid_count-remain_read_offset],
                        512 * 8 * remain_read_offset);
                    i_current_write_buffer += remain_read_offset;
                }
            }

            if(offset_write_zone  == (512 * 512)) {
                //cout << "through here 6" << endl;
                if (sel_zone > max_zone_num) sel_zone = 0;
                m2_zns_zone_finish(zns_info_list, sel_zone);
                sel_zone++;
                offset_write_zone = 0;
                cout << "Up to Max size of Zone, Move to next Zone!!" << endl;
                cout << "write zone : " << sel_zone << endl;
            }

            if(collection_invalid_count >= (512 * 512)) {
                //cout << "through here 7" << endl;
                if(i_current_write_buffer != 0) {
                    if (sel_zone > max_zone_num) sel_zone = 0;
                    io_result = m2_zns_write(zns_info_list, buffer_128KB_write, 512 * 8 * i_current_write_buffer,
                        sel_zone, offset_write_zone * 8);
                }
                goto Basic_GC_end;
            }
        } // end of Block in Zone
        
        m2_zns_zone_reset(zns_info_list, i_zone);
        Zone_bitmap[i_zone].reset_valid_blocks();
    } // end of Zone

Basic_GC_end: 
    printf("Basic ZGC End\n");
    
    return 0;
}

int ZNS_Simulation::lsm_zgc() {
    cout<< "Start LSM ZGC" <<endl;
    /*
    if(workload_list->size() == 0) {
        cout<< "Workload empty" <<endl;
        return -1;
    }

    list<Workload_Data * >::iterator iter;
    for (iter = workload_list->begin(); iter != workload_list->end(); ++iter){
        cout<< (*iter)->get_SN() << ", " << (*iter)->get_start_lba() <<endl;
        //(*iter)->print_workload_data();
    }
    */
    return 0;
}

int ZNS_Simulation::print_zns_totalzones() {
    cout << "fucking" << endl;
    cout << zns_info_list->totalzones << endl;
    return zns_info_list->totalzones;
}

int ZNS_Simulation::request_write(int start_lba, int blocks) {
    int numofrequest = blocks/8;  
    int reqeust_lbs = start_lba;

    if( (blocks%8) != 0 ) {
        numofrequest += 1;
    }

    for(int i= 0; i<numofrequest; i++) {
        if(this->serialize_map.find(reqeust_lbs) == this->serialize_map.end()) {
            //Not found start lba
            this->serialize_map.insert(make_pair(reqeust_lbs, current_i_block_bitmap));
            current_i_block_bitmap++;
        } else {
            //Found start lba
            this->serialize_map.find(100)->second = current_i_block_bitmap;
        }
    }

    for(int i= 0; i<numofrequest; i++) {
        cout<< "count : " << i << "/" << (numofrequest-1) <<endl;
        reqeust_lbs += 8;
        cout<< "start_lba : " << reqeust_lbs <<endl;
        cout<<endl;
    }
    
    return 0;
}

int ZNS_Simulation::init_zones_write(int numofzones) {
    int i_zone, i_segment, i_block, i_bitmap;
    int offset;
    int write_result;

    void * dummy_data = new char[ZNS_BLOCK_SIZE * 32];
    memset(dummy_data, 66, ZNS_BLOCK_SIZE * 32);

    cout << endl << "Init ZNS SSD Write ...ing" << endl;
    for(i_zone=0; i_zone<numofzones; i_zone++) {
        for (offset = 0; offset < 8192; offset++) {
            write_result = m2_zns_write(zns_info_list, dummy_data, ZNS_BLOCK_SIZE * 32, i_zone, offset * 256);
        }
        m2_zns_zone_finish(zns_info_list, i_zone);
        /*
        cout<< "Fill Zone " << i_zone <<endl;
        for(i_segment=0; i_segment<SEGMENT_COUNT_IN_ZONE; i_segment++) {

            for(i_block=0; i_block<BLOCK_COUNT_IN_SEGMENT; i_block++) {
                offset = get_offset_in_zone(i_segment, i_block);
                i_bitmap = get_i_bitmap(i_zone, i_segment, i_block);

                write_result = m2_zns_write(zns_info_list, dummy_data, ZNS_BLOCK_SIZE, i_zone, offset);
                //if(write_result == 0)
                    //block_bitmap[i_bitmap] = 1;
            }//end of blocks

        }//end of segments
        m2_zns_zone_finish(zns_info_list, i_zone);
        */
    }//end of zones

    cout << "Finish Init ZNS SSD Write" << endl;

    free(dummy_data);
    return 0;
}

void ZNS_Simulation::init_zone_reset(int numofzones) {
    int i_zone;

    cout<< "Zone Init Reset" <<endl;
    for(i_zone=0; i_zone<numofzones; i_zone++) {
        m2_zns_zone_reset(zns_info_list, i_zone);
    }
}

void ZNS_Simulation::init_all_zones_reset() {
    int i_zone = 0;
    cout << "Reset ZNS SSD ...ing" << endl;
    for (i_zone = 0; i_zone < zns_info_list->totalzones; i_zone++) {
        m2_zns_zone_reset(zns_info_list, i_zone);
    }
    cout << "Finish Reset ZNS SSD" << endl;
}

int ZNS_Simulation::get_offset_in_zone(int i_segment, int i_block) {
    return 0; //i_segment * ZONE_IN_SEGMENT_COUNT * SIM_BLOCK_IO_DEFAULT + i_block * SIM_BLOCK_IO_DEFAULT;
}

int ZNS_Simulation::get_i_bitmap(int i_zone, int i_segment, int i_block) {
    return 0; //i_zone * ZONE_IN_BLOCK_COUNT + i_segment * ZONE_IN_SEGMENT_COUNT + i_block;
}

//***************** test workload function *****************//

int ZNS_Simulation::setting_random_bitmap() {
    int i_block, i_segment, i_zone;

    random_device rd;
    std::mt19937 mersenne(rd());
    std::uniform_int_distribution<> dis(0, total_block_count);

    /*
    i_block =
        i_zone * SEGMENT_COUNT_IN_ZONE * BLOCK_COUNT_IN_SEGMENT
        + i_segment * BLOCK_COUNT_IN_SEGMENT
        + offset
    */
    i_block =  dis(mersenne);

    //find zone
    i_zone = i_block / (SEGMENT_COUNT_IN_ZONE * BLOCK_COUNT_IN_SEGMENT);

    //find segment
    i_segment = i_block - (i_zone * SEGMENT_COUNT_IN_ZONE * BLOCK_COUNT_IN_SEGMENT);
    i_segment = i_segment / BLOCK_COUNT_IN_SEGMENT;


    
    return 0;
}

//***************** End test workload function *****************//

//***************** print function *****************//

void ZNS_Simulation::print_block_info(int offset) {
    Block_bitmap[offset].print_block_info();
}

void ZNS_Simulation::print_segment_info(int offset) {
    Segment_bitmap[offset].print_segment_info();
}

void ZNS_Simulation::print_zone_info(int offset) {
    Zone_bitmap[offset].print_zone_info();
}

void ZNS_Simulation::print_segment_block_bitmap(int i_segment) {
    int start_i_block = Segment_bitmap[i_segment].get_i_start_block();
    int end_i_block = start_i_block + BLOCK_COUNT_IN_SEGMENT;
    int list_cnt = 0; //int valid_cnt = 0;

    cout<< "Segment " << i_segment << " block_bitmap" <<endl;
    for(int i_block=start_i_block; i_block<end_i_block; i_block++) {
        list_cnt++;
        cout << Block_bitmap[i_block].get_state() << " ";
        /*if (Block_bitmap[i_block].get_state() == VALID_BLOCK) {
            valid_cnt++;
        }*/
        if((list_cnt % 20) == 0)
            cout<<endl;
    }
    //cout << endl << valid_cnt << endl;
    cout<<endl;
}

void ZNS_Simulation::print_zone_block_bitmap(int i_zone) {
    int start_i_block = Zone_bitmap[i_zone].get_i_start_block();
    int end_i_block = start_i_block + BLOCK_COUNT_IN_SEGMENT * SEGMENT_COUNT_IN_ZONE;
    int list_cnt = 0;

    cout<< "Zone " << i_zone << " block_bitmap" <<endl;
    for(int i_block=start_i_block; i_block<end_i_block; i_block++) {
        list_cnt++;
        cout << Block_bitmap[i_block].get_state() << " ";
        if((list_cnt % 512) == 0)
            cout<<endl;
    }
    cout<<endl;
}

void ZNS_Simulation::print_zone_segment_bitmap(int i_zone) {
    int start_i_segment = Zone_bitmap[i_zone].get_i_start_segment();
    int end_i_segment = start_i_segment + SEGMENT_COUNT_IN_ZONE;
    int t_status;
    int list_cnt = 0;

    cout<< "Zone " << i_zone << " segment_bitmap" <<endl;
    for(int i_segment=start_i_segment; i_segment<end_i_segment; i_segment++) {
        list_cnt++;
        t_status = Segment_bitmap[i_segment].get_status();

        if( t_status == WARM_SEGMENT )
            cout<< "W ";
        else if( t_status == HOT_SEGMENT )
            cout<< "H ";
        else if( t_status == COLD_SEGMENT )
            cout<< "C ";

        if((list_cnt % 20) == 0)
            cout<<endl;
    }
    cout<<endl;
}


//***************** End print function *****************//