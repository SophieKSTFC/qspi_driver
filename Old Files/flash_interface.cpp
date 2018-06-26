/*
*   Flash Memory Interface Cmd Line Tool
*/

#include <iostream> 
#include <chrono>
#include <string.h>
#include <bitset>
#include <fstream>
#include <math.h>
#include <vector>
#include "qspi_memory.h"


#define MUX_BASE 0x41210000
#define MUX_OFFSET 0x08
#define MUX_SET_FL1 0x104
#define MUX_SET_FL2 0x105
#define MUX_SET_FL3 0x106
#define MUX_SET_FL4 0x107
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
#define FL_QUAD_PP 0x34
#define FL_BULK_ERASE 0x60

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
#define DEFAULT_FLASH 1
#define SIXTY_FOUR_MB 64000000
#define MAX_FLASH_ADDRESS (SIXTY_FOUR_MB - 16)


const std::string BIN_EXT = ".bin";

qspi_memory qspi_controller(QSPI_BASE);
qspi_memory multiplexer(MUX_BASE);

std::ofstream out_file; // file to read bytes too from memory
std::ifstream in_file;// file to write bytes to memory from

uint8_t crc_table[256];
uint8_t polynominal = 0x1D;

/*
*   Populates crc_table by calculating all CRC values for each byte value (0-255)
*/
void calc_CRC8_table(){

    for(int div = 0; div < 256; div++){
        uint8_t current_byte = (uint8_t)div;
        for(uint8_t bit = 0; bit < 8; bit ++){
            if((current_byte & 0x80) !=0){
                current_byte <<= 1; // the MSB was a 1, shift it out
                current_byte ^= polynominal; // XOR with the polynominal
            }
            else{
                current_byte <<=1; // else it was a 0, just shift it.
            }
        }
        crc_table[div] = current_byte;
    }
}

/*
*   Computes the 8 Bit Cyclic Redundancy Checksum for a given ifstream file
*   @param file : reference to an ifstream file object to use to calculate the checksum 
*   Returns the checksum as a uint8_t (unsigned char)
*/
uint8_t compute_crc(std::ifstream& file){

    std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
    uint8_t crc = 0;
    if(!file){
        std::cout << "File Failed to Load\n" << std::endl;
    }
    std::vector<char> buffer((std::istreambuf_iterator<char>(file)), (std::istreambuf_iterator<char>()));
    for(auto i = buffer.begin(); i != buffer.end(); i++){
        uint8_t byte = (uint8_t) (*i ^ crc); //XOR the byte 
        crc = crc_table[byte];
    }
    std::chrono::high_resolution_clock::time_point finish = std::chrono::high_resolution_clock::now();
    std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(finish - start).count() << " ms to complete CRC" << std::endl;   
    return crc;
}

/*  Check whether the TX buffer in the QSPI controller is empty
*   Returns True if the Status Reg Bit 2 is High.
*/
bool tx_empty(){
    std::bitset<8> qspi_status(qspi_controller.read_mem(QSPI_STATUS_R, QSPI_STD_WIDTH));
    return ((qspi_status[2] == 1) ? true : false);
}

/*  Check whether the RX buffer in the QSPI controller is empty
*   Returns True if the Status Reg Bit 0 is High.
*/
bool rx_empty(){
    std::bitset<8> qspi_status(qspi_controller.read_mem(QSPI_STATUS_R, QSPI_STD_WIDTH));
    return ((qspi_status[0] == 1) ? true : false);
}

/*  Reads the status register of the flash memory device
*   Returns the byte value of the status register.
*/
uint8_t read_flash_status_reg(){

    qspi_controller.write_mem(QSPI_CONFIG_R, RESET_FIFO_MSTR_CONFIG_ENABLE, QSPI_CR_WIDTH);
    qspi_controller.write_mem(QSPI_DTR, FL_READ_STATUS, QSPI_STD_WIDTH);
    qspi_controller.write_mem(QSPI_DTR, DUMMY_DATA, QSPI_STD_WIDTH);
    qspi_controller.write_mem(QSPI_SSR, CHIP_SELECT, QSPI_STD_WIDTH);
    qspi_controller.write_mem(QSPI_CONFIG_R, ENABLE_MASTER_TRAN, QSPI_CR_WIDTH);
    
    bool tx_state = tx_empty();
    while (tx_state == false){
        tx_state = tx_empty();
    }
    qspi_controller.write_mem(QSPI_SSR, CHIP_DESELECT, QSPI_STD_WIDTH);
    qspi_controller.write_mem(QSPI_CONFIG_R, DISABLE_MASTER_TRAN, QSPI_CR_WIDTH);
    qspi_controller.read_mem(QSPI_DRR, QSPI_STD_WIDTH);
    uint8_t sr = qspi_controller.read_mem(QSPI_DRR, QSPI_STD_WIDTH);
    
    return sr;
}

