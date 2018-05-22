#ifndef QSPI_MEMORY_H_
#define QSPI_MEMORY_H_

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

#include "MemException.cpp"

#define FATAL do { fprintf(stderr, "Error at line %d, file %s (%d) [%s]\n", \
  __LINE__, __FILE__, errno, strerror(errno)); throw MemException("Fatal Error");} while(0)
 
#define MAP_SIZE 4096UL // minimum memory space - size of address space? 
#define MAP_MASK (MAP_SIZE - 1)
#define MAX_FIFO 128
//using namespace Femii;

class qspi_memory{

    public:
        
        enum DataWidth{

          WIDTH_UNSUPPORTED = -1, 
          WIDTH_BYTE = 0,         //could do 2^8  -- 8bits
          WIDTH_WORD = 1,         //16 bits
          WIDTH_LONG = 2,         //32 bits
        
        };

        int size = getpagesize();
        int fd; //this is file to open using open(dev/mem)
        void *map_base, *virt_addr, *full_addr; 
        unsigned long read_result, writeval;
        off_t target; //address
        DataWidth width = WIDTH_UNSUPPORTED; // width

        qspi_memory(uint32_t base);
        ~qspi_memory(){};

        unsigned long read_mem(uint32_t offset, uint8_t width);
        unsigned long write_mem(uint32_t offset, unsigned long the_data, uint8_t width);
        void init_mmap();
        void unmap();
};

#endif
