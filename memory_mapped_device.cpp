#include "memory_mapped_device.h"

/*
*   Constructor for qspi_memory.
*   @Param base : base address for the device.
*/
memory_mapped_device::memory_mapped_device(uint32_t base) {//, uint32_t offset, DataWidth width){

    this->target = base;
}

/*
*   Initialises the memory map using the qspi_memory variables.
*   Throws memException if it failed to open /dev/mem
*   Throws memException if the mmap failed.
*/
void memory_mapped_device::map(){

    if((fd = open("/dev/mem", O_RDWR | O_SYNC)) == -1)
    {
        throw mem_exception("Dev mem failed to open."); // failed to open the dev/mem
    }

    /* Map one page */
    this->map_base = mmap(0, 
                    MAP_SIZE, //size of the address space
                    PROT_READ | PROT_WRITE, // read and write
                    MAP_SHARED, //share the mapping
                    this->fd, //dev/mem
                    this->target & ~MAP_MASK // target address entered AND NOT MAP_MASK, always clips to the page boundary (page size) (i.e removes 1 from 0x80000001) for mmap to work
    );

    if(this->map_base == (void *) -1){
       throw mem_exception("Memory map failed to map the addressed area.");  // failed to map
    } 

    this->virt_addr = this->map_base + (this->target & MAP_MASK); // base address clipped to page boundary i.e 0x80000000 plus .. (0x80000001 AND MAP_MASK) == 0x1. Getting the target address again.

}

/*
*   Reads memory from the address (base + offset) in the width provided.
*   Throws MemException if the datawidth is not supported.
*   Returns the value read from the address.
*/    
unsigned long memory_mapped_device::read_mem(uint32_t offset, uint8_t width){    

    this->full_addr = this->virt_addr + offset;
    
    switch(width) 
    {
        case 8:
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
            throw mem_exception("Illegal Data Width");
    }
    fflush(stdout);
    return this->read_result;
}

/*
*   Writes memory to the address (base + offset) in the width provided.
*   Throws MemException if the datawidth is not supported.
*   Returns the value read back from the address.
*/
unsigned long memory_mapped_device::write_mem(uint32_t offset, unsigned long the_data, uint8_t width){

        this->full_addr = this->virt_addr + offset;
        this->writeval = the_data;

        switch(width) 
        {
            case 8:
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
                throw mem_exception("Illegal Data Width");
        }

}

/*
*   Un maps the memory map..
*   Throws MemException if it failed to un map.
*/
void memory_mapped_device::unmap(){

    if(munmap((void *)this->map_base, MAP_SIZE) == -1) 
    {
        throw mem_exception("Memory Map Failed to Un-Map."); ;
    }
    close(this->fd);
}

