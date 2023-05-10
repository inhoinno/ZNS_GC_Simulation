/* 2020. 09. 22 - Date of initial creation
 * Creater : Gunhee Choi
 * Modifier : Hojin Shin 
 * This file is the ZNS_GC_Simulation main
*/

#include "zns_simulation.h"
#include <string>

//Argument
#define ARGUMENT_COUNT 7      //All Arugment + filename
#define DEV_NAME 1
#define DEV_NUM 2
#define ZONE_COUNT 3
#define SETTING_DEV_UTILIZATION 4
#define GC_NUMBER 5
#define WORKLOAD_TYPE 6

using namespace std;

int main(int argc, char * argv[]) {
    ZNS_Simulation * zns_simulation;

    if(argc != ARGUMENT_COUNT) {
        cout<< "Please, set argument" <<endl;
        cout<< "---> sudo ./Simulation {DEV_NAME} {DEV_NUM(1. M2, 2. U3)} {ZONE_COUNT} {SETTING_ZONE_UTILIZATION(%)} {GC_NUMBER(BASIC_ZGC/LSM_ZGC)} {WORKLOAD_TYPE(SEQ/RAND)} " <<endl;
        cout<< "---> example : sudo ./Simulation /dev/nvme0n1 3 512 60 BASIC_ZGC RAND" <<endl;
        exit(0);
    }
    
    //Print Setting Argument List
    cout<< endl << "Start Simulation!!" <<endl;
    cout<< "This Simulation is done in a single thread." <<endl;
    cout<< "Hot/Cold Standard = 70% Invalid Block(Hot)/ 30% Invaild Block(Cold)" <<endl << endl;
    cout<< "------------------------------------------------------" <<endl;
    cout<< "Device Name : " << argv[DEV_NAME] <<endl;
    cout<< "------------------------------------------------------" <<endl<<endl;

    //1. Init ZNS SSD
    cout<< "1. Start Init ZNS SSD Simulation" <<endl;
    cout<< "------------------------------------------------------" <<endl;
        zns_simulation = new ZNS_Simulation(argv[DEV_NAME], atoi(argv[ZONE_COUNT]), atoi(argv[SETTING_DEV_UTILIZATION]), atoi(argv[DEV_NUM]));
    cout<< "------------------------------------------------------" <<endl<<endl;
    
    //2. Create Workload
    cout<< "2. Start Create Workload" <<endl;
    cout<< "------------------------------------------------------" <<endl;
        if (strcmp(argv[WORKLOAD_TYPE],"SEQ") == 0) {
            cout << "Do Sequential" << endl;
            zns_simulation->request_sequential_workload();
        } else if (strcmp(argv[WORKLOAD_TYPE],"RAND") == 0) {
            cout << "Do Random" << endl;
            zns_simulation->request_random_workload();
        }
    cout<< "------------------------------------------------------" <<endl<<endl;

    //3. Start Workload Simualtion in memory.
    cout<< "3. Start Workload Simualtion in memory" <<endl;
    cout<< "------------------------------------------------------" <<endl;
        cout << "Do Update" << endl;
        zns_simulation->request_update_workload();
    cout<< "------------------------------------------------------" <<endl<<endl;

    //4. Start ZGC Simulation in ZNS SSD
    cout<< "4. Start GC Simulation in ZNS SSD" <<endl;
    cout<< "------------------------------------------------------" <<endl;
        if (strcmp(argv[GC_NUMBER],"BASIC_ZGC") == 0) {
            if(atoi(argv[DEV_NUM]) == 1) {
                zns_simulation->m2_basic_zgc();
            } else if (atoi(argv[DEV_NUM]) == 2 ) {
                zns_simulation->u3_basic_zgc();
            } else if(atoi(argv[DEV_NUM]) == 3 ){
                zns_simulation->basic_zgc();
            }
        } else if (strcmp(argv[GC_NUMBER],"LSM_ZGC") == 0) {
            if(atoi(argv[DEV_NUM]) == 1) {
                zns_simulation->m2_lsm_zgc();
            } else if (atoi(argv[DEV_NUM]) == 2 ) {
                zns_simulation->u3_lsm_zgc();
            } else if(atoi(argv[DEV_NUM]) == 3 ){
                zns_simulation->lsm_zgc();
            }
        }
    cout<< "------------------------------------------------------" <<endl<<endl;












    //Test
    //zns_simulation->print_zone_block_bitmap(0);
    //zns_simulation->print_segment_block_bitmap(0);
    //zns_simulation->print_zone_segment_bitmap(0);
    //zns_simulation->setting_random_bitmap();

    //Init Simulation
    //zns_simulation = new ZNS_Simulation("/dev/nvme0n1", workload_creator.getWorkloadlist());


    //zns_simulation->init_block_bitmap();
    //zns_simulation->init_zone_reset(1);
    //zns_simulation->init_zones_write(1);
    //zns_simulation->print_bitmap(1);
    
    //zns_simulation->request_write(100, 16);
    //zns_simulation->request_write(100, 12);

    /*
    workload_list = workload_creator.getWorkloadlist();
    workload_list->front()->print_workload_data();
    */

    return 0;
}
