#ifndef MEMORY_MAPPED_DEVICE_H_
#define MEMORY_MAPPED_DEVICE_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <stdint.h>
#include <ctype.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <stdint.h>

#include "mem_exception.h"

 
#define MAP_SIZE 4096UL // minimum memory space - size of address space? 
#define MAP_MASK (MAP_SIZE - 1)
#define MAX_FIFO 128


class memory_mapped_device{

    public:

        int fd; //this is file to open using open(dev/mem)
        void *map_base, *virt_addr, *full_addr; 
        unsigned long read_result, writeval;
        off_t target; //address
        memory_mapped_device();
        memory_mapped_device(uint32_t base);
        ~memory_mapped_device(){};

        unsigned long read_mem(uint32_t offset, uint8_t width);
        unsigned long write_mem(uint32_t offset, unsigned long the_data, uint8_t width);
        void map();
        void unmap();
};

#endif
