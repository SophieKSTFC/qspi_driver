#include "qspi_device.h"

qspi_device::qspi_device() : qspi(QSPI_BASE), mux(MUX_BASE){
     this->calc_CRC8_table();
}

/*
*   Populates crc_table by calculating all CRC values for each byte value (0-255)
*/
void qspi_device::calc_CRC8_table(){

    for(int div = 0; div < 256; div++){
        uint8_t current_byte = (uint8_t)div;
        for(uint8_t bit = 0; bit < 8; bit ++){
            if((current_byte & 0x80) !=0){
                current_byte <<= 1; // the MSB was a 1, shift it out
                current_byte ^= this->polynominal; // XOR with the polynominal
            }
            else{
                current_byte <<=1; // else it was a 0, just shift it.
            }
        }
        this->crc_table[div] = current_byte;
    }
}

/*  Check whether the TX buffer in the QSPI controller is empty
*   Returns True if the Status Reg Bit 2 is High.
*/
bool qspi_device::tx_empty(){
    std::bitset<8> qspi_status(this->qspi.read_mem(QSPI_STATUS_R, QSPI_STD_WIDTH));
    return ((qspi_status[2] == 1) ? true : false);
}

/*  Check whether the RX buffer in the QSPI controller is empty
*   Returns True if the Status Reg Bit 0 is High.
*/
bool qspi_device::rx_empty(){
    std::bitset<8> qspi_status(this->qspi.read_mem(QSPI_STATUS_R, QSPI_STD_WIDTH));
    return ((qspi_status[0] == 1) ? true : false);
}

/*  Reads the status register of the flash memory device
*   Returns the byte value of the status register.
*/
uint8_t qspi_device::read_flash_status_reg(){  

    this->qspi.write_mem(QSPI_CONFIG_R, RESET_FIFO_MSTR_CONFIG_ENABLE, QSPI_CR_WIDTH);
    this->qspi.write_mem(QSPI_DTR, FL_READ_STATUS, QSPI_STD_WIDTH);
    this->qspi.write_mem(QSPI_DTR, DUMMY_DATA, QSPI_STD_WIDTH);
    this->qspi.write_mem(QSPI_SSR, CHIP_SELECT, QSPI_STD_WIDTH);
    this->qspi.write_mem(QSPI_CONFIG_R, ENABLE_MASTER_TRAN, QSPI_CR_WIDTH);
    
    bool tx_state = tx_empty();
    while (tx_state == false){
        tx_state = tx_empty();
    }
    this->qspi.write_mem(QSPI_SSR, CHIP_DESELECT, QSPI_STD_WIDTH);
    this->qspi.write_mem(QSPI_CONFIG_R, DISABLE_MASTER_TRAN, QSPI_CR_WIDTH);
    this->qspi.read_mem(QSPI_DRR, QSPI_STD_WIDTH);
    uint8_t sr = this->qspi.read_mem(QSPI_DRR, QSPI_STD_WIDTH);
    
    return sr;
}

/*  Reads the config register of the flash memory device
*   Returns the byte value of the config register.
*/
uint8_t qspi_device::read_flash_config_reg(){

    this->qspi.write_mem(QSPI_CONFIG_R, RESET_FIFO_MSTR_CONFIG_ENABLE, QSPI_CR_WIDTH);
    this->qspi.write_mem(QSPI_DTR, FL_READ_CONFIG, QSPI_STD_WIDTH);
    this->qspi.write_mem(QSPI_DTR, DUMMY_DATA, QSPI_STD_WIDTH);
    this->qspi.write_mem(QSPI_SSR, CHIP_SELECT, QSPI_STD_WIDTH);
    this->qspi.write_mem(QSPI_CONFIG_R, ENABLE_MASTER_TRAN, QSPI_CR_WIDTH);
    
    bool tx_state = tx_empty();
    while (tx_state == false){
        tx_state = tx_empty();
    }

    this->qspi.write_mem(QSPI_SSR, CHIP_DESELECT, QSPI_STD_WIDTH);
    this->qspi.write_mem(QSPI_CONFIG_R, DISABLE_MASTER_TRAN, QSPI_CR_WIDTH);
    this->qspi.read_mem(QSPI_DRR, QSPI_STD_WIDTH);
    uint8_t config = this->qspi.read_mem(QSPI_DRR, QSPI_STD_WIDTH);


    return config;

}

