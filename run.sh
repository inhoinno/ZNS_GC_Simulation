#!/bin/bash

make; make clean; make;

#70
echo "70 util " >> output.txt
for run in {1..5}
do
    sudo ./Simulation /dev/nvme0n1 3 1 70 BASIC_ZGC RAND
    sudo blkzone reset /dev/nvme0n1;
    sleep 3;
done

#80
echo "80 util " >> output.txt
for run in {1..5}
do
    sudo ./Simulation /dev/nvme0n1 3 1 80 BASIC_ZGC RAND
    sudo blkzone reset /dev/nvme0n1;
    sleep 3;
done


#90
echo "90 util " >> output.txt
for run in {1..5}
do
    sudo ./Simulation /dev/nvme0n1 3 1 90 BASIC_ZGC RAND
    sudo blkzone reset /dev/nvme0n1;
    sleep 3;
done

mv output.txt output_1024_BASIC


#70
echo "70 util " >> output.txt
for run in {1..5}
do
    sudo ./Simulation /dev/nvme0n1 3 1 70 LSM_ZGC RAND
    sudo blkzone reset /dev/nvme0n1;
    sleep 3;
done

#80
echo "80 util " >> output.txt
for run in {1..5}
do
    sudo ./Simulation /dev/nvme0n1 3 1 80 LSM_ZGC RAND
    sudo blkzone reset /dev/nvme0n1;
    sleep 3;
done


#90
echo "90 util " >> output.txt
for run in {1..5}
do
    sudo ./Simulation /dev/nvme0n1 3 1 90 LSM_ZGC RAND
    sudo blkzone reset /dev/nvme0n1;
    sleep 3;
done

mv output.txt output_1024_LSM