/*  Reads the config register of the flash memory device
*   Returns the byte value of the config register.
*/
uint8_t read_flash_config_reg(){

    qspi_controller.write_mem(QSPI_CONFIG_R, RESET_FIFO_MSTR_CONFIG_ENABLE, QSPI_CR_WIDTH);
    qspi_controller.write_mem(QSPI_DTR, FL_READ_CONFIG, QSPI_STD_WIDTH);
    qspi_controller.write_mem(QSPI_DTR, DUMMY_DATA, QSPI_STD_WIDTH);
    qspi_controller.write_mem(QSPI_SSR, CHIP_SELECT, QSPI_STD_WIDTH);
    qspi_controller.write_mem(QSPI_CONFIG_R, ENABLE_MASTER_TRAN, QSPI_CR_WIDTH);
    
    bool tx_state = tx_empty();
    while (tx_state == false){
        tx_state = tx_empty();
    }

    qspi_controller.write_mem(QSPI_SSR, CHIP_DESELECT, QSPI_STD_WIDTH);
    qspi_controller.write_mem(QSPI_CONFIG_R, DISABLE_MASTER_TRAN, QSPI_CR_WIDTH);
    qspi_controller.read_mem(QSPI_DRR, QSPI_STD_WIDTH);
    uint8_t config = qspi_controller.read_mem(QSPI_DRR, QSPI_STD_WIDTH);


    return config;

}

/*  Check whether a write is in progress on the flash device
*   Returns True if the Status Reg Bit 0 is High.
*/
bool write_in_progress(){
    std::bitset<8> status(read_flash_status_reg());
    return ((status[0] == 1) ? true : false);
}

/*  Check whether write is enabled on the flash device
*   Returns True if the Status Reg Bit 1 is High.
*/
bool is_write_enabled(){
    std::bitset<8> status(read_flash_status_reg());
    return ((status[1] == 1) ? true : false);
}

/*  Check whether an erase error occured on the flash device
*   Returns True if the Status Reg Bit 5 is High.
*/
bool erase_error(){
    std::bitset<8> status(read_flash_status_reg());
    return ((status[5] == 1) ? true : false);
}

/*  Check whether a quad mode is enabled on the flash device
*   Returns True if the Config Reg Bit 1 is High.
*/
bool is_quad_enabled(){
    std::bitset<8> config(read_flash_config_reg());
    return ((config[1] == 1) ? true : false);
}

/*  Check whether a program error occured in the flash memory
*   Returns True if the statius Reg Bit 6 is High.
*/
bool program_error(){
    std::bitset<8> status(read_flash_status_reg());
    return ((status[6] == 1) ? true : false);
}

/*
*   Sets the write enable latch in the flash memory device if it is not already set
*   Quits program execution if write fails to enable.
*/
void write_enable(){

    if(!is_write_enabled()){

        qspi_controller.write_mem(QSPI_CONFIG_R, RESET_FIFO_MSTR_CONFIG_ENABLE, QSPI_CR_WIDTH);
        qspi_controller.write_mem(QSPI_DTR, FL_WRITE_ENABLE, QSPI_STD_WIDTH);
        qspi_controller.write_mem(QSPI_DTR, 0x00, QSPI_STD_WIDTH);
        qspi_controller.write_mem(QSPI_DTR, 0x01, QSPI_STD_WIDTH);
        qspi_controller.write_mem(QSPI_SSR, CHIP_SELECT, QSPI_STD_WIDTH);
        qspi_controller.write_mem(QSPI_CONFIG_R, ENABLE_MASTER_TRAN, QSPI_CR_WIDTH);
        qspi_controller.write_mem(QSPI_SSR, CHIP_DESELECT, QSPI_STD_WIDTH);
        qspi_controller.write_mem(QSPI_CONFIG_R, DISABLE_MASTER_TRAN, QSPI_CR_WIDTH);
        
        if(!is_write_enabled()){
            std::cout << "Write Failed to Enable" << std::endl;
            exit(1);
        }
    }
}

/*
*   Writes a single byte value to the flash memory status_register and configuration register
*   @param : status_reg uint8_t, the value to write to the status register
*   @param : config_reg uint8_t, the value to write to the config register
*/
void write_flash_registers(uint8_t& status_reg, uint8_t& config_reg){

    write_enable();
    qspi_controller.write_mem(QSPI_CONFIG_R, RESET_FIFO_MSTR_CONFIG_ENABLE, QSPI_CR_WIDTH);
    qspi_controller.write_mem(QSPI_DTR, FL_WRITE_REG, QSPI_STD_WIDTH);
    qspi_controller.write_mem(QSPI_DTR, status_reg, QSPI_STD_WIDTH);
    qspi_controller.write_mem(QSPI_DTR, config_reg, QSPI_STD_WIDTH);
    qspi_controller.write_mem(QSPI_SSR, CHIP_SELECT, QSPI_STD_WIDTH);
    qspi_controller.write_mem(QSPI_CONFIG_R, ENABLE_MASTER_TRAN, QSPI_CR_WIDTH);
    qspi_controller.write_mem(QSPI_SSR, CHIP_DESELECT, QSPI_STD_WIDTH);
    qspi_controller.write_mem(QSPI_CONFIG_R, DISABLE_MASTER_TRAN, QSPI_CR_WIDTH);

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
        qspi_controller.read_mem(QSPI_DRR, QSPI_STD_WIDTH);
    }

    //read the real ID
    for(int i = 0; i < 2; i++){
        printf("0x%X\n", qspi_controller.read_mem(QSPI_DRR, QSPI_STD_WIDTH));
    }
}