/*  Check whether a write is in progress on the flash device
*   Returns True if the Status Reg Bit 0 is High.
*/
bool qspi_device::write_in_progress(){
    std::bitset<8> status(read_flash_status_reg());
    return ((status[0] == 1) ? true : false);
}

/*  Check whether write is enabled on the flash device
*   Returns True if the Status Reg Bit 1 is High.
*/
bool qspi_device::is_write_enabled(){
    std::bitset<8> status(read_flash_status_reg());
    return ((status[1] == 1) ? true : false);
}

/*  Check whether an erase error occured on the flash device
*   Returns True if the Status Reg Bit 5 is High.
*/
bool qspi_device::erase_error(){
    std::bitset<8> status(read_flash_status_reg());
    return ((status[5] == 1) ? true : false);
}

/*  Check whether a quad mode is enabled on the flash device
*   Returns True if the Config Reg Bit 1 is High.
*/
bool qspi_device::is_quad_enabled(){
    std::bitset<8> config(read_flash_config_reg());
    return ((config[1] == 1) ? true : false);
}

/*  Check whether a program error occured in the flash memory
*   Returns True if the statius Reg Bit 6 is High.
*/
bool qspi_device::program_error(){
    std::bitset<8> status(read_flash_status_reg());
    return ((status[6] == 1) ? true : false);
}

/*
*   Sets the write enable latch in the flash memory device if it is not already set
*   Quits program execution if write fails to enable.
*/
void qspi_device::write_enable(){

    if(!is_write_enabled()){

        this->qspi.write_mem(QSPI_CONFIG_R, RESET_FIFO_MSTR_CONFIG_ENABLE, QSPI_CR_WIDTH);
        this->qspi.write_mem(QSPI_DTR, FL_WRITE_ENABLE, QSPI_STD_WIDTH);
        this->qspi.write_mem(QSPI_DTR, 0x00, QSPI_STD_WIDTH);
        this->qspi.write_mem(QSPI_DTR, 0x01, QSPI_STD_WIDTH);
        this->qspi.write_mem(QSPI_SSR, CHIP_SELECT, QSPI_STD_WIDTH);
        this->qspi.write_mem(QSPI_CONFIG_R, ENABLE_MASTER_TRAN, QSPI_CR_WIDTH);
        this->qspi.write_mem(QSPI_SSR, CHIP_DESELECT, QSPI_STD_WIDTH);
        this->qspi.write_mem(QSPI_CONFIG_R, DISABLE_MASTER_TRAN, QSPI_CR_WIDTH);
        
        if(!is_write_enabled()){
            throw mem_exception("Write Failed to Enable");
            //std::cout << "Write Failed to Enable" << std::endl;
            exit(1);
        }
    }
}

/*
*   Writes a single byte value to the flash memory status_register and configuration register
*   @param : status_reg uint8_t, the value to write to the status register
*   @param : config_reg uint8_t, the value to write to the config register
*/
void qspi_device::write_flash_registers(uint8_t& status_reg, uint8_t& config_reg){

    write_enable();
    this->qspi.write_mem(QSPI_CONFIG_R, RESET_FIFO_MSTR_CONFIG_ENABLE, QSPI_CR_WIDTH);
    this->qspi.write_mem(QSPI_DTR, FL_WRITE_REG, QSPI_STD_WIDTH);
    this->qspi.write_mem(QSPI_DTR, status_reg, QSPI_STD_WIDTH);
    this->qspi.write_mem(QSPI_DTR, config_reg, QSPI_STD_WIDTH);
    this->qspi.write_mem(QSPI_SSR, CHIP_SELECT, QSPI_STD_WIDTH);
    this->qspi.write_mem(QSPI_CONFIG_R, ENABLE_MASTER_TRAN, QSPI_CR_WIDTH);
    this->qspi.write_mem(QSPI_SSR, CHIP_DESELECT, QSPI_STD_WIDTH);
    this->qspi.write_mem(QSPI_CONFIG_R, DISABLE_MASTER_TRAN, QSPI_CR_WIDTH);

}


