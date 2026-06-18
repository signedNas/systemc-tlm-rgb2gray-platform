#include <systemc>
#include <iostream>
#include "memory_map.h"
#include "ram.h"
#include "persistent_storage.h"
#include "accelerator.h"
#include "bus.h"
#include "cpu.h"


int sc_main(int argc, char* argv[])
{
    const char* input_file;
    const char* output_file;

    if (argc <3)
        return -1;
        

    output_file = argv[2];
    input_file = argv[1];

    CPU               cpu    ("cpu");
    RAM               ram    ("ram");
    Accelerator       accel  ("accel");
    Bus               bus    ("bus");
    PersistentStorage storage("storage", input_file, output_file);

    cpu.init_socket   .bind(bus.targ_socket);
    bus.ram_socket    .bind(ram.cpu_port);
    bus.accel_socket  .bind(accel.cfg_socket);
    bus.storage_socket.bind(storage.targ_socket);
    accel.dma_socket  .bind(ram.dma_port);

    sc_core::sc_start();


    return 0;
}