/*  Loop function to perform @num_bytes of memory reads from @address in blocks of @increment.
*   Writes the byte data to the binary file, in binary format, if to_file is true.
*   Returns the next address to read from 
*   Used to handle reading bytes which do not align on the FIFO depth.
*/
uint32_t topup_read(uint32_t& address, unsigned long& num_bytes, unsigned long& increment, uint8_t& crc, bool to_file){

    //crc = 0;
    uint8_t buffer_128[increment]; 
    qspi_controller.write_mem(QSPI_CONFIG_R, RESET_FIFO_MSTR_CONFIG_ENABLE, QSPI_CR_WIDTH);
    qspi_controller.write_mem(QSPI_DTR, FL_READ_QUAD_OUT, QSPI_STD_WIDTH);

    // bit shift the four byte address into 4 bytes
    uint8_t msb = (address & 0xFF000000) >> 24;
    uint8_t mid1 = (address & 0x00FF0000) >> 16;
    uint8_t mid2 = (address & 0x0000FF00) >> 8;
    uint8_t lsb = (address & 0x000000FF);

    qspi_controller.write_mem(QSPI_DTR, msb, QSPI_STD_WIDTH);
    qspi_controller.write_mem(QSPI_DTR, mid1, QSPI_STD_WIDTH);
    qspi_controller.write_mem(QSPI_DTR, mid2, QSPI_STD_WIDTH);
    qspi_controller.write_mem(QSPI_DTR, lsb, QSPI_STD_WIDTH);

    for(int j =0; j < increment + PREAMBLE_SIZE; j++){
        qspi_controller.write_mem(QSPI_DTR, DUMMY_DATA, QSPI_STD_WIDTH);
    }
    qspi_controller.write_mem(QSPI_SSR, CHIP_SELECT, QSPI_STD_WIDTH);
    qspi_controller.write_mem(QSPI_CONFIG_R, ENABLE_MASTER_TRAN, QSPI_CR_WIDTH);
    
    bool tx_state = tx_empty();
    while (tx_state == false){
        tx_state = tx_empty();
    }

    // read out but don't print the preamble junk
    for(int k = 0; k < PREAMBLE_SIZE; k++){
        qspi_controller.read_mem(QSPI_DRR, QSPI_STD_WIDTH);
    }

    // read and write bytes to binary file 
    for(int d =0; d < increment; d++){
        uint8_t byte = qspi_controller.read_mem(QSPI_DRR, QSPI_STD_WIDTH);
        //out_file.write((char*)&byte, sizeof byte);
        buffer_128[d] = byte;
        uint8_t crc_byte = (uint8_t) (byte ^ crc); //XOR the byte
        crc = crc_table[crc_byte];
    }
    if(to_file){
        out_file.write((char*)&buffer_128, sizeof (buffer_128));
        memset(&buffer_128[0], 0, sizeof(buffer_128));
    }
    unsigned long bytes_read = increment;

    while(bytes_read < num_bytes){
        for(int j =0; j < increment; j++){
            qspi_controller.write_mem(QSPI_DTR, DUMMY_DATA, QSPI_STD_WIDTH);
        }

        bool tx_state = tx_empty();
        while (tx_state == false){
            tx_state = tx_empty();
        }

        // read and write bytes to binary file 
        for(int d =0; d < increment; d++){
            uint8_t byte = qspi_controller.read_mem(QSPI_DRR, QSPI_STD_WIDTH);
            buffer_128[d] = byte;
            uint8_t crc_byte = (uint8_t) (byte ^ crc); //XOR the byte
            crc = crc_table[crc_byte];
        }
        if(to_file){
            out_file.write((char*)&buffer_128, sizeof (buffer_128));
            memset(&buffer_128[0], 0, sizeof(buffer_128));
        }
        bytes_read +=increment;
    }
    qspi_controller.write_mem(QSPI_SSR, CHIP_DESELECT, QSPI_STD_WIDTH);
    qspi_controller.write_mem(QSPI_CONFIG_R, DISABLE_MASTER_TRAN, QSPI_CR_WIDTH);

    return bytes_read;
}

