#include <iostream> 
#include <chrono>
#include <bitset>
#include <fstream>
#include <math.h>
#include "qspi_memory.h"


#define MUX_BASE 0x41210008
#define MUX_SET 0x104
#define MUX_DESET 0x100
#define MUX_WIDTH 32 

#define QSPI_CR_WIDTH 32
#define QSPI_STD_WIDTH 8 
#define QSPI_BASE 0xA0030000
#define QSPI_CONFIG_R 0x60
#define QSPI_STATUS_R 0x64
#define QSPI_DTR 0x68
#define QSPI_DRR 0x6C
#define QSPI_SSR 0x70

//Flash instruction codes
#define FL_READ_ID 0x90
#define FL_WRITE_ENABLE 0x06
#define FL_WRITE_REG 0x01
#define FL_READ_STATUS 0x05
#define FL_READ_CONFIG 0x35
#define FL_READ_BAR 0x16
#define FL_READ_QUAD_OUT 0x6C
#define FL_READ_QUAD_IO 0xEB

#define RESET_FIFO_MSTR_CONFIG_ENABLE 0x000001E6

#define ENABLE_MASTER_TRAN 0x00000086
#define DISABLE_MASTER_TRAN 0x00000186
#define CHIP_SELECT 0x00
#define CHIP_DESELECT 0x01
#define DUMMY_DATA 0xDD

#define FL_MEM_START 0x00000000
#define FL_MEM_SIZE 28734812
#define FIFO_DEPTH 128
#define PREAMBLE_SIZE 9

const std::string BIN_EXT = ".bin";

qspi_memory qspi_controller(QSPI_BASE);
std::ofstream binfile; 

/*  Check whether the TX buffer in the QSPI controller is empty
*   Returns True if the Status Reg Bit 2 is High.
*/
bool tx_empty(){
    //int status_reg = qspi_controller.read_mem(QSPI_STATUS_R, QSPI_STD_WIDTH);
    return ((qspi_controller.read_mem(QSPI_STATUS_R, QSPI_STD_WIDTH) == 36) ? true : false);
}

/*  Reads the Device ID of the Spansion Flash Memory
*   Prints the two byte ID.
*/
void read_spansion_id(){

    std::cout << "Reading Spansion ID via MMAP" << std::endl;

    qspi_controller.write_mem(QSPI_CONFIG_R, RESET_FIFO_MSTR_CONFIG_ENABLE, QSPI_CR_WIDTH);
    qspi_controller.write_mem(QSPI_DTR, FL_READ_ID, QSPI_STD_WIDTH);

    for(int i = 0; i < 3; i++){
        qspi_controller.write_mem(QSPI_DTR, 0x00, QSPI_STD_WIDTH);
    }

    for(int i = 0; i < 2; i++){
        qspi_controller.write_mem(QSPI_DTR, DUMMY_DATA, QSPI_STD_WIDTH);
    }

    qspi_controller.write_mem(QSPI_SSR, CHIP_SELECT, QSPI_STD_WIDTH);
    qspi_controller.write_mem(QSPI_CONFIG_R, ENABLE_MASTER_TRAN, QSPI_CR_WIDTH);

    bool tx_state = tx_empty();
    while (tx_state == false){
        tx_state = tx_empty();
    }

    qspi_controller.write_mem(QSPI_SSR, CHIP_DESELECT, QSPI_STD_WIDTH);
    qspi_controller.write_mem(QSPI_CONFIG_R, DISABLE_MASTER_TRAN, QSPI_CR_WIDTH);

    //push the address bytes out
    for(int i = 0; i < 4; i++){
        qspi_controller.read_mem(QSPI_DRR, QSPI_CR_WIDTH);
    }

    //read the real ID
    for(int i = 0; i < 2; i++){
        printf("0x%X\n", qspi_controller.read_mem(QSPI_DRR, QSPI_CR_WIDTH));
    }

}

