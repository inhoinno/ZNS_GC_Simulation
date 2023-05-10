/* 2020. 09. 22 - Date of initial creation
 * Creater : Gunhee Choi
 * Modifier : Hojin Shin
 * This file implements the functions of Simulation Class
*/

#include "zns_simulation.h"
#include "controller.h"
#include <ctime>
#include <cmath>
int from = 0;//atoi(argv[3]);
int size = 10;//atoi(argv[4]);
FILE * output;
FILE * breakdown;
double read_time=0;
double write_time=0;
double reset_time=0;
double memcpy_time=0;

ZNS_Simulation::ZNS_Simulation(char * path, int zone_count, int zone_util, int dev_num) {
    if (dev_num == 1) {// M2 ZNS SSD Init
        m2zns_info_list = (struct m2_zns_share_info *) malloc(sizeof(struct m2_zns_share_info));
        m2_zns_init(path, m2zns_info_list);
    } else if (dev_num == 2) {// U3 ZNS SSD Init
        u3_zns_get_info(path);
        u3_zns_get_zone_desc(REPORT_ALL, REPORT_ALL_STATE, 0, 0, true);
    } else if (dev_num == 3){ //FEMU ZNS SSD init
        zns_get_info(path);
        //zns_get_zone_desc();
        //report_zones(0, NULL, NULL,NULL);
        //zns_get_zone_desc(REPORT_ALL, REPORT_ALL_STATE, 0, 0, true);
        //report_zones(0, NULL, NULL,NULL);
	    //zns_init("/dev/nvme0n1", zns_info_list);
     }else{// Dev_num error return
        cout << "This not a SSD number, please input correct SSD number!" << endl;
        cout << "Error for {DEV_NUM}" << endl;
        exit(0);
    }

    // Workload Argument
    if (dev_num == 1) {// M2 ZNS SSD
        workload_creator = new Workload_Creator(m2zns_info_list, zone_count, dev_num, zone_util);
    } else if (dev_num == 2) {// U3 ZNS SSD
        cout << "  WRKLD CREAET 2 " << endl;
        workload_creator = new Workload_Creator(zone_count, dev_num, zone_util);
    } else if (dev_num == 3){ //FEMU ZNS SSD
        cout << "  WRKLD CREAET 3 " << endl;
        workload_creator = new Workload_Creator(zone_count, zone_util);
    }
        
    // Init Argument
    Zone_count = zone_count;
    Zone_util = zone_util;
    Dev_num = dev_num;
    if (dev_num == 1) {// M2 ZNS SSD Spec
        Segment_count = M2_SEGMENT_COUNT_IN_ZONE;
        Block_count = M2_BLOCK_COUNT_IN_SEGMENT;
    } else if (dev_num == 2) {// U3 ZNS SSD Spec
        Segment_count = U3_SEGMENT_COUNT_IN_ZONE;
        Block_count = U3_BLOCK_COUNT_IN_SEGMENT;
    } else if(dev_num ==3 ){
        Segment_count = FEMU_SEGMENT_COUNT_IN_ZONE;
        Block_count = FEMU_BLOCK_COUNT_IN_SEGMENT;
    }
    total_segment_count = Zone_count * Segment_count;
    total_block_count = total_segment_count * Block_count;
    cout << "total_segment_count " << total_segment_count << " total_block_count " << total_block_count <<endl;
    // Init bitmap
    init_zone_bitmap();

    // Init ZNS SSD
    if(dev_num == 1) {
        m2_init_all_zones_reset();
        m2_init_zones_write(Zone_count);
    } else if (dev_num == 2) {
        u3_init_all_zones_reset();
        u3_init_zones_write(Zone_count);
    } else if(dev_num == 3){
        //init_all_zones_reset();
        //init_zones_write(Zone_count);
    }
        
    current_i_block_bitmap = 0;
}

//***************** Init function *****************//