/*  Sets up the read memory loop function, calculates how many bytes to read across the FIFO depth
*   If to_file is true - writes to file. 
*   Calls read_loop with the FIFO aligned num_bytes and then with the overflow num_bytes
*   Calculates the time the read has taken in milliseconds.
*   Returns the CRC code.
*/
uint8_t read_spansion_memory(uint32_t& mem_address, unsigned long& num_bytes, std::string& filename, bool to_file){

    std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
    
    unsigned long increment = FIFO_DEPTH; //max FIFO = 128
    unsigned long int FIFO_aligned_num_bytes = (floor(num_bytes/FIFO_DEPTH)) * FIFO_DEPTH;   // calculate how many times 128 goes into num_bytes round down to nearest integer calculate how many bytes can be read evenly on the FIFO boundary
    unsigned long int overflow_bytes = num_bytes - FIFO_aligned_num_bytes; // find the overflow between even bytes and requested bytes. 
   
    if(to_file){
        out_file.open(filename, std::ios::out | std::ios::binary);
        if(!out_file.is_open()){
            std::cout << "Failed to Open Bin File.. Quiting" << std::endl;
            exit(1);
        }
    }
    
    uint8_t crc = 0;
    //quad_enable.
    if(!is_quad_enabled()){
        uint8_t status = 0x00;
        uint8_t config = 0x02;
        write_flash_registers(status, config);
    }
    if(is_quad_enabled()){
        
        uint32_t next_addr = topup_read(mem_address, FIFO_aligned_num_bytes, increment, crc, to_file);
        topup_read(next_addr, overflow_bytes, overflow_bytes, crc, to_file);
        printf("CRC code for read : 0x%X\n", crc);
        if(to_file){
            out_file.close();
        }
        std::chrono::high_resolution_clock::time_point finish = std::chrono::high_resolution_clock::now();
        std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(finish - start).count() << " ms to read" << std::endl;
    }
    else{
        std::cout << "Quad Mode Did Not Enable, Read Operation In-valid" << std::endl;
    }
    return crc;
}

/*
*   Erase the entire (64MB) flash memory by setting all bytes to 0xFF.
*   @param flash_num, int currently used to protect the flash memory 1 from being erased.
*
*/
void erase_flash_memory(int& flash_num){

    //temporay hack to ensure we dont erase flash 1..
    if(flash_num == 1){
        std::cerr << "FATAL : COMMAND SET TO ERASE FLASH MEMORY CHIP 1.. QUITING" << std::endl;
        exit(1);
    }
    else{
    
        std::chrono::high_resolution_clock::time_point start_erase = std::chrono::high_resolution_clock::now();
        write_enable();
        qspi_controller.write_mem(QSPI_CONFIG_R, RESET_FIFO_MSTR_CONFIG_ENABLE, QSPI_CR_WIDTH);
        qspi_controller.write_mem(QSPI_DTR, FL_BULK_ERASE, QSPI_STD_WIDTH);

        qspi_controller.write_mem(QSPI_SSR, CHIP_SELECT, QSPI_STD_WIDTH);
        qspi_controller.write_mem(QSPI_CONFIG_R, ENABLE_MASTER_TRAN, QSPI_CR_WIDTH);

        qspi_controller.write_mem(QSPI_SSR, CHIP_DESELECT, QSPI_STD_WIDTH);
        qspi_controller.write_mem(QSPI_CONFIG_R, DISABLE_MASTER_TRAN, QSPI_CR_WIDTH);

        bool wip = write_in_progress();
        while(wip == true){
            wip = write_in_progress();
        }

        if(erase_error()){
            std::cout << "Erase Error Has Occured, Perform a Clear Status Register Operation to Reset the Device" << std::endl;
        }
        else{
            std::chrono::high_resolution_clock::time_point finish_erase = std::chrono::high_resolution_clock::now();
            std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(finish_erase - start_erase).count() << " ms to erase." << std::endl;
            std::cout << "Erase Operation Complete" << std::endl;
        }

    }
}


