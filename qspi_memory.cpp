#include "qspi_memory.h"

//using namespace Femii;

/*
*   Constructor for mem_reader.
*   @Param base : base address for the device.
*   @param offset : offset address for the device.
*   @param width : the DataWidth to read/write in.
*/
qspi_memory::qspi_memory(uint32_t base) {//, uint32_t offset, DataWidth width){

    this->target = base; // + offset;
    //this->width = width;
}

/*
*   Initialises the memory map using the mem_reader variables.
*   Throws Fem2Exception if it failed to open /dev/mem
*   Throws Fem2Exception if the mmap failed.
*/
void qspi_memory::init_mmap(){

    if((fd = open("/dev/mem", O_RDWR | O_SYNC)) == -1)
    {
        FATAL; // failed to open the dev/mem
    }

    printf("/dev/mem opened.\n");
    fflush(stdout);
 
    /* Map one page */
    this->map_base = mmap(0, 
                    MAP_SIZE, //size of the address space
                    PROT_READ | PROT_WRITE, // read and write
                    MAP_SHARED, //share the mapping
                    this->fd, //dev/mem
                    this->target & ~MAP_MASK // target address entered AND NOT MAP_MASK, always clips to the page boundary (page size) (i.e removes 1 from 0x80000001) for mmap to work
    );

    if(this->map_base == (void *) -1){
        FATAL; // failed to map
    } 
    printf("Memory mapped at address %p.\n", map_base);
    fflush(stdout);
    this->virt_addr = this->map_base + (this->target & MAP_MASK); // base address clipped to page boundary i.e 0x80000000 plus .. (0x80000001 AND MAP_MASK) == 0x1. Getting the target address again.

}

/*
*   Reads memory from the address (base + offset) in the DataWidth provided.
*   Throws Fem2Exception if the datawidth is not supported.
*   Returns the value read from the address.
*/

//unsigned long read_mem(uint32_t offset, DataWidth width);
        
unsigned long qspi_memory::read_mem(uint32_t offset, uint8_t width){    

    this->full_addr = this->virt_addr + offset;
    
    switch(width) 
    {
        case 8:
            //this->read_result = *((unsigned char *) this->virt_addr); // cast virt_address as an unsigned char pointer, then dereference i.e get the value.
            this->read_result = *((unsigned char *) this->full_addr);
            break;
        case 16:
            this->read_result = *((unsigned short *) this->full_addr);
            //this->read_result = *((unsigned short *) this->virt_addr);
            break;
        case 32:
            this->read_result = *((unsigned short *) this->full_addr);
            //this->read_result = *((unsigned long *) this->virt_addr);
            break;
        default:
            throw MemException("Illegal Data Type");
    }

    //printf("Value at address 0x%X (%p): 0x%X\n", (this->target + offset), this->full_addr, this->read_result);
    //return this->read_result;
    //printf("\n");
    fflush(stdout);
    //fsync(this->fd); //maybe?
    return this->read_result;
}

/*
*   Writes memory to the address (base + offset) in the DataWidth provided.
*   Throws Fem2Exception if the datawidth is not supported.
*   Returns the value read back from the address.
*/

//unsigned long write_mem(uint32_t offset, unsigned long the_data, DataWidth width);

unsigned long qspi_memory::write_mem(uint32_t offset, unsigned long the_data, uint8_t width){

        this->full_addr = this->virt_addr + offset;
        this->writeval = the_data;

        switch(width) 
        {
            case 8:

                /*
                *((unsigned char *) this->virt_addr) = this->writeval;
                this->read_result = *((unsigned char *) this->virt_addr);
                */

                *((unsigned char *) this->full_addr) = this->writeval;
                this->read_result = *((unsigned char *) this->full_addr);
                break;
            case 16:
                *((unsigned short *) this->full_addr) = this->writeval;
                this->read_result = *((unsigned short *) this->full_addr);
                break;
            case 32:
                *((unsigned long *) this->full_addr) = this->writeval;
                this->read_result = *((unsigned long *) this->full_addr);
                break;
            default:                          
                //fprintf(stderr, "Illegal data type .\n");
                throw MemException("Illegal Data Type");
        }
        //printf("Written 0x%X; readback 0x%X\n", this->writeval, this->read_result);
        //printf("Written 0x%X\n", this->writeval); 
    
        //sleep(1);
        //return this->read_result;
        //printf("\n");
        //fflush(stdout);

        //fsync(this->fd);
    
    //return this->read_result;
}

/*
*   Un maps the memory map..
*   Throws Fem2Exception if it failed to un map.
*/
void qspi_memory::unmap(){

    if(munmap((void *)this->map_base, MAP_SIZE) == -1) 
    {
        FATAL;
    }
    close(this->fd);
}