/*  Reads the Device ID of the Spansion Flash Memory
*   Prints the two byte ID.
*/
void qspi_device::read_spansion_id(){

    std::cout << "Reading Spansion ID via MMAP" << std::endl;

    this->qspi.write_mem(QSPI_CONFIG_R, RESET_FIFO_MSTR_CONFIG_ENABLE, QSPI_CR_WIDTH);
    this->qspi.write_mem(QSPI_DTR, FL_READ_ID, QSPI_STD_WIDTH);

    for(int i = 0; i < 3; i++){
        this->qspi.write_mem(QSPI_DTR, 0x00, QSPI_STD_WIDTH);
    }

    for(int i = 0; i < 2; i++){
        this->qspi.write_mem(QSPI_DTR, DUMMY_DATA, QSPI_STD_WIDTH);
    }

    this->qspi.write_mem(QSPI_SSR, CHIP_SELECT, QSPI_STD_WIDTH);
    this->qspi.write_mem(QSPI_CONFIG_R, ENABLE_MASTER_TRAN, QSPI_CR_WIDTH);

    bool tx_state = tx_empty();
    while (tx_state == false){
        tx_state = tx_empty();
    }

    this->qspi.write_mem(QSPI_SSR, CHIP_DESELECT, QSPI_STD_WIDTH);
    this->qspi.write_mem(QSPI_CONFIG_R, DISABLE_MASTER_TRAN, QSPI_CR_WIDTH);

    //push the address bytes out
    for(int i = 0; i < 4; i++){
        this->qspi.read_mem(QSPI_DRR, QSPI_STD_WIDTH);
    }

    //read the real ID
    for(int i = 0; i < 2; i++){
        std::cout << std::hex << "Device ID : 0x" << this->qspi.read_mem(QSPI_DRR, QSPI_STD_WIDTH) << std::dec << std::endl;
        //printf("0x%X\n", this->qspi.read_mem(QSPI_DRR, QSPI_STD_WIDTH));
    }
}


