/*
*   memory_mapped_device.cpp
*   @Author Sophie Kirkham STFC, 2018
*   Implementation for the memory_mapped_device class
*   Memory maps an area of memory, providing read/write access to the area
*/

#include "memory_mapped_device.h"

/*
*   Constructor for qspi_memory objects.
*   @Param base : base address for the device.
*/
memory_mapped_device::memory_mapped_device(uint32_t base) {

    this->target = base;
}

/*
*   Initialises the memory map using the qspi_memory class variables.
*   Memory map is initialised with read,write and shared mapping.
*   @throws mem_exception: if dev mem fails to open.
*   @throws mem_exception: if the mmap fails to map the area.
*/
void memory_mapped_device::map(){

    // Open dev/mem setting the file descriptor (fd) with the output.
    if((this->file_descriptor = open("/dev/mem", O_RDWR | O_SYNC)) == -1)
    {
        throw mem_exception("Dev mem failed to open.");
    }

    // Map the address space, setting map_base.
    this->map_base = mmap(0, 
                    MAP_SIZE, // Size of the address space
                    PROT_READ | PROT_WRITE, // Read and write access
                    MAP_SHARED, // Share the mapping
                    this->file_descriptor, // File descriptor for dev/mem
                    this->target & ~MAP_MASK // Target is clipped to the page boundary. 
    );

    // Check to see if the mapped area has been mapped.
    if(this->map_base == (void *) -1){
       throw mem_exception("Memory map failed to map the addressed area.");
    } 

    // Set the virtual address to be the correct target address.
    this->virt_addr = this->map_base + (this->target & MAP_MASK); 

}

/*
*   Reads memory from the address (base + offset) in the width provided.
*   @param offset : offset address to read from
*   @param width : data width to read in (8, 16, 32 bits)
*   @throws mem_exception: if the datawidth is not supported.
*   @returns read_result : the value read from the mapped address space.
*/    
unsigned long memory_mapped_device::read_mem(uint32_t offset, uint8_t width){    

    // get the full address to read from by adding the virtual base + offset
    this->full_addr = this->virt_addr + offset;
    switch(width) 
    {
        case 8:
            this->read_result = *((unsigned char *) this->full_addr);
            break;
        case 16:
            this->read_result = *((unsigned short *) this->full_addr);
            break;
        case 32:
            this->read_result = *((unsigned short *) this->full_addr);
            break;
        default:
            throw mem_exception("Illegal Data Width");
    }
    fflush(stdout);
    return this->read_result;
}

/*
*   Writes memory to the address (base + offset) in the width provided.
*   @param offset : the offset address to write to.
*   @param the_data : the data value to write
*   @param width : the data width to write in (8, 16, 32 bits)
*   @throws mem_exception : if the datawidth is not supported.
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
*   Un maps the memory map
*   @throws mem_exception : if it failed to un map the virtual address space.
*/
void memory_mapped_device::unmap(){

    if(munmap((void *)this->map_base, MAP_SIZE) == -1) 
    {
        throw mem_exception("Memory Map Failed to Un-Map."); ;
    }
    close(this->file_descriptor);
}