void ZNS_Simulation::init_block_bitmap() {
    Block_bitmap = new SIM_Block[total_block_count];
    int valid_cnt = 0;

    cout<<endl<<"Init Block_bitmap ...ing"<<endl;
    
    // Set data valid by using utilization
    for(int i=0; i<Zone_count; i++) {
        //cout<<endl<<"Zone_bitmap " << &Zone_bitmap[i]<< "  "<< i <<endl;
        int start_block = Zone_bitmap[i].get_i_start_block();
        int end_block;
        
        if (Dev_num == 1) {
            end_block = start_block + (M2_SEGMENT_COUNT_IN_ZONE * M2_BLOCK_COUNT_IN_SEGMENT) -1;
        } else if (Dev_num == 2) {
            end_block = start_block + (U3_SEGMENT_COUNT_IN_ZONE * U3_BLOCK_COUNT_IN_SEGMENT) -1;
        }else if (Dev_num ==3 ){
            end_block = start_block + (FEMU_SEGMENT_COUNT_IN_ZONE * FEMU_BLOCK_COUNT_IN_SEGMENT) -1;
        }

        for(int j = start_block; j <= end_block; j++) {
            Block_bitmap[j].set_block_info(j);
            Block_bitmap[j].set_state(VALID_BLOCK);
        }
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
        if (Dev_num == 1) {
            for(int i=0; i<total_segment_count; i++) {
                Segment_bitmap[i].set_segment_info(i, i * M2_BLOCK_COUNT_IN_SEGMENT, COLD_SEGMENT);
            }    
        } else if (Dev_num == 2) {
            for(int i=0; i<total_segment_count; i++) {
                Segment_bitmap[i].set_segment_info(i, i * U3_BLOCK_COUNT_IN_SEGMENT, COLD_SEGMENT);
            }    
        } else if(Dev_num == 3){
            for(int i=0; i<total_segment_count; i++) {
                Segment_bitmap[i].set_segment_info(i, i * FEMU_BLOCK_COUNT_IN_SEGMENT, COLD_SEGMENT);
            }    
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
        if (Dev_num == 1) {
            for(int i=0; i<Zone_count; i++) {
                Zone_bitmap[i].set_zone_info(i, i * M2_SEGMENT_COUNT_IN_ZONE, i * M2_SEGMENT_COUNT_IN_ZONE * M2_BLOCK_COUNT_IN_SEGMENT);
            }
        } else if (Dev_num == 2) {
            for(int i=0; i<Zone_count; i++) {
                Zone_bitmap[i].set_zone_info(i, i * U3_SEGMENT_COUNT_IN_ZONE, i * U3_SEGMENT_COUNT_IN_ZONE * U3_BLOCK_COUNT_IN_SEGMENT);
            }
        }else if (Dev_num == 3) {
            for(int i=0; i<Zone_count; i++) {
                Zone_bitmap[i].set_zone_info(i, i * FEMU_SEGMENT_COUNT_IN_ZONE, i * FEMU_SEGMENT_COUNT_IN_ZONE * FEMU_BLOCK_COUNT_IN_SEGMENT);
            }
        }
        cout<<"Finish Init Zone_bitmap"<<endl;
        cout<< "#############################" <<endl;

        cout<<endl<<"Zone Info First & Last"<<endl;
        cout<< "#############################" <<endl;
        cout<< "[First Zone Info]" << endl;
        print_zone_info(0);
        cout<< "[Last Zone Info] Zone count " << Zone_count << endl;
        print_zone_info(Zone_count-1);
        cout<<endl;

        init_segment_bitmap();
}

//***************** End init function *****************//

//***************** M2 ZNS SSD GC function *****************//

int ZNS_Simulation::m2_read_valid_data(int i_block_offset) { 
    int index = i_block_offset;
    int read_count = 0;

    while(1) {
        if (Block_bitmap[index].get_state() == VALID_BLOCK) {
            read_count++;
            index++;
        } else break;
 
        if (read_count == (_128KB/_4KB)) break; // Max IO size 128KB (4KB * 32)
    }

    return read_count;
}

int ZNS_Simulation::m2_read_valid_data_lsm(int i_block_offset) { 
    int read_count = 0;

    for(int i = i_block_offset; i < i_block_offset+(_128KB/_4KB); i++) {
        if (Block_bitmap[i].get_state() == VALID_BLOCK) {
            read_count++;
        }
    }

    return read_count;
}

int ZNS_Simulation::read_valid_data(int i_block_offset) { 
    int index = i_block_offset;
    int read_count = 0;

    while(1) {
        if (Block_bitmap[index].get_state() == VALID_BLOCK) {
            read_count++;
            index++;
        } else break;
 
        if (read_count == (_128KB/_4KB)) break; // Max IO size 128KB (4KB * 32)
    }

    return read_count;
}


int ZNS_Simulation::read_valid_data_lsm(int i_block_offset) { 
    int read_count = 0;

    for(int i = i_block_offset; i < i_block_offset+32; i++) {
        if (Block_bitmap[i].get_state() == VALID_BLOCK) {
            read_count++;
        }
    }

    return read_count;
}

int ZNS_Simulation::basic_zgc() {
    cout<< "Start Basic ZGC" <<endl;
    struct timeval start, end;
    struct timeval time_old, time_new;
    double diffTime;
    double duration;

    int sel_zone = Zone_count + update_zone_count;
    int max_zone_num = 512;
    int i_zone, i_segment, i_block;

    int collection_invalid_count = 0;
    int i_bitmap_current;
    int read_valid_count;
    int io_result = 0;
    int valid_cnt;

    int i_zone_start_block, i_zone_end_block;

    Block_data * buffer_128KB_read = (Block_data *)malloc(_128KB);
    Block_data * buffer_128KB_write = (Block_data *)malloc(_128KB);

    memset(buffer_128KB_read, 0, _128KB);
    memset(buffer_128KB_write, 0, _128KB);

    int offset_write_zone = update_write_offset;
    int i_current_write_buffer = 0; //Max 48

    int buffer_write_temp_offset;
    int remain_read_offset;

    output = fopen("output.txt", "a");
    breakdown = fopen("breakdown.txt", "a");

    //start = time(NULL);
    gettimeofday(&start, NULL);
    for(i_zone = 0; i_zone < Zone_count; i_zone++) {
        i_zone_start_block = Zone_bitmap[i_zone].get_i_start_block();
        i_zone_end_block = Zone_bitmap[i_zone].get_i_start_block() + (FEMU_BLOCK_COUNT_IN_SEGMENT * FEMU_SEGMENT_COUNT_IN_ZONE);
        cout << "izone "<< i_zone <<" i_zone_start_block " << i_zone_start_block << endl;        
        cout << "izone "<< i_zone <<" i_zone_end_block " << i_zone_end_block << endl;

        if(Zone_bitmap[i_zone].get_valid_blocks(Zone_bitmap, Block_bitmap, i_zone) == 0) continue;

        for(i_bitmap_current = i_zone_start_block; i_bitmap_current < i_zone_end_block; ) {
            if (Block_bitmap[i_bitmap_current].get_state() == INVALID_BLOCK) {
                collection_invalid_count++;
                i_bitmap_current++;
                continue;
            }
        
            read_valid_count = read_valid_data(i_bitmap_current);
            
            if (read_valid_count == 0) {
                i_bitmap_current++;
                continue;
            } 
            gettimeofday(&time_old, NULL);
            io_result = zns_read(buffer_128KB_read, SECTOR_SIZE * read_valid_count, i_zone, (i_bitmap_current - FEMU_BLOCK_COUNT_IN_SEGMENT * FEMU_SEGMENT_COUNT_IN_ZONE * i_zone) * 8);
            gettimeofday(&time_new, NULL); read_time += (((time_new.tv_sec - time_old.tv_sec)*1000*1000) + (time_new.tv_usec - time_old.tv_usec));
            if(io_result == 0) {
                i_bitmap_current += read_valid_count;
                
            } else {
                cout << "goto Basic_GC_end" << endl;
                cout << "read_count : " << read_valid_count << endl;
                cout << "IO result : " << io_result << endl;
                goto Basic_GC_end;
            }

            // Buffer write (write before fill 192KB )
            if( (i_current_write_buffer + read_valid_count) < (_128KB/_4KB) ) { //check 128KB
                gettimeofday(&time_old, NULL);
                memcpy(&buffer_128KB_write[i_current_write_buffer], buffer_128KB_read,SECTOR_SIZE * read_valid_count);
                gettimeofday(&time_new, NULL); memcpy_time += (((time_new.tv_sec - time_old.tv_sec)*1000*1000) + (time_new.tv_usec - time_old.tv_usec));
                
                i_current_write_buffer += read_valid_count;
            } else {
                if((i_current_write_buffer + read_valid_count) == (_128KB/_4KB)) { //128KB write
                    
                    gettimeofday(&time_old, NULL);
                    memcpy(&buffer_128KB_write[i_current_write_buffer], buffer_128KB_read, SECTOR_SIZE * read_valid_count);
                    gettimeofday(&time_new, NULL); memcpy_time += (((time_new.tv_sec - time_old.tv_sec)*1000*1000) + (time_new.tv_usec - time_old.tv_usec));
                    
                    gettimeofday(&time_old, NULL);
                    io_result = zns_write(buffer_128KB_write, _128KB, sel_zone);
                    gettimeofday(&time_new, NULL); write_time += (((time_new.tv_sec - time_old.tv_sec)*1000*1000) + (time_new.tv_usec - time_old.tv_usec));

                    if(io_result == 0) {
                        offset_write_zone += (_128KB/_4KB);
                        i_current_write_buffer = 0;
                    } else {
                        cout << "write IO result : " << io_result << endl;
                        exit(0);
                    }
                } else {
                    remain_read_offset = i_current_write_buffer + read_valid_count - (_128KB/_4KB);
                    gettimeofday(&time_old, NULL);
                    memcpy(&buffer_128KB_write[i_current_write_buffer], buffer_128KB_read, SECTOR_SIZE * (read_valid_count-remain_read_offset));
                    gettimeofday(&time_new, NULL); memcpy_time += (((time_new.tv_sec - time_old.tv_sec)*1000*1000) + (time_new.tv_usec - time_old.tv_usec));
                    
                    
                    gettimeofday(&time_old, NULL);
                    io_result = zns_write(buffer_128KB_write, _128KB, sel_zone);
                    gettimeofday(&time_new, NULL); write_time += (((time_new.tv_sec - time_old.tv_sec)*1000*1000) + (time_new.tv_usec - time_old.tv_usec));
                    
                    if(io_result == 0) {
                        offset_write_zone += (_128KB/_4KB);
                        i_current_write_buffer = 0;
                    } else {
                        cout << "write IO result : " << io_result << endl;
                        exit(0);
                    }
                    gettimeofday(&time_old, NULL);
                    memcpy(&buffer_128KB_write[i_current_write_buffer], &buffer_128KB_read[read_valid_count-remain_read_offset],
                        _4KB * remain_read_offset);
                    gettimeofday(&time_new, NULL); memcpy_time += (((time_new.tv_sec - time_old.tv_sec)*1000*1000) + (time_new.tv_usec - time_old.tv_usec));
                    i_current_write_buffer += remain_read_offset;
                }
            }

            if(offset_write_zone  == (FEMU_BLOCK_COUNT_IN_SEGMENT * FEMU_SEGMENT_COUNT_IN_ZONE)) {
                zns_set_zone(sel_zone, MAN_FINISH);

                if (sel_zone >= max_zone_num) sel_zone = 0;
                sel_zone++;
                offset_write_zone = 0;
                cout << "Up to Max size of Zone, Move to next Zone!!" << endl;
                cout << "Write zone : " << sel_zone << endl;
            }

            if(collection_invalid_count >= (FEMU_BLOCK_COUNT_IN_SEGMENT * FEMU_SEGMENT_COUNT_IN_ZONE)) {
                if(i_current_write_buffer != 0) {
                    gettimeofday(&time_old, NULL);
                    io_result = zns_write(buffer_128KB_write, _128KB, sel_zone);
                    gettimeofday(&time_new, NULL); write_time += (((time_new.tv_sec - time_old.tv_sec)*1000*1000) + (time_new.tv_usec - time_old.tv_usec));
                }
            }
        } // end of Block in Zone
        // reinitialize collection_invalid_count
        collection_invalid_count = 0;
        
        gettimeofday(&time_old, NULL);
        zns_set_zone(i_zone, MAN_RESET);
        gettimeofday(&time_new, NULL); reset_time += (((time_new.tv_sec - time_old.tv_sec)*1000*1000) + (time_new.tv_usec - time_old.tv_usec));


        Zone_bitmap[i_zone].reset_valid_blocks();
    } // end of Zone
    gettimeofday(&end, NULL);
    duration = (end.tv_sec - start.tv_sec)*1000*1000;
    duration += ((end.tv_usec - start.tv_usec ));
    printf("%lf us\n", duration);

    cout << "ZNS SSD BASIC_ZGC time : " << duration << "usec" << endl;
    fprintf(output, "%lf us BASIC\n", duration);
    fprintf(breakdown, "%lf us BASIC  read %lf write %lf reset %lf memcpy %lf else %lf\n", duration, read_time, write_time, reset_time, memcpy_time, (duration - (read_time+ write_time+ reset_time+ memcpy_time)));
    fclose(output);
    fclose(breakdown);
Basic_GC_end: 
    printf("Basic ZGC End\n");
    
    return 0;
}

int ZNS_Simulation::lsm_zgc() {
    cout<< "Start LSM ZGC" <<endl;
    struct timeval start, end;
    struct timeval time_old, time_new;

    double duration;

    int sel_zone = Zone_count + update_zone_count;
    int max_zone_num = 512;
    int i_zone, i_segment, i_block;

    int collection_invalid_count = 0;
    int i_bitmap_current;
    int read_valid_count;
    int read_count;
    int io_result = 0;
    int valid_cnt = 0;

    int i_zone_start_block, i_zone_end_block;
    Block_data * buffer_128KB_read = (Block_data *)malloc(_128KB);
    Block_data * buffer_128KB_write = (Block_data *)malloc(_128KB);

    memset(buffer_128KB_read, 0, _128KB);
    memset(buffer_128KB_write, 0, _128KB);

    int offset_write_zone = update_write_offset;
    int i_current_write_buffer = 0; //Max 32
    int i_current_read_offset = 0;

    int buffer_write_temp_offset;
    int remain_read_offset;
    output = fopen("output.txt", "a");
    breakdown = fopen("breakdown.txt", "a");
    
    //start = time(NULL);
    gettimeofday(&start, NULL);
    for(i_zone = 0; i_zone < Zone_count; i_zone++) {

        i_zone_start_block = Zone_bitmap[i_zone].get_i_start_block();
        i_zone_end_block = Zone_bitmap[i_zone].get_i_start_block() + (FEMU_SEGMENT_COUNT_IN_ZONE * FEMU_BLOCK_COUNT_IN_SEGMENT) -1;

        if(Zone_bitmap[i_zone].get_valid_blocks(Zone_bitmap, Block_bitmap, i_zone) == 0) continue;

        for(i_bitmap_current = i_zone_start_block; i_bitmap_current <= i_zone_end_block; i_bitmap_current += (_128KB/_4KB)) {
            gettimeofday(&time_old, NULL);
            io_result = zns_read(buffer_128KB_read, _128KB, i_zone, (i_bitmap_current - FEMU_SEGMENT_COUNT_IN_ZONE * FEMU_BLOCK_COUNT_IN_SEGMENT*i_zone) * 8);
            gettimeofday(&time_new, NULL);
            read_time += (((time_new.tv_sec - time_old.tv_sec)*1000*1000) + (time_new.tv_usec - time_old.tv_usec));
            if(io_result != 0) {
                printf("LSM ZGC Read X\n");
                goto LSM_ZGC_end;
            }
            i_current_read_offset = 0;
            i_current_write_buffer = 0;
            for (int i = i_bitmap_current; i < i_bitmap_current+32;) {
                //cout<< " i "<< i << " Block_bitmap size " << sizeof(Block_bitmap) <<" Start LSM ZGC" <<endl;
                if (Block_bitmap[i].get_state() == VALID_BLOCK) {
                    //printf("2 LSM ZGC\n");
                    collection_invalid_count++;
                    i++;
                } else {
                    read_count = read_valid_data_lsm(i_bitmap_current);
                    //printf("2 LSM ZGC %d \n",read_count);
                    gettimeofday(&time_old, NULL);
                    memcpy(&buffer_128KB_write[i_current_write_buffer], &buffer_128KB_read[i_current_read_offset], _4KB*read_count);    //SIGSEGV
                    gettimeofday(&time_new, NULL); memcpy_time += (((time_new.tv_sec - time_old.tv_sec)*1000*1000) + (time_new.tv_usec - time_old.tv_usec));
                    //printf("2 LSM ZGC\n");
                    i += read_count;
                    i_current_read_offset += read_count;
                }
            }

            if((i_current_write_buffer + i_current_read_offset) < 32 ) { //check 128KB
                gettimeofday(&time_old, NULL);
                memcpy(&buffer_128KB_write[i_current_write_buffer], buffer_128KB_read, _4KB* i_current_read_offset);
                gettimeofday(&time_new, NULL);
                i_current_write_buffer += i_current_read_offset;
            } else {
                if( (i_current_write_buffer + i_current_read_offset) ==  32) { //128KB write
                    gettimeofday(&time_old, NULL);
                    memcpy(&buffer_128KB_write[i_current_write_buffer], buffer_128KB_read, _4KB* i_current_read_offset);
                    gettimeofday(&time_new, NULL); memcpy_time += (((time_new.tv_sec - time_old.tv_sec)*1000*1000) + (time_new.tv_usec - time_old.tv_usec));
                    io_result = zns_write(buffer_128KB_write, _128KB, sel_zone);
                    offset_write_zone += 32;
                    i_current_write_buffer = 0;
                } else {
                    remain_read_offset = i_current_write_buffer + i_current_read_offset -  32;
                    gettimeofday(&time_old, NULL);
                    memcpy(&buffer_128KB_write[i_current_write_buffer], buffer_128KB_read, _4KB * (i_current_read_offset-remain_read_offset));
                    gettimeofday(&time_new, NULL); memcpy_time += (((time_new.tv_sec - time_old.tv_sec)*1000*1000) + (time_new.tv_usec - time_old.tv_usec));
                    
                    gettimeofday(&time_old, NULL);
                    io_result = zns_write(buffer_128KB_write, _128KB, sel_zone);
                    gettimeofday(&time_new, NULL); write_time += (((time_new.tv_sec - time_old.tv_sec)*1000*1000) + (time_new.tv_usec - time_old.tv_usec));

                    offset_write_zone += 32;
                    i_current_write_buffer = 0;
                    
                    gettimeofday(&time_old, NULL);
                    memcpy(&buffer_128KB_write[i_current_write_buffer], &buffer_128KB_read[i_current_read_offset-remain_read_offset], _4KB * remain_read_offset);
                    gettimeofday(&time_new, NULL); memcpy_time += (((time_new.tv_sec - time_old.tv_sec)*1000*1000) + (time_new.tv_usec - time_old.tv_usec));
                    
                    i_current_write_buffer += remain_read_offset;
                }
            }
            if(offset_write_zone  == (FEMU_SEGMENT_COUNT_IN_ZONE * FEMU_BLOCK_COUNT_IN_SEGMENT) ) {  //Code smell wtf is 512*512 ?? 
                zns_set_zone(sel_zone, MAN_FINISH);
                sel_zone++;
                offset_write_zone = 0;
                cout << "Up to Max size of Zone, Move to next Zone!!" << endl;
                cout << "Write zone : " << sel_zone << endl;
            }
            if(collection_invalid_count >= (FEMU_SEGMENT_COUNT_IN_ZONE * FEMU_BLOCK_COUNT_IN_SEGMENT)) {//Code smell
                if(i_current_write_buffer != 0) {
                    gettimeofday(&time_old, NULL);
                    io_result = zns_write(buffer_128KB_write, _128KB, sel_zone);
                    gettimeofday(&time_new, NULL); write_time += (((time_new.tv_sec - time_old.tv_sec)*1000*1000) + (time_new.tv_usec - time_old.tv_usec));

                }
            }

        }
        // reinitialize collection_invalid_count
        collection_invalid_count = 0;
        gettimeofday(&time_old, NULL);
        zns_set_zone(i_zone, MAN_RESET);  
        gettimeofday(&time_new, NULL); reset_time += (((time_new.tv_sec - time_old.tv_sec)*1000*1000) + (time_new.tv_usec - time_old.tv_usec));

        Zone_bitmap[i_zone].reset_valid_blocks();
    } // end of zone

    gettimeofday(&end, NULL);
    duration = (end.tv_sec - start.tv_sec)*1000*1000;
    duration += ((end.tv_usec - start.tv_usec ));
    printf("%lf s\n", duration);

    cout << "FEMU ZNS SSD LSM_ZGC time : " << duration << "usec" << endl;
    fprintf(output, "%lf us LSM_ZGC\n", duration);
    fprintf(breakdown, "%lf us LSM_ZGC  read %lf write %lf reset %lf memcpy %lf else %lf\n", duration, read_time, write_time, reset_time, memcpy_time, (duration - (read_time+ write_time+ reset_time+ memcpy_time)));
    fclose(output);
    fclose(breakdown);

LSM_ZGC_end: 
    printf("LSM_ZGC End\n");
    
    return 0;
}

int ZNS_Simulation::m2_basic_zgc() {
    cout<< "Start Basic ZGC" <<endl;
    time_t start, end;
    double duration;

    int sel_zone = Zone_count + update_zone_count;
    int max_zone_num = 530;
    int i_zone, i_segment, i_block;

    int collection_invalid_count = 0;
    int i_bitmap_current;
    int read_valid_count;
    int io_result = 0;
    int valid_cnt;

    int i_zone_start_block, i_zone_end_block;

    Block_data * buffer_128KB_read = (Block_data *)malloc(512 * 8 * 32);
    Block_data * buffer_128KB_write = (Block_data *)malloc(512 * 8 * 32);

    memset(buffer_128KB_read, 0, 512 * 8 * 32);
    memset(buffer_128KB_write, 0, 512 * 8 * 32);

    int offset_write_zone = update_write_offset;
    int i_current_write_buffer = 0; //Max 32

    int buffer_write_temp_offset;
    int remain_read_offset;

    start = time(NULL);

    for(i_zone = 0; i_zone < Zone_count; i_zone++) {
        cout << i_zone << endl;
        i_zone_start_block = Zone_bitmap[i_zone].get_i_start_block();
        i_zone_end_block = Zone_bitmap[i_zone].get_i_start_block() + M2_SEGMENT_COUNT_IN_ZONE * M2_BLOCK_COUNT_IN_SEGMENT;

        if(Zone_bitmap[i_zone].m2_get_valid_blocks(Zone_bitmap, Block_bitmap, i_zone) == 0) continue;
        
        for(i_bitmap_current = i_zone_start_block; i_bitmap_current <= i_zone_end_block; ) {
            if (Block_bitmap[i_bitmap_current].get_state() == INVALID_BLOCK) {
                collection_invalid_count++;
                i_bitmap_current++;
                continue;
            }
        
            read_valid_count = m2_read_valid_data(i_bitmap_current);
            
            if (read_valid_count == 0) {
                i_bitmap_current++;
                continue;
            } 

            io_result = m2_zns_read(m2zns_info_list, buffer_128KB_read, 512 * 8 * read_valid_count, 
                i_zone, (i_bitmap_current - 512 * 512 * i_zone) * 8);
            
            if(io_result == 0) {
                i_bitmap_current += read_valid_count;
            } else {
                cout << "goto Basic_GC_end" << endl;
                cout << "read_count : " << read_valid_count << endl;
                cout << "IO result : " << io_result << endl;
                goto Basic_GC_end;
            }

            // Buffer write (write before fill 128KB )
            if( (i_current_write_buffer + read_valid_count) < 32 ) { //check 128KB
                memcpy(&buffer_128KB_write[i_current_write_buffer], buffer_128KB_read, 512 * 8 * read_valid_count);
                i_current_write_buffer += read_valid_count;
            } else {
                if((i_current_write_buffer + read_valid_count) == 32) { //128KB write
                    memcpy(&buffer_128KB_write[i_current_write_buffer], buffer_128KB_read, 512 * 8 * read_valid_count);
                    io_result = m2_zns_write(m2zns_info_list, buffer_128KB_write, 512 * 8 * 32, sel_zone, offset_write_zone * 8);
                    if(io_result == 0) {
                        offset_write_zone += 32;
                        i_current_write_buffer = 0;
                    } else {
                        cout << "write IO result : " << io_result << endl;
                        exit(0);
                    }
                } else {
                    remain_read_offset = i_current_write_buffer + read_valid_count - 32;
                    memcpy(&buffer_128KB_write[i_current_write_buffer], buffer_128KB_read, 512 * 8 * (read_valid_count-remain_read_offset));
                    io_result = m2_zns_write(m2zns_info_list, buffer_128KB_write, 512 * 8 * 32, sel_zone, offset_write_zone * 8);
                    if(io_result == 0) {
                        offset_write_zone += 32;
                        i_current_write_buffer = 0;
                    } else {
                        cout << "write IO result : " << io_result << endl;
                        exit(0);
                    }

                    memcpy(&buffer_128KB_write[i_current_write_buffer], &buffer_128KB_read[read_valid_count-remain_read_offset],
                        512 * 8 * remain_read_offset);
                    i_current_write_buffer += remain_read_offset;
                }
            }

            if(offset_write_zone  == (512 * 512)) {
                m2_zns_zone_finish(m2zns_info_list, sel_zone);

                if (sel_zone >= max_zone_num) sel_zone = 0;
                sel_zone++;
                offset_write_zone = 0;
                cout << "Up to Max size of Zone, Move to next Zone!!" << endl;
                cout << "Write zone : " << sel_zone << endl;
            }

            if(collection_invalid_count >= (512 * 512)) {
                if(i_current_write_buffer != 0) {
                    io_result = m2_zns_write(m2zns_info_list, buffer_128KB_write, 512 * 8 * i_current_write_buffer,
                        sel_zone, offset_write_zone * 8);
                }
            }
        } // end of Block in Zone
        // reinitialize collection_invalid_count
        collection_invalid_count = 0;
        
        m2_zns_zone_reset(m2zns_info_list, i_zone);
        Zone_bitmap[i_zone].reset_valid_blocks();
    } // end of Zone

    end = time(NULL);
    duration = (double)(end-start);
    cout << "M2 ZNS SSD BASIC_ZGC time : " << duration << "sec" << endl;

Basic_GC_end: 
    printf("Basic ZGC End\n");
    
    return 0;
}

int ZNS_Simulation::m2_lsm_zgc() {
    cout<< "Start LSM ZGC" <<endl;
    time_t start, end;
    double duration;

    int sel_zone = Zone_count + update_zone_count;
    int max_zone_num = 530;
    int i_zone, i_segment, i_block;

    int collection_invalid_count = 0;
    int i_bitmap_current;
    int read_valid_count;
    int read_count;
    int io_result = 0;
    int valid_cnt = 0;

    int i_zone_start_block, i_zone_end_block;

    Block_data * buffer_128KB_read = (Block_data *)malloc(512 * 8 * 32);
    Block_data * buffer_128KB_write = (Block_data *)malloc(512 * 8 * 32);

    memset(buffer_128KB_read, 0, 512 * 8 * 32);
    memset(buffer_128KB_write, 0, 512 * 8 * 32);

    int offset_write_zone = update_write_offset;
    int i_current_write_buffer = 0; //Max 32
    int i_current_read_offset = 0;

    int buffer_write_temp_offset;
    int remain_read_offset;
    
    start = time(NULL);

    for(i_zone = 0; i_zone < Zone_count; i_zone++) {

        i_zone_start_block = Zone_bitmap[i_zone].get_i_start_block();
        i_zone_end_block = Zone_bitmap[i_zone].get_i_start_block() + M2_SEGMENT_COUNT_IN_ZONE * M2_BLOCK_COUNT_IN_SEGMENT;

        if(Zone_bitmap[i_zone].m2_get_valid_blocks(Zone_bitmap, Block_bitmap, i_zone) == 0) continue;

        for(i_bitmap_current = i_zone_start_block; i_bitmap_current <= i_zone_end_block; i_bitmap_current += 32) {
            io_result = m2_zns_read(m2zns_info_list, buffer_128KB_read, 512 * 8 * 32, 
                i_zone, (i_bitmap_current - 512*512*i_zone) * 8);
            if(io_result != 0) {
                printf("LSM ZGC Read X\n");
                goto LSM_ZGC_end;
            }
            i_current_read_offset = 0;
            i_current_write_buffer = 0;
            for (int i = i_bitmap_current; i < i_bitmap_current+32;) {
                if (Block_bitmap[i].get_state() == VALID_BLOCK) {
                    collection_invalid_count++;
                    i++;
                } else {
                    read_count = m2_read_valid_data_lsm(i_bitmap_current);
                    memcpy(&buffer_128KB_write[i_current_write_buffer], &buffer_128KB_read[i_current_read_offset], 512 * 8 * read_count);
                    i += read_count;
                    i_current_read_offset += read_count;
                }
            }

            if((i_current_write_buffer + i_current_read_offset) < 32 ) { //check 128KB
                memcpy(&buffer_128KB_write[i_current_write_buffer], buffer_128KB_read, 512 * 8 * i_current_read_offset);
                i_current_write_buffer += i_current_read_offset;
            } else {
                if((i_current_write_buffer + i_current_read_offset) == 32) { //128KB write
                    memcpy(&buffer_128KB_write[i_current_write_buffer], buffer_128KB_read, 512 * 8 * i_current_read_offset);
                    io_result = m2_zns_write(m2zns_info_list, buffer_128KB_write, 512 * 8 * 32, sel_zone, offset_write_zone * 8);
                    offset_write_zone += 32;
                    i_current_write_buffer = 0;
                } else {
                    remain_read_offset = i_current_write_buffer + i_current_read_offset - 32;
                    memcpy(&buffer_128KB_write[i_current_write_buffer], buffer_128KB_read, 512 * 8 * (i_current_read_offset-remain_read_offset));
                    io_result = m2_zns_write(m2zns_info_list, buffer_128KB_write, 512 * 8 * 32, sel_zone, offset_write_zone * 8);
                    offset_write_zone += 32;
                    i_current_write_buffer = 0;
                    memcpy(&buffer_128KB_write[i_current_write_buffer], &buffer_128KB_read[i_current_read_offset-remain_read_offset], 512 * 8 * remain_read_offset);
                    i_current_write_buffer += remain_read_offset;
                }
            }
            if(offset_write_zone  == (512*512) ) {
                m2_zns_zone_finish(m2zns_info_list, sel_zone);
                sel_zone++;
                offset_write_zone = 0;
                cout << "Up to Max size of Zone, Move to next Zone!!" << endl;
                cout << "Write zone : " << sel_zone << endl;
            }
            if(collection_invalid_count >= (512*512)) {
                if(i_current_write_buffer != 0) {
                    io_result = m2_zns_write(m2zns_info_list, buffer_128KB_write, 512 * 8 * i_current_write_buffer, sel_zone, offset_write_zone * 8);
                }
            }
        }
        // reinitialize collection_invalid_count
        collection_invalid_count = 0;
        
        m2_zns_zone_reset(m2zns_info_list, i_zone);
        Zone_bitmap[i_zone].reset_valid_blocks();
    } // end of zone

    end = time(NULL);
    duration = (double)(end - start);
    cout << "M2 ZNS SSD LSM_ZGC time : " << duration << "sec" << endl;

LSM_ZGC_end: 
    printf("LSM_ZGC End\n");
    
    return 0;
}

//***************** End M2 ZNS SSD GC function *****************//

//***************** U3 ZNS SSD GC function *****************//

int ZNS_Simulation::u3_read_valid_data(int i_block_offset) { 
    int index = i_block_offset;
    int read_count = 0;

    while(1) {
        if (Block_bitmap[index].get_state() == VALID_BLOCK) {
            read_count++;
            index++;
        } else break;
 
        if (read_count == 48) break; // Max IO size 192KB (4KB * 48)
    }

    return read_count;
}

int ZNS_Simulation::u3_read_valid_data_lsm(int i_block_offset) { 
    int read_count = 0;

    for(int i = i_block_offset; i < i_block_offset+48; i++) {
        if (Block_bitmap[i].get_state() == VALID_BLOCK) {
            read_count++;
        }
    }

    return read_count;
}

int ZNS_Simulation::u3_basic_zgc() {
    cout<< "Start Basic ZGC" <<endl;
    time_t start, end;
    double duration;

    int sel_zone = Zone_count + update_zone_count;
    int max_zone_num = 29172;
    int i_zone, i_segment, i_block;

    int collection_invalid_count = 0;
    int i_bitmap_current;
    int read_valid_count;
    int io_result = 0;
    int valid_cnt;

    int i_zone_start_block, i_zone_end_block;

    Block_data * buffer_192KB_read = (Block_data *)malloc(512 * 8 * 48);
    Block_data * buffer_192KB_write = (Block_data *)malloc(512 * 8 * 48);

    memset(buffer_192KB_read, 0, 512 * 8 * 48);
    memset(buffer_192KB_write, 0, 512 * 8 * 48);

    int offset_write_zone = update_write_offset;
    int i_current_write_buffer = 0; //Max 48

    int buffer_write_temp_offset;
    int remain_read_offset;

    start = time(NULL);

    for(i_zone = 0; i_zone < Zone_count; i_zone++) {
        i_zone_start_block = Zone_bitmap[i_zone].get_i_start_block();
        i_zone_end_block = Zone_bitmap[i_zone].get_i_start_block() + U3_SEGMENT_COUNT_IN_ZONE * U3_BLOCK_COUNT_IN_SEGMENT;
        
        if(Zone_bitmap[i_zone].u3_get_valid_blocks(Zone_bitmap, Block_bitmap, i_zone) == 0) continue;

        for(i_bitmap_current = i_zone_start_block; i_bitmap_current <= i_zone_end_block; ) {
            if (Block_bitmap[i_bitmap_current].get_state() == INVALID_BLOCK) {
                collection_invalid_count++;
                i_bitmap_current++;
                continue;
            }
        
            read_valid_count = u3_read_valid_data(i_bitmap_current);
            
            if (read_valid_count == 0) {
                i_bitmap_current++;
                continue;
            } 

            io_result = u3_zns_read(buffer_192KB_read, 512 * 8 * read_valid_count, i_zone, (i_bitmap_current - 512 * 512 * i_zone) * 8);
            
            if(io_result == 0) {
                i_bitmap_current += read_valid_count;
            } else {
                cout << "goto Basic_GC_end" << endl;
                cout << "read_count : " << read_valid_count << endl;
                cout << "IO result : " << io_result << endl;
                goto Basic_GC_end;
            }

            // Buffer write (write before fill 192KB )
            if( (i_current_write_buffer + read_valid_count) < 48 ) { //check 192KB
                memcpy(&buffer_192KB_write[i_current_write_buffer], buffer_192KB_read, 512 * 8 * read_valid_count);
                i_current_write_buffer += read_valid_count;
            } else {
                if((i_current_write_buffer + read_valid_count) == 48) { //192KB write
                    memcpy(&buffer_192KB_write[i_current_write_buffer], buffer_192KB_read, 512 * 8 * read_valid_count);
                    io_result = u3_zns_write(buffer_192KB_write, _192KB, sel_zone);
                    if(io_result == 0) {
                        offset_write_zone += 48;
                        i_current_write_buffer = 0;
                    } else {
                        cout << "write IO result : " << io_result << endl;
                        exit(0);
                    }
                } else {
                    remain_read_offset = i_current_write_buffer + read_valid_count - 48;
                    memcpy(&buffer_192KB_write[i_current_write_buffer], buffer_192KB_read, 512 * 8 * (read_valid_count-remain_read_offset));
                    io_result = u3_zns_write(buffer_192KB_write, _192KB, sel_zone);
                    if(io_result == 0) {
                        offset_write_zone += 48;
                        i_current_write_buffer = 0;
                    } else {
                        cout << "write IO result : " << io_result << endl;
                        exit(0);
                    }
                    memcpy(&buffer_192KB_write[i_current_write_buffer], &buffer_192KB_read[read_valid_count-remain_read_offset],
                        512 * 8 * remain_read_offset);
                    i_current_write_buffer += remain_read_offset;
                }
            }

            if(offset_write_zone  == (512 * 36)) {
                u3_zns_set_zone(sel_zone, MAN_FINISH);

                if (sel_zone >= max_zone_num) sel_zone = 0;
                sel_zone++;
                offset_write_zone = 0;
                cout << "Up to Max size of Zone, Move to next Zone!!" << endl;
                cout << "Write zone : " << sel_zone << endl;
            }

            if(collection_invalid_count >= (512 * 36)) {
                if(i_current_write_buffer != 0) {
                    io_result = u3_zns_write(buffer_192KB_write, _192KB, sel_zone);
                }
            }
        } // end of Block in Zone
        // reinitialize collection_invalid_count
        collection_invalid_count = 0;
        
        u3_zns_set_zone(i_zone, MAN_RESET);
        Zone_bitmap[i_zone].reset_valid_blocks();
    } // end of Zone

    end = time(NULL);
    duration = (double)(end - start);
    cout << "U3 ZNS SSD BASIC_ZGC time : " << duration << "sec" << endl;

Basic_GC_end: 
    printf("Basic ZGC End\n");
    
    return 0;
}

int ZNS_Simulation::u3_lsm_zgc() {
    cout<< "Start LSM ZGC" <<endl;
    time_t start, end;
    double duration;

    int sel_zone = Zone_count + update_zone_count;
    int max_zone_num = 29172;
    int i_zone, i_segment, i_block;

    int collection_invalid_count = 0;
    int i_bitmap_current;
    int read_valid_count;
    int read_count;
    int io_result = 0;
    int valid_cnt = 0;

    int i_zone_start_block, i_zone_end_block;

    Block_data * buffer_192KB_read = (Block_data *)malloc(512 * 8 * 48);
    Block_data * buffer_192KB_write = (Block_data *)malloc(512 * 8 * 48);

    memset(buffer_192KB_read, 0, 512 * 8 * 48);
    memset(buffer_192KB_write, 0, 512 * 8 * 48);

    int offset_write_zone = update_write_offset;
    int i_current_write_buffer = 0; //Max 48
    int i_current_read_offset = 0;

    int buffer_write_temp_offset;
    int remain_read_offset;

    start = time(NULL);

    for(i_zone = 0; i_zone < Zone_count; i_zone++) {        
        i_zone_start_block = Zone_bitmap[i_zone].get_i_start_block();
        i_zone_end_block = Zone_bitmap[i_zone].get_i_start_block() + U3_SEGMENT_COUNT_IN_ZONE * U3_BLOCK_COUNT_IN_SEGMENT;
        
        if(Zone_bitmap[i_zone].u3_get_valid_blocks(Zone_bitmap, Block_bitmap, i_zone) == 0) continue;

        for(i_bitmap_current = i_zone_start_block; i_bitmap_current <= i_zone_end_block; i_bitmap_current += 48) {
            io_result = u3_zns_read(buffer_192KB_read, _192KB, i_zone, (i_bitmap_current - 512*512*i_zone) * 8);
            if(io_result != 0) {
                printf("LSM ZGC Read X\n");
                goto LSM_ZGC_end;
            }
            i_current_read_offset = 0;
            i_current_write_buffer = 0;
            for (int i = i_bitmap_current; i < i_bitmap_current+48;) {
                if (Block_bitmap[i].get_state() == VALID_BLOCK) {
                    collection_invalid_count++;
                    i++;
                } else {
                    read_count = u3_read_valid_data_lsm(i_bitmap_current);
                    memcpy(&buffer_192KB_write[i_current_write_buffer], &buffer_192KB_read[i_current_read_offset], 512 * 8 * read_count);
                    i += read_count;
                    i_current_read_offset += read_count;
                }
            }
            if( (i_current_write_buffer + i_current_read_offset) < 48 ) { //check 192KB
                memcpy(&buffer_192KB_write[i_current_write_buffer], buffer_192KB_read, 512 * 8 * i_current_read_offset);
                i_current_write_buffer += i_current_read_offset;
            } else {
                if((i_current_write_buffer + i_current_read_offset) == 48) { //192KB write
                    memcpy(&buffer_192KB_write[i_current_write_buffer], buffer_192KB_read, 512 * 8 * i_current_read_offset);
                    io_result = u3_zns_write(buffer_192KB_write, _192KB, sel_zone);
                    offset_write_zone += 48;
                    i_current_write_buffer = 0;
                } else {
                    remain_read_offset = i_current_write_buffer + i_current_read_offset - 48;
                    memcpy(&buffer_192KB_write[i_current_write_buffer], buffer_192KB_read, 512 * 8 * remain_read_offset);
                    io_result = u3_zns_write(buffer_192KB_write, _192KB, sel_zone);
                    offset_write_zone += 48;
                    i_current_write_buffer = 0;
                    memcpy(&buffer_192KB_write[i_current_write_buffer], &buffer_192KB_read[i_current_read_offset-remain_read_offset], 512 * 8 * remain_read_offset);
                    i_current_write_buffer += remain_read_offset;
                }
            }
            if( offset_write_zone  == (512*36) ) {
                u3_zns_set_zone(sel_zone, MAN_FINISH);
                sel_zone++;
                offset_write_zone = 0;
                cout << "Up to Max size of Zone, Move to next Zone!!" << endl;
                cout << "Write zone : " << sel_zone << endl;
            }
            if(collection_invalid_count >= (512*36)) {
                if(i_current_write_buffer != 0) {
                    io_result = u3_zns_write(buffer_192KB_write, _192KB, sel_zone);
                }
            }
        }
        // reinitialize collection_invalid_count
        collection_invalid_count = 0;

        u3_zns_set_zone(i_zone, MAN_RESET);        
        Zone_bitmap[i_zone].reset_valid_blocks();
    } // end of zone

    end = time(NULL);
    duration = (double)(end - start);
    cout << "U3 ZNS SSD LSM_ZGC time : " << duration << "sec" << endl;

LSM_ZGC_end: 
    printf("LSM_ZGC End\n");
    
    return 0;
}

//***************** End U3 ZNS SSD GC function *****************//

//***************** ZNS SSD Zone write/reset function *****************//

int ZNS_Simulation::init_zones_write(int numofzones) {
    int i_zone, i_segment, i_block, i_bitmap;
    int write_num;
    int write_result;
    int zonesize_divide_by_128K = (FEMU_BLOCK_COUNT_IN_SEGMENT*FEMU_SEGMENT_COUNT_IN_ZONE) / (_128KB / _4KB);
    void * dummy_data = new char[_128KB];
    memset(dummy_data, 66, _128KB);

    cout << endl << "Init ZNS SSD Write ...ing" << endl;
    for(i_zone=0; i_zone<numofzones; i_zone++) {
        for (write_num = 0; write_num < zonesize_divide_by_128K; write_num++) {
            write_result = zns_write(dummy_data, _128KB, i_zone);
        }
        zns_set_zone(i_zone, MAN_FINISH);
    }

    cout << "Finish Init ZNS SSD Write" << endl;

    free(dummy_data);
    return 0;
}


void ZNS_Simulation::init_all_zones_reset() {
    int i_zone = 0;
    cout << "Reset ZNS SSD ...ing" << endl;
    for (i_zone = 0; i_zone < FEMU_ZONE_MAX_COUNT; i_zone++) {
        zns_set_zone(i_zone, MAN_RESET);
    }
    cout << "Finish Reset ZNS SSD" << endl;
}
int ZNS_Simulation::m2_init_zones_write(int numofzones) {
    int i_zone, i_segment, i_block, i_bitmap;
    int offset;
    int write_result;

    void * dummy_data = new char[ZNS_BLOCK_SIZE * 32];
    memset(dummy_data, 66, ZNS_BLOCK_SIZE * 32);

    cout << endl << "Init ZNS SSD Write ...ing" << endl;
    for(i_zone=0; i_zone<numofzones; i_zone++) {
        for (offset = 0; offset < 8192; offset++) {
            write_result = m2_zns_write(m2zns_info_list, dummy_data, ZNS_BLOCK_SIZE * 32, i_zone, offset * 256);
        }
        m2_zns_zone_finish(m2zns_info_list, i_zone);
    }

    cout << "Finish Init ZNS SSD Write" << endl;

    free(dummy_data);
    return 0;
}

void ZNS_Simulation::m2_init_zone_reset(int numofzones) {
    int i_zone;

    cout<< "Zone Init Reset" <<endl;
    for(i_zone=0; i_zone<numofzones; i_zone++) {
        m2_zns_zone_reset(m2zns_info_list, i_zone);
    }
}

void ZNS_Simulation::m2_init_all_zones_reset() {
    int i_zone = 0;
    cout << "Reset ZNS SSD ...ing" << endl;
    for (i_zone = 0; i_zone < M2_ZONE_MAX_COUNT; i_zone++) {
        m2_zns_zone_reset(m2zns_info_list, i_zone);
    }
    cout << "Finish Reset ZNS SSD" << endl;
}

int ZNS_Simulation::u3_init_zones_write(int numofzones) {
    int i_zone, i_segment, i_block, i_bitmap;
    int write_num;
    int write_result;

    void * dummy_data = new char[ZNS_BLOCK_SIZE * 48];
    memset(dummy_data, 66, ZNS_BLOCK_SIZE * 48);

    cout << endl << "Init ZNS SSD Write ...ing" << endl;
    for(i_zone=0; i_zone<numofzones; i_zone++) {
        for (write_num = 0; write_num < 384; write_num++) {
            write_result = u3_zns_write(dummy_data, _192KB, i_zone);
        }
        u3_zns_set_zone(i_zone, MAN_FINISH);
    }

    cout << "Finish Init ZNS SSD Write" << endl;

    free(dummy_data);
    return 0;
}

void ZNS_Simulation::u3_init_zone_reset(int numofzones) {
    int i_zone;

    cout<< "Zone Init Reset" <<endl;
    for(i_zone=0; i_zone<numofzones; i_zone++) {
        u3_zns_set_zone(i_zone, MAN_RESET);
    }
}

void ZNS_Simulation::u3_init_all_zones_reset() {
    int i_zone = 0;
    cout << "Reset ZNS SSD ...ing" << endl;
    for (i_zone = 0; i_zone < U3_ZONE_MAX_COUNT; i_zone++) {
        u3_zns_set_zone(i_zone, MAN_RESET);
    }
    cout << "Finish Reset ZNS SSD" << endl;
}

//***************** End ZNS SSD Zone write/reset function *****************//

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
    int end_i_block;
    int list_cnt = 0; //int valid_cnt = 0;

    if(Dev_num == 1) 
        end_i_block = start_i_block + M2_BLOCK_COUNT_IN_SEGMENT;
    else if (Dev_num == 2)
        end_i_block = start_i_block + U3_BLOCK_COUNT_IN_SEGMENT;

    cout<< "Segment " << i_segment << " block_bitmap" <<endl;
    for(int i_block=start_i_block; i_block<end_i_block; i_block++) {
        list_cnt++;
        cout << Block_bitmap[i_block].get_state() << " ";
        if((list_cnt % 20) == 0)
            cout<<endl;
    }
    cout<<endl;
}

void ZNS_Simulation::print_zone_block_bitmap(int i_zone) {
    int start_i_block = Zone_bitmap[i_zone].get_i_start_block();
    int end_i_block;
    int list_cnt = 0;

    if(Dev_num == 1) 
        end_i_block = start_i_block + M2_BLOCK_COUNT_IN_SEGMENT * M2_SEGMENT_COUNT_IN_ZONE;
    else if (Dev_num == 2)
        end_i_block = start_i_block + U3_BLOCK_COUNT_IN_SEGMENT * U3_SEGMENT_COUNT_IN_ZONE;
    else if (Dev_num == 3 )
        end_i_block = start_i_block + FEMU_BLOCK_COUNT_IN_SEGMENT * FEMU_SEGMENT_COUNT_IN_ZONE;


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
    int end_i_segment;
    int t_status;
    int list_cnt = 0;

    if(Dev_num == 1) 
        end_i_segment = start_i_segment + M2_SEGMENT_COUNT_IN_ZONE;
    else if (Dev_num == 2)
        end_i_segment = start_i_segment + U3_SEGMENT_COUNT_IN_ZONE;

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

//************* Workload Creator function **************//

int * ZNS_Simulation::request_sequential_workload() {
    update_bitmap = workload_creator->create_sequential_workload(Zone_bitmap, Segment_bitmap, Block_bitmap);

    return update_bitmap;
}

int * ZNS_Simulation::request_random_workload() {
    update_bitmap = workload_creator->create_random_workload(Zone_bitmap, Segment_bitmap, Block_bitmap);

    return update_bitmap;
}

void ZNS_Simulation::request_update_workload() {
    pair<int,int> update_count;
    if (Dev_num == 1)
        update_count = workload_creator->m2_update_block_in_memory(Zone_bitmap, Segment_bitmap, Block_bitmap, update_bitmap);
    else if (Dev_num == 2)
        update_count = workload_creator->u3_update_block_in_memory(Zone_bitmap, Segment_bitmap, Block_bitmap, update_bitmap);
    else if (Dev_num == 3)
        update_count = workload_creator->update_block_in_memory(Zone_bitmap, Segment_bitmap, Block_bitmap, update_bitmap);

    this->update_write_offset = update_count.first;
    this->update_zone_count = update_count.second;
}

//************* End Workload Creator function **************//