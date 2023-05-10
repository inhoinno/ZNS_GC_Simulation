CC=g++
CFLAGS=-g -pg -Wall -Wno-write-strings 
OBJS=main.o controller.o m2controller.o u3controller.o workload_creator.o zns_simulation.o zns_simulation_datastructure.o 
TARGET=Simulation
INC=-Iutil
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(INC) -g -o $@ $(OBJS)

main.o : m2controller.h u3controller.h workload_creator.h zns_simulation_datastructure.h main.cc
controller.o: controller.cc controller.h m2controller.h m2controller.cc u3controller.h u3controller.cc 
workload_creator.o: workload_creator.h workload_creator.cc
zns_simulation.o: zns_simulation.h zns_simulation.cc
simulation_datastructure.o: zns_simulation_datastructure.h zns_simulation_datastructure.cc

all: $(TARGET)

clean:
	rm -f *.o
	rm -f $(TARGET)

clena:
	rm -f *.o
	rm -f $(TARGET)