/*  Loop function to perform @num_bytes of memory reads from @address in blocks of @increment.
*   Writes the byte data to the binary file, in binary format, if to_file is true.
*   Returns the next address to read from 
*   Used to handle reading bytes which do not align on the FIFO depth.
*/
uint32_t qspi_device::topup_read(uint32_t& address, unsigned long& num_bytes, unsigned long& increment, uint8_t& crc, bool to_file){

    //crc = 0;
    uint8_t buffer_128[increment]; 
    this->qspi.write_mem(QSPI_CONFIG_R, RESET_FIFO_MSTR_CONFIG_ENABLE, QSPI_CR_WIDTH);
    this->qspi.write_mem(QSPI_DTR, FL_READ_QUAD_OUT, QSPI_STD_WIDTH);

    // bit shift the four byte address into 4 bytes
    uint8_t msb = (address & 0xFF000000) >> 24;
    uint8_t mid1 = (address & 0x00FF0000) >> 16;
    uint8_t mid2 = (address & 0x0000FF00) >> 8;
    uint8_t lsb = (address & 0x000000FF);

    this->qspi.write_mem(QSPI_DTR, msb, QSPI_STD_WIDTH);
    this->qspi.write_mem(QSPI_DTR, mid1, QSPI_STD_WIDTH);
    this->qspi.write_mem(QSPI_DTR, mid2, QSPI_STD_WIDTH);
    this->qspi.write_mem(QSPI_DTR, lsb, QSPI_STD_WIDTH);

    for(int j =0; j < increment + PREAMBLE_SIZE; j++){
        this->qspi.write_mem(QSPI_DTR, DUMMY_DATA, QSPI_STD_WIDTH);
    }
    this->qspi.write_mem(QSPI_SSR, CHIP_SELECT, QSPI_STD_WIDTH);
    this->qspi.write_mem(QSPI_CONFIG_R, ENABLE_MASTER_TRAN, QSPI_CR_WIDTH);
    
    bool tx_state = tx_empty();
    while (tx_state == false){
        tx_state = tx_empty();
    }

    // read out but don't print the preamble junk
    for(int k = 0; k < PREAMBLE_SIZE; k++){
        this->qspi.read_mem(QSPI_DRR, QSPI_STD_WIDTH);
    }

    // read and write bytes to binary file 
    for(int d =0; d < increment; d++){
        uint8_t byte = this->qspi.read_mem(QSPI_DRR, QSPI_STD_WIDTH);
        buffer_128[d] = byte;
        uint8_t crc_byte = (uint8_t) (byte ^ crc); //XOR the byte
        crc = this->crc_table[crc_byte];
    }
    if(to_file){
        this->out_file.write((char*)&buffer_128, sizeof (buffer_128));
        memset(&buffer_128[0], 0, sizeof(buffer_128));
    }
    unsigned long bytes_read = increment;

    while(bytes_read < num_bytes){
        for(int j =0; j < increment; j++){
            this->qspi.write_mem(QSPI_DTR, DUMMY_DATA, QSPI_STD_WIDTH);
        }

        bool tx_state = tx_empty();
        while (tx_state == false){
            tx_state = tx_empty();
        }

        // read and write bytes to binary file 
        for(int d =0; d < increment; d++){
            uint8_t byte = this->qspi.read_mem(QSPI_DRR, QSPI_STD_WIDTH);
            buffer_128[d] = byte;
            uint8_t crc_byte = (uint8_t) (byte ^ crc); //XOR the byte
            crc = this->crc_table[crc_byte];
        }
        if(to_file){
            this->out_file.write((char*)&buffer_128, sizeof (buffer_128));
            memset(&buffer_128[0], 0, sizeof(buffer_128));
        }
        bytes_read +=increment;
    }
    this->qspi.write_mem(QSPI_SSR, CHIP_DESELECT, QSPI_STD_WIDTH);
    this->qspi.write_mem(QSPI_CONFIG_R, DISABLE_MASTER_TRAN, QSPI_CR_WIDTH);

    return bytes_read;
}