/*
*   Writes @num_bytes to the flash memory device, starting at @mem_address from in_file 
*   Writes the FIFO aligned number of bytes provided from the binary file in_file in blocks of 512 bytes (page size)
*   Returns the next address to write to 
*   Quits program execution if there was a program error.
*/
uint32_t file_write_loop(uint32_t& mem_address, unsigned long& num_bytes, uint8_t& crc){

    //uint8_t crc = 0;
    uint8_t buffer[FIFO_DEPTH];
    in_file.read((char*)(&buffer[0]), FIFO_DEPTH);  
    if(!in_file){
        std::cout << "File Failed to read FIFO Depth Bytes\n" << std::endl;
    }

    uint32_t fbyte_address = mem_address;

    if(!is_write_enabled()){
        write_enable();
    }
    qspi_controller.write_mem(QSPI_CONFIG_R, RESET_FIFO_MSTR_CONFIG_ENABLE, QSPI_CR_WIDTH);
    qspi_controller.write_mem(QSPI_DTR, FL_QUAD_PP, QSPI_STD_WIDTH);

    uint8_t msb = (fbyte_address & 0xFF000000) >> 24;
    uint8_t mid1 = (fbyte_address & 0x00FF0000) >> 16;
    uint8_t mid2 = (fbyte_address & 0x0000FF00) >> 8;
    uint8_t lsb = (fbyte_address & 0x000000FF);

    qspi_controller.write_mem(QSPI_DTR, msb, QSPI_STD_WIDTH);
    qspi_controller.write_mem(QSPI_DTR, mid1, QSPI_STD_WIDTH);
    qspi_controller.write_mem(QSPI_DTR, mid2, QSPI_STD_WIDTH);
    qspi_controller.write_mem(QSPI_DTR, lsb, QSPI_STD_WIDTH);


    for(int d =0; d < FIFO_DEPTH; d++){
        qspi_controller.write_mem(QSPI_DTR, buffer[d], QSPI_STD_WIDTH);
        uint8_t byte = (uint8_t) (buffer[d] ^ crc); //XOR the byte
        crc = crc_table[byte];
    }

    qspi_controller.write_mem(QSPI_SSR, CHIP_SELECT, QSPI_STD_WIDTH);
    qspi_controller.write_mem(QSPI_CONFIG_R, ENABLE_MASTER_TRAN, QSPI_CR_WIDTH);
    
    bool tx__state = tx_empty();
    while (tx__state == false){
        tx__state = tx_empty();
    }

    unsigned long bytes_written = FIFO_DEPTH;
    memset(&buffer[0], 0, sizeof(buffer));
    in_file.seekg(FIFO_DEPTH);

    while(bytes_written < num_bytes){

        in_file.read((char*)(&buffer[0]), FIFO_DEPTH);  
        if(!in_file){
            std::cout << "File Failed to read FIFO Depth Bytes\n" << std::endl;
        }
        for(int d =0; d < FIFO_DEPTH; d++){
            qspi_controller.write_mem(QSPI_DTR, buffer[d], QSPI_STD_WIDTH);
            uint8_t byte = (uint8_t) (buffer[d] ^ crc); //XOR the byte
            crc = crc_table[byte];
        }
        if(bytes_written % 512 == 0){
            qspi_controller.write_mem(QSPI_SSR, CHIP_SELECT, QSPI_STD_WIDTH);
            qspi_controller.write_mem(QSPI_CONFIG_R, ENABLE_MASTER_TRAN, QSPI_CR_WIDTH);
        }
        bool tx_state_ = tx_empty();
        while (tx_state_ == false){
            tx_state_ = tx_empty();
        }

        bytes_written += FIFO_DEPTH;
        memset(&buffer[0], 0, sizeof(buffer));
        in_file.seekg(bytes_written);

        if(bytes_written % 512 == 0){
            qspi_controller.write_mem(QSPI_SSR, CHIP_DESELECT, QSPI_STD_WIDTH);
            qspi_controller.write_mem(QSPI_CONFIG_R, DISABLE_MASTER_TRAN, QSPI_CR_WIDTH);
    
            bool wip = write_in_progress();
            while(wip == true){
                wip = write_in_progress();
            }

            write_enable();

            qspi_controller.write_mem(QSPI_CONFIG_R, RESET_FIFO_MSTR_CONFIG_ENABLE, QSPI_CR_WIDTH);
            qspi_controller.write_mem(QSPI_DTR, FL_QUAD_PP, QSPI_STD_WIDTH);

            uint8_t msb = (bytes_written & 0xFF000000) >> 24;
            uint8_t mid1 = (bytes_written & 0x00FF0000) >> 16;
            uint8_t mid2 = (bytes_written & 0x0000FF00) >> 8;
            uint8_t lsb = (bytes_written & 0x000000FF);

            qspi_controller.write_mem(QSPI_DTR, msb, QSPI_STD_WIDTH);
            qspi_controller.write_mem(QSPI_DTR, mid1, QSPI_STD_WIDTH);
            qspi_controller.write_mem(QSPI_DTR, mid2, QSPI_STD_WIDTH);
            qspi_controller.write_mem(QSPI_DTR, lsb, QSPI_STD_WIDTH);
        }
    }

    qspi_controller.write_mem(QSPI_SSR, CHIP_DESELECT, QSPI_STD_WIDTH);
    qspi_controller.write_mem(QSPI_CONFIG_R, DISABLE_MASTER_TRAN, QSPI_CR_WIDTH);
    
    bool wip = write_in_progress();
    while(wip == true){
        wip = write_in_progress();
    }

    if(program_error()){
        std::cout << "Write Failed" << std::endl;
        exit(1);
    }
    return bytes_written;
}