/*  Loop function to perform @num_bytes of memory reads from @address in blocks of @increment.
*   Writes the byte data to the binary file, in binary format
*   Returns the next address to read from 
*   Used to ensure bytes are read on the FIFO boundaries.
*/
uint32_t read_loop(uint32_t& address, unsigned long& num_bytes, unsigned long& increment){

    uint32_t fbyte_address = address;

    for(int i = 0; i < num_bytes; i+= increment){

        qspi_controller.write_mem(QSPI_CONFIG_R, RESET_FIFO_MSTR_CONFIG_ENABLE, QSPI_CR_WIDTH);
        qspi_controller.write_mem(QSPI_DTR, FL_READ_QUAD_OUT, QSPI_STD_WIDTH);

        // bit shift the four byte address into 4 bytes
        uint8_t msb = (fbyte_address & 0xFF000000) >> 24;
        uint8_t mid1 = (fbyte_address & 0x00FF0000) >> 16;
        uint8_t mid2 = (fbyte_address & 0x0000FF00) >> 8;
        uint8_t lsb = (fbyte_address & 0x000000FF);

        qspi_controller.write_mem(QSPI_DTR, msb, QSPI_STD_WIDTH);
        qspi_controller.write_mem(QSPI_DTR, mid1, QSPI_STD_WIDTH);
        qspi_controller.write_mem(QSPI_DTR, mid2, QSPI_STD_WIDTH);
        qspi_controller.write_mem(QSPI_DTR, lsb, QSPI_STD_WIDTH);

        // fill with dummy data
        for(int j =0; j < increment + PREAMBLE_SIZE; j++){
            qspi_controller.write_mem(QSPI_DTR, DUMMY_DATA, QSPI_STD_WIDTH);
        }
        
        qspi_controller.write_mem(QSPI_SSR, CHIP_SELECT, QSPI_STD_WIDTH);
        qspi_controller.write_mem(QSPI_CONFIG_R, ENABLE_MASTER_TRAN, QSPI_CR_WIDTH);
        
        // check the tx buffer is empty - i.e we have allowed enough time before trying to read
        bool tx_state = tx_empty();
        while (tx_state == false){
            tx_state = tx_empty();
        }  

        qspi_controller.write_mem(QSPI_SSR, CHIP_DESELECT, QSPI_STD_WIDTH);
        qspi_controller.write_mem(QSPI_CONFIG_R, DISABLE_MASTER_TRAN, QSPI_CR_WIDTH);
        
        // read out but don't print the preamble junk
        for(int k = 0; k < PREAMBLE_SIZE; k++){
            qspi_controller.read_mem(QSPI_DRR, QSPI_CR_WIDTH);
        }

        // read and write bytes to binary file 
        for(int d =0; d < increment; d++){
            uint8_t byte = qspi_controller.read_mem(QSPI_DRR, QSPI_CR_WIDTH);
            binfile.write((char*)&byte, sizeof byte);
        }

        fbyte_address += increment;
    }
    return fbyte_address;
}
/*  Sets up the read memory loop function, calculates how many bytes to read across the FIFO depth 
*   Calls read_loop with the FIFO aligned num_bytes and then with the overflow num_bytes
*   Calculates the time the read has taken in milliseconds.
*/
void read_spansion_memory(uint32_t& mem_address, unsigned long& num_bytes, std::string& filename){

    std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
    
    unsigned long increment = FIFO_DEPTH; //max FIFO = 128
    float float_num_loops = num_bytes/FIFO_DEPTH; // calculate how many times 128 goes into num_bytes
    int whole_num_loops = floor(float_num_loops); // round down to nearest integer
    unsigned long int FIFO_aligned_num_bytes = whole_num_loops * FIFO_DEPTH; // calculate how many bytes can be read evenly on the FIFO boundary
    unsigned long int overflow_bytes = num_bytes - FIFO_aligned_num_bytes; // find the overflow between even bytes and requested bytes. 

    //binfile.open("full_qspi_driver_bitfile.bin", std::ios::out | std::ios::binary);

    binfile.open(filename, std::ios::out | std::ios::binary);

    if(!binfile.is_open()){
        std::cout << "Failed to Open Bin File.. Quiting" << std::endl;
        exit(1);
    }
    
    uint32_t next_addr = read_loop(mem_address, FIFO_aligned_num_bytes, increment);
    read_loop(next_addr, overflow_bytes, overflow_bytes);

    binfile.close();
    std::chrono::high_resolution_clock::time_point finish = std::chrono::high_resolution_clock::now();
    auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(finish - start).count();
    std::cout << milliseconds << " ms" << std::endl;
    
}

int main(int argc, char* argv[]){

    
    if(argc < 4){
        std::cerr << "Usage : " << std::endl << "<filename> (binary file to write FLASH memory to)" << std::endl;
        std::cerr << "<start_address> (memory address to begin reading from, in hex)" << std::endl; 
        std::cerr << "<number_bytes> (number of bytes to read, in decimal)" << std::endl; 
        return 1;
    }
    else{
    
        char* end;
        std::string filename = argv[1] + BIN_EXT;
        uint32_t start_address = strtoul(argv[2], &end, 16);
        unsigned long num_bytes = strtoul(argv[3], &end, 10);

        printf("\nReading %d bytes starting from address 0x%X ..\n", num_bytes, start_address);
        printf("Printing to binary file called %s\n\n", filename.c_str()); 


        qspi_controller.init_mmap();
        //read_spansion_id();
        //read_spansion_memory(0x01b66ff0, 1388); // last 1387 bytes.
        //read_spansion_memory(FL_MEM_START, FL_MEM_SIZE, filename);
        
        
        read_spansion_memory(start_address, num_bytes, filename);
        qspi_controller.unmap();

        return 0;
    }
}


