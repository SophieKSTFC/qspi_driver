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

qspi_memory qspi_controller(QSPI_BASE);

std::ofstream binfile; 



bool tx_empty(){

    int status_reg = qspi_controller.read_mem(QSPI_STATUS_R, QSPI_STD_WIDTH);
    //printf("0x%X\n", status_reg);
    //fflush(stdout);
    return ((status_reg == 36) ? true : false);

}


/*
void to_binary(const uint8_t& byte){

   std::cout <<  std::bitset<8>(byte) << std::endl ;

}
*/

void read_spansion_id(){

    /*
    binfile.open("qspi_driver_bitfile.bin", std::ios::out | std::ios::binary);

    if(!binfile.is_open()){
        std::cout << "Failed to Open Bin File.. Quiting" << std::endl;
        exit(1);
    }
    */

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
        //fflush(stdout);
    }

    //read the real ID
    for(int i = 0; i < 2; i++){
        printf("0x%X\n", qspi_controller.read_mem(QSPI_DRR, QSPI_CR_WIDTH));
        //uint8_t byte = qspi_controller.read_mem(QSPI_DRR, QSPI_CR_WIDTH);
        //binfile.write((char*)&byte, sizeof byte);

    }

    //binfile.close();

}

uint32_t read_loop(uint32_t& address, unsigned long& num_bytes, unsigned long& increment){

    uint32_t fbyte_address = address;
    int address_preamble = 9;

    for(int i = 0; i < num_bytes; i+= increment){

        //std::cout << num_bytes << std::endl;
        //std::cout << fbyte_address << std::endl;

        qspi_controller.write_mem(QSPI_CONFIG_R, RESET_FIFO_MSTR_CONFIG_ENABLE, QSPI_CR_WIDTH);
        qspi_controller.write_mem(QSPI_DTR, FL_READ_QUAD_OUT, QSPI_STD_WIDTH);

        uint8_t msb = (fbyte_address & 0xFF000000) >> 24;
        uint8_t mid1 = (fbyte_address & 0x00FF0000) >> 16;
        uint8_t mid2 = (fbyte_address & 0x0000FF00) >> 8;
        uint8_t lsb = (fbyte_address & 0x000000FF);

        qspi_controller.write_mem(QSPI_DTR, msb, QSPI_STD_WIDTH);
        qspi_controller.write_mem(QSPI_DTR, mid1, QSPI_STD_WIDTH);
        qspi_controller.write_mem(QSPI_DTR, mid2, QSPI_STD_WIDTH);
        qspi_controller.write_mem(QSPI_DTR, lsb, QSPI_STD_WIDTH);

        for(int j =0; j < increment + address_preamble; j++){
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
        
        // read out but don't print the preamble junk
        for(int k = 0; k < address_preamble; k++){
            qspi_controller.read_mem(QSPI_DRR, QSPI_CR_WIDTH);
        }

        for(int d =0; d < increment; d++){

            //printf("0x%X\n", qspi_controller.read_mem(QSPI_DRR, QSPI_CR_WIDTH));
            uint8_t byte = qspi_controller.read_mem(QSPI_DRR, QSPI_CR_WIDTH);
            binfile.write((char*)&byte, sizeof byte);
            //printf("0x%X\n", byte);
        }

        fbyte_address += increment;
    }

    return fbyte_address;
}

void read_spansion_memory(uint32_t mem_address, unsigned long num_bytes){

    std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
    
    unsigned long int increment = 128; //max FIFO = 128
    int address_preamble = 9;

    float float_num_loops = num_bytes/increment; // calculate how many times 128 goes into num_bytes
    int whole_num_loops = floor(float_num_loops); // round down to nearest integer
    unsigned long int FIFO_aligned_num_bytes = whole_num_loops * increment; // calculate how many bytes can be read evenly on the FIFO boundary
    unsigned long int overflow_bytes = num_bytes - FIFO_aligned_num_bytes; // find the overflow between even bytes and requested bytes. 


    uint32_t fbyte_address = mem_address;

    binfile.open("TEST_qspi_driver_bitfile.bin", std::ios::out | std::ios::binary);

    if(!binfile.is_open()){
        std::cout << "Failed to Open Bin File.. Quiting" << std::endl;
        exit(1);
    }
    
    uint32_t next_addr = read_loop(mem_address, FIFO_aligned_num_bytes, increment);
    //std::cout << "next address ="
    read_loop(next_addr, overflow_bytes, overflow_bytes);

    /*
    for(int i = 0; i < num_bytes; i+= increment){
        
        qspi_controller.write_mem(QSPI_CONFIG_R, RESET_FIFO_MSTR_CONFIG_ENABLE, QSPI_CR_WIDTH);
        qspi_controller.write_mem(QSPI_DTR, FL_READ_QUAD_OUT, QSPI_STD_WIDTH);

        uint8_t msb = (fbyte_address & 0xFF000000) >> 24;
        uint8_t mid1 = (fbyte_address & 0x00FF0000) >> 16;
        uint8_t mid2 = (fbyte_address & 0x0000FF00) >> 8;
        uint8_t lsb = (fbyte_address & 0x000000FF);

        qspi_controller.write_mem(QSPI_DTR, msb, QSPI_STD_WIDTH);
        qspi_controller.write_mem(QSPI_DTR, mid1, QSPI_STD_WIDTH);
        qspi_controller.write_mem(QSPI_DTR, mid2, QSPI_STD_WIDTH);
        qspi_controller.write_mem(QSPI_DTR, lsb, QSPI_STD_WIDTH);

        for(int j =0; j < increment + address_preamble; j++){
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
        

        // read out but don't print the preamble junk
        for(int k = 0; k < address_preamble; k++){
           qspi_controller.read_mem(QSPI_DRR, QSPI_CR_WIDTH);
        }

        for(int d =0; d < increment; d++){

            //printf("0x%X\n", qspi_controller.read_mem(QSPI_DRR, QSPI_CR_WIDTH));
            uint8_t byte = qspi_controller.read_mem(QSPI_DRR, QSPI_CR_WIDTH);
            binfile.write((char*)&byte, sizeof byte);
        }

        fbyte_address += increment;

    }
*/
    binfile.close();
    std::chrono::high_resolution_clock::time_point finish = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(finish - start).count();
    auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(finish - start).count();

    std::cout << duration << " microseconds" << std::endl;
    std::cout << milliseconds << " milliseconds" << std::endl;
    
    
}

int main(){


    qspi_controller.init_mmap();
    //read_spansion_id();
    //read_spansion_memory(0x00000000, 28734816);

    //read_spansion_memory(0x00000000, 512);

    //read_spansion_memory(0x00000000, 1024);
     int bytes = 160;
    //read_spansion_memory(0x00000000, 486160);

    read_spansion_memory(0x01b66ff0, 1388); // last 1387 bytes.
    qspi_controller.unmap();

    return 0;
}