/*  Loop function to perform @num_bytes of memory writes starting from @mem_address, calculates the CRC of the file
*   Writes the byte data from the binary file in_file 
*   Used to handle writing bytes which do not align on the FIFO depth.
*   Quits program execution if there was a program error.
*/
void custom_file_write_loop(uint32_t& mem_address, unsigned long& num_bytes, uint8_t& crc){
    
    uint8_t buffer[num_bytes];
    in_file.seekg(mem_address);
    in_file.read((char*)(&buffer[0]), num_bytes);

    if(!in_file){
        std::cout << "File Failed to read FIFO Depth Bytes\n" << std::endl;
    }
    if(!is_write_enabled()){
        write_enable();
    }

    qspi_controller.write_mem(QSPI_CONFIG_R, RESET_FIFO_MSTR_CONFIG_ENABLE, QSPI_CR_WIDTH);
    qspi_controller.write_mem(QSPI_DTR, FL_QUAD_PP, QSPI_STD_WIDTH);

    uint8_t msb = (mem_address & 0xFF000000) >> 24;
    uint8_t mid1 = (mem_address & 0x00FF0000) >> 16;
    uint8_t mid2 = (mem_address & 0x0000FF00) >> 8;
    uint8_t lsb = (mem_address & 0x000000FF);

    qspi_controller.write_mem(QSPI_DTR, msb, QSPI_STD_WIDTH);
    qspi_controller.write_mem(QSPI_DTR, mid1, QSPI_STD_WIDTH);
    qspi_controller.write_mem(QSPI_DTR, mid2, QSPI_STD_WIDTH);
    qspi_controller.write_mem(QSPI_DTR, lsb, QSPI_STD_WIDTH);

    for(int d =0; d < num_bytes; d++){
        qspi_controller.write_mem(QSPI_DTR, buffer[d], QSPI_STD_WIDTH);
        uint8_t byte = (uint8_t) (buffer[d] ^ crc); //XOR the byte
        crc = crc_table[byte];
    }
    qspi_controller.write_mem(QSPI_SSR, CHIP_SELECT, QSPI_STD_WIDTH);
    qspi_controller.write_mem(QSPI_CONFIG_R, ENABLE_MASTER_TRAN, QSPI_CR_WIDTH);
    
    bool tx_state_ = tx_empty();
    while (tx_state_ == false){
        tx_state_ = tx_empty();
    }
    
    qspi_controller.write_mem(QSPI_SSR, CHIP_DESELECT, QSPI_STD_WIDTH);
    qspi_controller.write_mem(QSPI_CONFIG_R, DISABLE_MASTER_TRAN, QSPI_CR_WIDTH);
        
    bool wip = write_in_progress();
    while(wip == true){
        wip = write_in_progress();
    }

    if(program_error()){
        std::cout << "Write Failed" << std::endl;
        exit(1);
    }
    memset(&buffer[0], 0, sizeof(buffer));
}


/*
*   Sets up the looping file_write functions to write @num bytes from @filename, starting from @mem_address and to flash memory @flash_num 
*   configures the binary file in_file to use.
*   erases the flash memory and ensures both write and quad mode is enabled on the flash
*   Quits if the file fails to open  or there is a program error. 
*/
void write_flash_memory(int& flash_num, uint32_t& mem_address, unsigned long& num_bytes, std::string& filename, bool& verify){

    in_file.open(filename, std::ios::in | std::ios::binary);

    if(!in_file){
        std::cout << "File failed to open" << std::endl;
        exit(1);
    }

    erase_flash_memory(flash_num);    
    std::chrono::high_resolution_clock::time_point start_write = std::chrono::high_resolution_clock::now();

    unsigned long int FIFO_aligned_num_bytes = (floor(num_bytes/FIFO_DEPTH)) * FIFO_DEPTH;   // calculate how many times 128 goes into num_bytes round down to nearest integer calculate how many bytes can be read evenly on the FIFO boundary
    unsigned long int overflow_bytes = num_bytes - FIFO_aligned_num_bytes; // find the overflow between even bytes and requested bytes. 

    write_enable();
    uint8_t crc = 0;

    if(!is_quad_enabled()){
        uint8_t status = 0x00;
        uint8_t config = 0x02;
        write_flash_registers(status, config);
    }
    else{
        
        uint32_t next_address = file_write_loop(mem_address, FIFO_aligned_num_bytes, crc);
        custom_file_write_loop(next_address, overflow_bytes, crc);
        if(program_error()){
            std::cout << "Write Failed" << std::endl;
            exit(1);
        }
        else{

            printf("CRC code for write : 0x%X\n", crc);
            std::chrono::high_resolution_clock::time_point finish_write = std::chrono::high_resolution_clock::now();
            std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(finish_write - start_write).count() << " ms to write." << std::endl;
            std::cout << "Write Successfull" << std::endl;
        }
    }
    //read the flash memory, not to file, calculating the CRC.
    if(verify){

        uint8_t read_crc = read_spansion_memory(mem_address, num_bytes,filename, false);
        
        if(crc == read_crc){
            std::cout << "Flash Program Verified Successfully" << std::endl;
        }
        else{
            std::cout << "Flash Program Verification Failed" << std::endl;
        }
    }

}