/*  Sets up the read memory loop function, calculates how many bytes to read across the FIFO depth
*   If to_file is true - writes to file. 
*   Calls read_loop with the FIFO aligned num_bytes and then with the overflow num_bytes
*   Calculates the time the read has taken in milliseconds.
*   Returns the CRC code.
*/
uint8_t qspi_device::read_spansion_memory(uint32_t& mem_address, unsigned long& num_bytes, std::string& filename, bool to_file){

    std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
    
    unsigned long increment = FIFO_DEPTH; //max FIFO = 128
    unsigned long int FIFO_aligned_num_bytes = (floor(num_bytes/FIFO_DEPTH)) * FIFO_DEPTH;   // calculate how many times 128 goes into num_bytes round down to nearest integer calculate how many bytes can be read evenly on the FIFO boundary
    unsigned long int overflow_bytes = num_bytes - FIFO_aligned_num_bytes; // find the overflow between even bytes and requested bytes. 
   
    if(to_file){
        this->out_file.open(filename, std::ios::out | std::ios::binary);
        if(!this->out_file.is_open()){

            throw mem_exception("Failed to Open .bin File");
            //std::cout << "Failed to Open Bin File.. Quiting" << std::endl;
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
        
        std::cout << std::hex << "CRC code for read : 0x" << crc << std::dec << std::endl;
        //printf("CRC code for read : 0x%X\n", crc);

        if(to_file){
            this->out_file.close();
        }
        std::chrono::high_resolution_clock::time_point finish = std::chrono::high_resolution_clock::now();
        std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(finish - start).count() << " ms to read" << std::endl;
    }
    else{
        throw mem_exception("Quad Mode Did Not Enable, Read Operation In-valid");
        //std::cout << "Quad Mode Did Not Enable, Read Operation In-valid" << std::endl;
    }
    return crc;
}

/*
*   Erase the entire (64MB) flash memory by setting all bytes to 0xFF.
*   @param flash_num, int currently used to protect the flash memory 1 from being erased.
*
*/
void qspi_device::erase_flash_memory(int& flash_num){

    //temporay hack to ensure we dont erase flash 1..
    if(flash_num == 1){
        throw mem_exception("FATAL : COMMAND SET TO ERASE FLASH MEMORY CHIP 1");
        //std::cerr << "FATAL : COMMAND SET TO ERASE FLASH MEMORY CHIP 1.. QUITING" << std::endl;
        exit(1);
    }
    else{
    
        std::chrono::high_resolution_clock::time_point start_erase = std::chrono::high_resolution_clock::now();
        write_enable();
        this->qspi.write_mem(QSPI_CONFIG_R, RESET_FIFO_MSTR_CONFIG_ENABLE, QSPI_CR_WIDTH);
        this->qspi.write_mem(QSPI_DTR, FL_BULK_ERASE, QSPI_STD_WIDTH);

        this->qspi.write_mem(QSPI_SSR, CHIP_SELECT, QSPI_STD_WIDTH);
        this->qspi.write_mem(QSPI_CONFIG_R, ENABLE_MASTER_TRAN, QSPI_CR_WIDTH);

        this->qspi.write_mem(QSPI_SSR, CHIP_DESELECT, QSPI_STD_WIDTH);
        this->qspi.write_mem(QSPI_CONFIG_R, DISABLE_MASTER_TRAN, QSPI_CR_WIDTH);

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
*   Writes @num_bytes to the flash memory device, starting at @mem_address from this->in_file 
*   Writes the FIFO aligned number of bytes provided from the binary file this->in_file in blocks of 512 bytes (page size)
*   Returns the next address to write to 
*   Quits program execution if there was a program error.
*/
uint32_t qspi_device::file_write_loop(uint32_t& mem_address, unsigned long& num_bytes, uint8_t& crc){

    //uint8_t crc = 0;
    uint8_t buffer[FIFO_DEPTH];
    this->in_file.read((char*)(&buffer[0]), FIFO_DEPTH);  
    if(!this->in_file){
        std::cout << "File Failed to read FIFO Depth Bytes\n" << std::endl;
    }

    uint32_t fbyte_address = mem_address;

    if(!is_write_enabled()){
        write_enable();
    }
    this->qspi.write_mem(QSPI_CONFIG_R, RESET_FIFO_MSTR_CONFIG_ENABLE, QSPI_CR_WIDTH);
    this->qspi.write_mem(QSPI_DTR, FL_QUAD_PP, QSPI_STD_WIDTH);

    uint8_t msb = (fbyte_address & 0xFF000000) >> 24;
    uint8_t mid1 = (fbyte_address & 0x00FF0000) >> 16;
    uint8_t mid2 = (fbyte_address & 0x0000FF00) >> 8;
    uint8_t lsb = (fbyte_address & 0x000000FF);

    this->qspi.write_mem(QSPI_DTR, msb, QSPI_STD_WIDTH);
    this->qspi.write_mem(QSPI_DTR, mid1, QSPI_STD_WIDTH);
    this->qspi.write_mem(QSPI_DTR, mid2, QSPI_STD_WIDTH);
    this->qspi.write_mem(QSPI_DTR, lsb, QSPI_STD_WIDTH);


    for(int d =0; d < FIFO_DEPTH; d++){
        this->qspi.write_mem(QSPI_DTR, buffer[d], QSPI_STD_WIDTH);
        uint8_t byte = (uint8_t) (buffer[d] ^ crc); //XOR the byte
        crc = this->crc_table[byte];
    }

    this->qspi.write_mem(QSPI_SSR, CHIP_SELECT, QSPI_STD_WIDTH);
    this->qspi.write_mem(QSPI_CONFIG_R, ENABLE_MASTER_TRAN, QSPI_CR_WIDTH);
    
    bool tx__state = tx_empty();
    while (tx__state == false){
        tx__state = tx_empty();
    }

    unsigned long bytes_written = FIFO_DEPTH;
    memset(&buffer[0], 0, sizeof(buffer));
    this->in_file.seekg(FIFO_DEPTH);

    while(bytes_written < num_bytes){

        this->in_file.read((char*)(&buffer[0]), FIFO_DEPTH);  
        if(!this->in_file){
            std::cout << "File Failed to read FIFO Depth Bytes\n" << std::endl;
        }
        for(int d =0; d < FIFO_DEPTH; d++){
            this->qspi.write_mem(QSPI_DTR, buffer[d], QSPI_STD_WIDTH);
            uint8_t byte = (uint8_t) (buffer[d] ^ crc); //XOR the byte
            crc = this->crc_table[byte];
        }
        if(bytes_written % 512 == 0){
            this->qspi.write_mem(QSPI_SSR, CHIP_SELECT, QSPI_STD_WIDTH);
            this->qspi.write_mem(QSPI_CONFIG_R, ENABLE_MASTER_TRAN, QSPI_CR_WIDTH);
        }
        bool tx_state_ = tx_empty();
        while (tx_state_ == false){
            tx_state_ = tx_empty();
        }

        bytes_written += FIFO_DEPTH;
        memset(&buffer[0], 0, sizeof(buffer));
        this->in_file.seekg(bytes_written);

        if(bytes_written % 512 == 0){
            this->qspi.write_mem(QSPI_SSR, CHIP_DESELECT, QSPI_STD_WIDTH);
            this->qspi.write_mem(QSPI_CONFIG_R, DISABLE_MASTER_TRAN, QSPI_CR_WIDTH);
    
            bool wip = write_in_progress();
            while(wip == true){
                wip = write_in_progress();
            }

            write_enable();

            this->qspi.write_mem(QSPI_CONFIG_R, RESET_FIFO_MSTR_CONFIG_ENABLE, QSPI_CR_WIDTH);
            this->qspi.write_mem(QSPI_DTR, FL_QUAD_PP, QSPI_STD_WIDTH);

            uint8_t msb = (bytes_written & 0xFF000000) >> 24;
            uint8_t mid1 = (bytes_written & 0x00FF0000) >> 16;
            uint8_t mid2 = (bytes_written & 0x0000FF00) >> 8;
            uint8_t lsb = (bytes_written & 0x000000FF);

            this->qspi.write_mem(QSPI_DTR, msb, QSPI_STD_WIDTH);
            this->qspi.write_mem(QSPI_DTR, mid1, QSPI_STD_WIDTH);
            this->qspi.write_mem(QSPI_DTR, mid2, QSPI_STD_WIDTH);
            this->qspi.write_mem(QSPI_DTR, lsb, QSPI_STD_WIDTH);
        }
    }

    this->qspi.write_mem(QSPI_SSR, CHIP_DESELECT, QSPI_STD_WIDTH);
    this->qspi.write_mem(QSPI_CONFIG_R, DISABLE_MASTER_TRAN, QSPI_CR_WIDTH);
    
    bool wip = write_in_progress();
    while(wip == true){
        wip = write_in_progress();
    }

    if(program_error()){
        throw mem_exception ("Program Error : Write Operation Failed.");
        //std::cout << "Write Failed" << std::endl;
        exit(1);
    }
    return bytes_written;
}

/*  Loop function to perform @num_bytes of memory writes starting from @mem_address, calculates the CRC of the file
*   Writes the byte data from the binary file this->in_file 
*   Used to handle writing bytes which do not align on the FIFO depth.
*   Quits program execution if there was a program error.
*/
void qspi_device::custom_file_write_loop(uint32_t& mem_address, unsigned long& num_bytes, uint8_t& crc){
    
    uint8_t buffer[num_bytes];
    this->in_file.seekg(mem_address);
    this->in_file.read((char*)(&buffer[0]), num_bytes);

    if(!this->in_file){
        std::cout << "File Failed to read FIFO Depth Bytes\n" << std::endl;
    }
    if(!is_write_enabled()){
        write_enable();
    }

    this->qspi.write_mem(QSPI_CONFIG_R, RESET_FIFO_MSTR_CONFIG_ENABLE, QSPI_CR_WIDTH);
    this->qspi.write_mem(QSPI_DTR, FL_QUAD_PP, QSPI_STD_WIDTH);

    uint8_t msb = (mem_address & 0xFF000000) >> 24;
    uint8_t mid1 = (mem_address & 0x00FF0000) >> 16;
    uint8_t mid2 = (mem_address & 0x0000FF00) >> 8;
    uint8_t lsb = (mem_address & 0x000000FF);

    this->qspi.write_mem(QSPI_DTR, msb, QSPI_STD_WIDTH);
    this->qspi.write_mem(QSPI_DTR, mid1, QSPI_STD_WIDTH);
    this->qspi.write_mem(QSPI_DTR, mid2, QSPI_STD_WIDTH);
    this->qspi.write_mem(QSPI_DTR, lsb, QSPI_STD_WIDTH);

    for(int d =0; d < num_bytes; d++){
        this->qspi.write_mem(QSPI_DTR, buffer[d], QSPI_STD_WIDTH);
        uint8_t byte = (uint8_t) (buffer[d] ^ crc); //XOR the byte
        crc = this->crc_table[byte];
    }
    this->qspi.write_mem(QSPI_SSR, CHIP_SELECT, QSPI_STD_WIDTH);
    this->qspi.write_mem(QSPI_CONFIG_R, ENABLE_MASTER_TRAN, QSPI_CR_WIDTH);
    
    bool tx_state_ = tx_empty();
    while (tx_state_ == false){
        tx_state_ = tx_empty();
    }
    
    this->qspi.write_mem(QSPI_SSR, CHIP_DESELECT, QSPI_STD_WIDTH);
    this->qspi.write_mem(QSPI_CONFIG_R, DISABLE_MASTER_TRAN, QSPI_CR_WIDTH);
        
    bool wip = write_in_progress();
    while(wip == true){
        wip = write_in_progress();
    }

    if(program_error()){
        throw mem_exception ("Program Error : Write Operation Failed");
        //std::cout << "Write Failed" << std::endl;
        exit(1);
    }
    memset(&buffer[0], 0, sizeof(buffer));
}


/*
*   Sets up the looping file_write functions to write @num bytes from @filename, starting from @mem_address and to flash memory @flash_num 
*   configures the binary file this->in_file to use.
*   erases the flash memory and ensures both write and quad mode is enabled on the flash
*   Quits if the file fails to open  or there is a program error. 
*/
void qspi_device::write_flash_memory(int& flash_num, uint32_t& mem_address, unsigned long& num_bytes, std::string& filename, bool& verify){

    this->in_file.open(filename, std::ios::in | std::ios::binary);

    if(!this->in_file){
        throw mem_exception ("File Failed to Open");
        //std::cout << "File failed to open" << std::endl;
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
            throw mem_exception("Program Error : Write Operation Failed");
            //std::cout << "Write Failed" << std::endl;
            exit(1);
        }
        else{

            std::cout << "CRC code for write : 0x" << std::hex << crc << std::endl << std::dec;

            //printf("CRC code for write : 0x%X\n", crc);
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
