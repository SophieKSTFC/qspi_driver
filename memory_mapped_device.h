/*
*   memory_mapped_device.h
*   @Author Sophie Kirkham STFC, 2018
*   Header file for the memory_mapped_device class
*   Class to memory map address space and access physical resources.
*/

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

#define MAP_SIZE 4096UL 
#define MAP_MASK (MAP_SIZE - 1)
#define MAX_FIFO 128

/*
*   Class to memory map address space to access hardware resources
*   Reads and Writes to memory in 8, 16 and 32 bits.
*/
class memory_mapped_device{

    public:

        int file_descriptor; 
        void *map_base, *virt_addr, *full_addr; 
        unsigned long read_result, writeval;
        off_t target; 
        memory_mapped_device();
        memory_mapped_device(uint32_t base);
        ~memory_mapped_device(){};
        unsigned long read_mem(uint32_t offset, uint8_t width);
        unsigned long write_mem(uint32_t offset, unsigned long the_data, uint8_t width);
        void map();
        void unmap();
};

#endif