/*
*   Checks through the command line arguments and returns true if the parameter was found
*   If the parameter was found, @value is initiated with the flag value.
*   Returns true/false depending on whether the param was found.
*/
bool arg_found(int arg_num, char * argv[], char * param, std::string& value){
    
    bool found = false;
    for(int i = 1; i < (arg_num); i++){
        if (strcmp(argv[i], param) == 0){
            found = true;
            if((arg_num - 1) > i){
                value = argv[i+1];
            }
            else{
                value = "null";
            }
        }
    }
    return found;
}

/*
*   Helper function to initialise the qspi_controller and the multiplexer memory maps
*/
void init_qspi_mux(){
    qspi_controller.init_mmap();
    multiplexer.init_mmap();
}

/*
*   Helper function to unmap the qspi_controller and the multiplexer memory maps and deset the multiplexer
*/
void un_init_qspi_mux(){
    qspi_controller.unmap();
    multiplexer.write_mem(MUX_OFFSET, MUX_DESET, MUX_WIDTH);
    multiplexer.unmap();
}


/*
*   Helper function to set the flash memory chip to use through the multiplexer MMAP
*   @param flash, integer value representing the flash chip number
*   Returns 1 if an invalid flash memory chip was called.( ! 1-4 )
*/
int set_flash(int flash){

    switch(flash){
        case 1:
            std::cout << "Using Flash Memory Chip 1.." << std::endl;
            multiplexer.write_mem(MUX_OFFSET, MUX_SET_FL1, MUX_WIDTH);
            break;
        case 2:
            std::cout << "Using Flash Memory Chip 2.." << std::endl;
            multiplexer.write_mem(MUX_OFFSET, MUX_SET_FL2, MUX_WIDTH);
            break;
        case 3:
            std::cout << "Using Flash Memory Chip 3.." << std::endl;
            multiplexer.write_mem(MUX_OFFSET, MUX_SET_FL3, MUX_WIDTH);
            break;
        case 4:
            std::cout << "Using Flash Memory Chip 4.." << std::endl;
            multiplexer.write_mem(MUX_OFFSET, MUX_SET_FL4, MUX_WIDTH);
            break;
        default:
            std::cerr << "Invalid flash number, acceptable flash numbers are 1 - 4" << std::endl;
            return 1;
            break;
    }
}

/*
*   Helper function to search for the -f flash number command line argument and set the flash through set_flash()
*   Returns the flash number or -1 if the set_flash() fails due to an invalid flash number
*   @param argc command line argument count
*   @param argv[], command line argument array
*/
int find_set_flash(int argc, char * argv[]){

    int flash_num;
    char* end;
    std::string value;
    
    // if we find a -f flag, pass the value 
    if(arg_found(argc, argv, "-f", value)){

        flash_num = strtoul(value.c_str(), &end, 10); 
        if(set_flash(flash_num) != 1){
            // do nothing
        }
        else{
            return -1; // invalid flash number, quit.
        }
    }
    else{ // no flash flag was provided, use the default - 1.
        std::cout << "Using default flash" << std::endl;
        set_flash(DEFAULT_FLASH);
    }
    return flash_num;
}

/*
*   Helper function to search for the -a address command line argument
*   If no -a flag is found, the default value of 0x0 is returned.
*   Returns the address value or -1 if an invalid memory address is provided
*   @param argc command line argument count
*   @param argv[], command line argument array
*/
uint32_t find_address(int argc, char * argv[]){

    uint32_t address;
    char* end;
    std::string value;
    
    // if we find a -a flag, pass the value 
    if(arg_found(argc, argv, "-a", value)){

        address = strtoul(value.c_str(), &end, 16); 
        if(address < 0 || address > MAX_FLASH_ADDRESS){
            std::cerr << "Address cannot be less than 0  or greater than 64MB" << std::endl;
            return -1;
        }
        else{
            return address;
        }
    }
    else{
        std::cout << "No address provided, using default address of 0x00000000" << std::endl;
        return 0x0;
    }
}

/*
*   Helper function to search for the -s size command line argument
*   If no -s is provided, the default of 64MB is used.
*   Returns the size value or -1 if an invalid size is provided
*   @param argc command line argument count
*   @param argv[], command line argument array
*/
uint32_t find_size(int argc, char * argv[]){

    uint32_t size;
    char* end;
    std::string value;
    
    // if we find a -a flag, pass the value 
    if(arg_found(argc, argv, "-s", value)){

        size = strtoul(value.c_str(), &end, 10); 
        if(size < 1 || size > SIXTY_FOUR_MB){
            std::cout << "size cannot be less than 1 byte or greater than 64MB" << std::endl;
            return -1;
        }
        else{
            return size;
        }
    }
    else{
        std::cout << "No size was provided, using default size of 64MB" << std::endl;
        return SIXTY_FOUR_MB;
    }
}

/*
*   Helper function to search for the -o output command line argument
*   If no -o is provided, the default filename is used
*   Returns the filename with the extension of .bin
*   @param argc command line argument count
*   @param argv[], command line argument array
*/
std::string get_out_filename(int argc, char * argv[]){

    std::string file = "flash_memory_bitfile_default";
    char* end;
    std::string value;
    
    // if we find a -a flag, pass the value 
    if(arg_found(argc, argv, "-o", value)){
        file = value + BIN_EXT;
    }
    else{
        std::cout << "No filename was provided, using default filename 'flash_memory_bitfile_default.bin' " << std::endl;
        file = file + BIN_EXT;
    }
    return file;
}

/*
*   Helper function to search for the -i input command line argument
*   Returns the filename with the extension of .bin or "-1" if no -i was provided.
*   @param argc command line argument count
*   @param argv[], command line argument array
*/
std::string get_in_filename(int argc, char * argv[]){

    std::string file;// = "flash_memory_bitfile_default";
    char* end;
    std::string value;
    
    // if we find a -a flag, pass the value 
    if(arg_found(argc, argv, "-i", value)){
        file = value + BIN_EXT;
        return file;
    }
    else{
        std::cout << "No filename was provided to program the flash array with" << std::endl;
        return "-1";
    }
}

/*
*   Prints the command line help menu to show usage.
*/
void print_help(){

    std::cerr << "Usage : " << std::endl; 
    std::cerr << "-p <cmd - program> (program flash memory)" << std::endl;
    std::cerr << "-e <cmd - erase> (erase flash memory)" << std::endl;
    std::cerr << "-r <cmd - read> (read flash memory)" << std::endl; 
    std::cerr << "-v <config option - verify> (verify flash memory contents)" << std::endl; 
    std::cerr << "-a <config option - address> (hex address to start read/write from)" << std::endl;
    std::cerr << "-s <config option - size> (number of bytes to read/write)" << std::endl;
    std::cerr << "-i <config option - input> (binary file input to program/verify flash memory with)" << std::endl;
    std::cerr << "-o <config option - output> (binary file to output flash read to)" << std::endl;
    std::cerr << "-f <config option - flash #> (flash device to use, 1-4)" << std::endl;
    std::cerr << "-h <print help> (prints this usage guide)" << std::endl; 
}



int main(int argc, char* argv[]){

    calc_CRC8_table();
   
   //pass cmd line arguments
    if(argc == 1){
        std::cerr << "Insufficient arguments.. quiting." << std::endl;
        print_help();
        return 1;
    }
    else if (argc == 2 && strcmp(argv[1], "-h") == 0){
        print_help();   
        return 1;
    }
    else{
        std::string value;
        bool verify = false;
        if(arg_found(argc, argv, "-p", value)){
 
            init_qspi_mux();

            int flash = find_set_flash(argc, argv);
            if(flash == -1){
                un_init_qspi_mux();
                return 1;
            }
            else{
                uint32_t start_address = find_address(argc, argv);
                if(start_address == -1){
                    un_init_qspi_mux();
                    return 1;
                }
                else{
                    std::string filename = get_in_filename(argc, argv);
                    if(filename.compare("-1") == 0){
                        return 1;
                    }
                    else{
                        unsigned long size = find_size(argc, argv);
                        if(size == -1){
                            un_init_qspi_mux();
                            return 1;
                        }
                        else if(size == SIXTY_FOUR_MB){
                            size -= start_address;
                        }
                        if(arg_found(argc, argv, "-v", value)){
                            verify = true;
                        }
                        printf("Writing %d bytes to flash number %d starting at address 0x%X, from a file called %s.\n", size, flash, start_address, filename.c_str());
                        write_flash_memory(flash, start_address, size, filename, verify);
                    } 
                }
            }
            un_init_qspi_mux();
            return 0;
        }

        else if(arg_found(argc, argv, "-r", value)){

            init_qspi_mux();

            int flash = find_set_flash(argc, argv);
            if(flash == -1){
                un_init_qspi_mux();
                return 1;
            }
            else{
                uint32_t start_address = find_address(argc, argv);
                if(start_address == -1){
                    un_init_qspi_mux();
                    return 1;
                }
                else{
                    std::string filename = get_out_filename(argc, argv);
                    unsigned long size = find_size(argc, argv);
                    if(size == -1){
                        un_init_qspi_mux();
                        return 1;
                    }
                    else if(size == SIXTY_FOUR_MB){
                        size -= start_address;
                    }
                    printf("Reading %d bytes from flash number %d starting at address 0x%X, printing to a file called %s.\n", size, flash, start_address, filename.c_str());
                    read_spansion_memory(start_address, size, filename, true);  
                }
                un_init_qspi_mux();
            }
            return 0;
        }

        else if(arg_found(argc, argv, "-e", value)){

            init_qspi_mux();
            int flash = find_set_flash(argc, argv);
            if(flash == -1){
                un_init_qspi_mux();
                return 1;
            }
            else{
                printf("Erasing flash number %d.\n", flash);
                erase_flash_memory(flash);
            }
            un_init_qspi_mux();
            return 0;
        }
        else{
            std::cerr << "No suitable program command was provided" << std::endl;
            print_help();
            return 1;
        }

        return 0;
    }

    
}


