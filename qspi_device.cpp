/*
*   qspi_device.cpp
*   @Author Sophie Kirkham STFC, 2018
*   Implementation of the qspi_device class
*   A class to provide the core functionality required to drive the QSPI Controller
*   And multiplexer on board the FEM-II to enable program/erase/read operations..
*   to be performed on the 4 flash memory devices on the FEM-II.
*/

#include "qspi_device.h"

/*
*   qspi_device constructor method, constructs a qspi_device object
*   Calls the constructor for both qspi and mux, initialising them with the
*   base address definitions from qspi_flash_defines.h
*   Calculates the crc8 table for future use.
*/
qspi_device::qspi_device() : qspi(QSPI_BASE), mux(MUX_BASE){
     this->calc_CRC8_table();
}

/*
*   Populates crc_table by calculating all CRC values for each byte value (0-255)
*/
void qspi_device::calc_CRC8_table(){

    for(int div = 0; div < 256; div++){ //iterate through the byte values
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
        this->crc_table[div] = current_byte; // populate the crc_table
    }
}

/*  Check whether the TX buffer in the QSPI controller is empty
*   @returns True if the Status Reg Bit 2 is High.
*/
bool qspi_device::tx_empty(){
    std::bitset<8> qspi_status(this->qspi.read_mem(QSPI_STATUS_R, QSPI_STD_WIDTH));
    return ((qspi_status[2] == 1) ? true : false);
}

/*  Check whether the RX buffer in the QSPI controller is empty
*   @returns True if the Status Reg Bit 0 is High.
*/
bool qspi_device::rx_empty(){
    std::bitset<8> qspi_status(this->qspi.read_mem(QSPI_STATUS_R, QSPI_STD_WIDTH));
    return ((qspi_status[0] == 1) ? true : false);
}

/*  Reads the status register of the flash memory device
*   @returns status_val : the byte value of the status register.
*/
uint8_t qspi_device::read_flash_status_reg(){  

    // reset the fifo, enable master configuration
    this->qspi.write_mem(QSPI_CONFIG_R, RESET_FIFO_MSTR_CONFIG_ENABLE, QSPI_CR_WIDTH);
    // issue flash read status instruction code onto the data transmit reg
    this->qspi.write_mem(QSPI_DTR, FL_READ_STATUS, QSPI_STD_WIDTH);
    // push one dummy data byte onto the data transmit reg
    this->qspi.write_mem(QSPI_DTR, DUMMY_DATA, QSPI_STD_WIDTH);
    // issue chip select instruction onto the slave select register
    this->qspi.write_mem(QSPI_SSR, CHIP_SELECT, QSPI_STD_WIDTH);
    // issue enable master transaction on the config reg to start the QSPI clock
    this->qspi.write_mem(QSPI_CONFIG_R, ENABLE_MASTER_TRAN, QSPI_CR_WIDTH);
    
    // wait for tx buffer to be empty
    bool tx_state = tx_empty();
    while (tx_state == false){
        tx_state = tx_empty();
    }

    // issue chip deselect instruction onto the slave select register
    this->qspi.write_mem(QSPI_SSR, CHIP_DESELECT, QSPI_STD_WIDTH);
    // issue disable master transaction on the config reg to stop the QSPI clock
    this->qspi.write_mem(QSPI_CONFIG_R, DISABLE_MASTER_TRAN, QSPI_CR_WIDTH);
    //  read the dummy data byte and discard from the data receive reg
    this->qspi.read_mem(QSPI_DRR, QSPI_STD_WIDTH);
    //  read the status register value from the data receive regiser
    uint8_t status_val = this->qspi.read_mem(QSPI_DRR, QSPI_STD_WIDTH);
    
    return status_val;
}

/*  Reads the config register of the flash memory device
*   @return config_val : the byte value of the config register.
*/
uint8_t qspi_device::read_flash_config_reg(){

    // reset the fifo, enable master configuration
    this->qspi.write_mem(QSPI_CONFIG_R, RESET_FIFO_MSTR_CONFIG_ENABLE, QSPI_CR_WIDTH);
    // issue flash read config instruction code onto the data transmit reg
    this->qspi.write_mem(QSPI_DTR, FL_READ_CONFIG, QSPI_STD_WIDTH);
    // push one dummy data byte onto the data transmit reg
    this->qspi.write_mem(QSPI_DTR, DUMMY_DATA, QSPI_STD_WIDTH);
     // issue chip select instruction onto the slave select register
    this->qspi.write_mem(QSPI_SSR, CHIP_SELECT, QSPI_STD_WIDTH);
    // issue enable master transaction on the config reg to start the QSPI clock
    this->qspi.write_mem(QSPI_CONFIG_R, ENABLE_MASTER_TRAN, QSPI_CR_WIDTH);
    
    // wait for tx buffer to be empty
    bool tx_state = tx_empty();
    while (tx_state == false){
        tx_state = tx_empty();
    }
    // issue chip deselect instruction onto the slave select register
    this->qspi.write_mem(QSPI_SSR, CHIP_DESELECT, QSPI_STD_WIDTH);
    // issue disable master transaction on the config reg to stop the QSPI clock
    this->qspi.write_mem(QSPI_CONFIG_R, DISABLE_MASTER_TRAN, QSPI_CR_WIDTH);
    //  read the dummy data byte and discard from the data receive reg
    this->qspi.read_mem(QSPI_DRR, QSPI_STD_WIDTH);
    //  read the config register value from the data receive regiser
    uint8_t config_val = this->qspi.read_mem(QSPI_DRR, QSPI_STD_WIDTH);

    return config_val;

}

/*  Check whether a write is in progress on the flash device
*   @returns True if the Status Reg Bit 0 is High.
*/
bool qspi_device::write_in_progress(){
    std::bitset<8> status(read_flash_status_reg());
    return ((status[0] == 1) ? true : false);
}

/*  Check whether write is enabled on the flash device
*   @returns True if the Status Reg Bit 1 is High.
*/
bool qspi_device::is_write_enabled(){
    std::bitset<8> status(read_flash_status_reg());
    return ((status[1] == 1) ? true : false);
}

/*  Check whether an erase error occured on the flash device
*   @returns True if the Status Reg Bit 5 is High.
*/
bool qspi_device::erase_error(){
    std::bitset<8> status(read_flash_status_reg());
    return ((status[5] == 1) ? true : false);
}

/*  Check whether a quad mode is enabled on the flash device
*   @returns True if the Config Reg Bit 1 is High.
*/
bool qspi_device::is_quad_enabled(){
    std::bitset<8> config(read_flash_config_reg());
    return ((config[1] == 1) ? true : false);
}

/*  Check whether a program error occured in the flash memory
*   @returns True if the statius Reg Bit 6 is High.
*/
bool qspi_device::program_error(){
    std::bitset<8> status(read_flash_status_reg());
    return ((status[6] == 1) ? true : false);
}

/*
*   Sets the write enable latch in the flash memory device
*   Checks whether write enable is already set before setting the latch
*   @throws mem_exception : if write fails to enable.
*/
void qspi_device::write_enable(){

    if(!is_write_enabled()){

        // reset the fifo, enable master configuration
        this->qspi.write_mem(QSPI_CONFIG_R, RESET_FIFO_MSTR_CONFIG_ENABLE, QSPI_CR_WIDTH);
        // issue flash write enable instruction code onto the data transmit reg
        this->qspi.write_mem(QSPI_DTR, FL_WRITE_ENABLE, QSPI_STD_WIDTH);
        // push 0x00, 0x01 onto the data transmit transmit register
        this->qspi.write_mem(QSPI_DTR, 0x00, QSPI_STD_WIDTH);
        this->qspi.write_mem(QSPI_DTR, 0x01, QSPI_STD_WIDTH);
        // issue chip select instruction onto the slave select register
        this->qspi.write_mem(QSPI_SSR, CHIP_SELECT, QSPI_STD_WIDTH);
        // issue enable master transaction on the config reg to start the QSPI clock
        this->qspi.write_mem(QSPI_CONFIG_R, ENABLE_MASTER_TRAN, QSPI_CR_WIDTH);
        // issue chip deselect instruction onto the slave select register
        this->qspi.write_mem(QSPI_SSR, CHIP_DESELECT, QSPI_STD_WIDTH);
        // issue disable master transaction on the config reg to stop the QSPI clock
        this->qspi.write_mem(QSPI_CONFIG_R, DISABLE_MASTER_TRAN, QSPI_CR_WIDTH);
        
        // check whether write enabled, throw exception if failed.
        if(!is_write_enabled()){
            throw mem_exception("Write Failed to Enable");
        }
    }
}

/*
*   Writes a single byte value to both the flash memory status_register ..
*   and configuration register, calls write_enable().
*   @param : status_reg uint8_t, the value to write to the status register
*   @param : config_reg uint8_t, the value to write to the config register
*/
void qspi_device::write_flash_registers(uint8_t& status_reg, uint8_t& config_reg){

    write_enable(); // enable write

    // reset the fifo, enable master configuration
    this->qspi.write_mem(QSPI_CONFIG_R, RESET_FIFO_MSTR_CONFIG_ENABLE, QSPI_CR_WIDTH);
    // issue flash write registers instruction code onto the data transmit reg
    this->qspi.write_mem(QSPI_DTR, FL_WRITE_REG, QSPI_STD_WIDTH);
    // push the status register value byte onto the data transmit reg
    this->qspi.write_mem(QSPI_DTR, status_reg, QSPI_STD_WIDTH);
    // push the config register value byte onto the data transmit reg
    this->qspi.write_mem(QSPI_DTR, config_reg, QSPI_STD_WIDTH);
     // issue chip select instruction onto the slave select register
    this->qspi.write_mem(QSPI_SSR, CHIP_SELECT, QSPI_STD_WIDTH);
    // issue enable master transaction on the config reg to start the QSPI clock
    this->qspi.write_mem(QSPI_CONFIG_R, ENABLE_MASTER_TRAN, QSPI_CR_WIDTH);
    // issue chip deselect instruction onto the slave select register
    this->qspi.write_mem(QSPI_SSR, CHIP_DESELECT, QSPI_STD_WIDTH);
    // issue disable master transaction on the config reg to stop the QSPI clock
    this->qspi.write_mem(QSPI_CONFIG_R, DISABLE_MASTER_TRAN, QSPI_CR_WIDTH);

}


/*  Reads the Device ID of the Spansion Flash Memory
*   Prints the two byte ID.
*/
void qspi_device::read_spansion_id(){

    // reset the fifo, enable master configuration
    this->qspi.write_mem(QSPI_CONFIG_R, RESET_FIFO_MSTR_CONFIG_ENABLE, QSPI_CR_WIDTH);
    // issue flash read ID instruction code onto the data transmit reg
    this->qspi.write_mem(QSPI_DTR, FL_READ_ID, QSPI_STD_WIDTH);

    // push 3 x 0x00 onto the data transmit transmit register as addressing
    for(int i = 0; i < 3; i++){
        this->qspi.write_mem(QSPI_DTR, 0x00, QSPI_STD_WIDTH);
    }
    // push 2 x dummy data bytes onto the data transmit register
    for(int i = 0; i < 2; i++){
        this->qspi.write_mem(QSPI_DTR, DUMMY_DATA, QSPI_STD_WIDTH);
    }

    // issue chip select instruction onto the slave select register
    this->qspi.write_mem(QSPI_SSR, CHIP_SELECT, QSPI_STD_WIDTH);
    // issue enable master transaction on the config reg to start the QSPI clock
    this->qspi.write_mem(QSPI_CONFIG_R, ENABLE_MASTER_TRAN, QSPI_CR_WIDTH);

    //check for the tx buffer to be empty
    bool tx_state = tx_empty();
    while (tx_state == false){
        tx_state = tx_empty();
    }
    // issue chip deselect instruction onto the slave select register
    this->qspi.write_mem(QSPI_SSR, CHIP_DESELECT, QSPI_STD_WIDTH);
    // issue disable master transaction on the config reg to stop the QSPI clock
    this->qspi.write_mem(QSPI_CONFIG_R, DISABLE_MASTER_TRAN, QSPI_CR_WIDTH);

    // discard the 4 dummy bytes.
    for(int i = 0; i < 4; i++){
        this->qspi.read_mem(QSPI_DRR, QSPI_STD_WIDTH);
    }
    // print the two hex ID bytes using std::cout
    for(int i = 0; i < 2; i++){
        std::cout << std::hex << "Device ID : 0x" << \
        this->qspi.read_mem(QSPI_DRR, QSPI_STD_WIDTH) << std::dec << std::endl;
    }
}


/*  Reads a specificed number of bytes from the flash memory device
*   @param address : the memory address to being the read operation from
*   @param num_bytes :  the number of bytes to read in total
*   @param increment :  the number of bytes to read in one transaction
*   @param crc :    the current cyclic redundancy check value
*   @param to_file : boolean value, when true - bytes are written to .bin file
*   Reads in set increments to allow for bytes to be read that are not aligned..
*   with the fifo depth. Constantly refills the tx buffer to complete the ..
*   read operation in minimal transactions.
*   Calcualtes the crc code for the read operation on the fly.
*   Writes the byte data to the binary file, in binary format, if to_file is true.
*   @returns the next address to read from 
*/
uint32_t qspi_device::read_n_bytes(uint32_t& address, unsigned long& num_bytes, unsigned long& increment, uint8_t& crc, bool to_file){

    //initialise an empty buffer to hold @increment numbers of bytes for writing
    uint8_t write_buffer[increment]; 

    // reset the fifo, enable master configuration
    this->qspi.write_mem(QSPI_CONFIG_R, RESET_FIFO_MSTR_CONFIG_ENABLE, QSPI_CR_WIDTH);
    // issue flash read quad out instruction code onto the data transmit reg
    this->qspi.write_mem(QSPI_DTR, FL_READ_QUAD_OUT, QSPI_STD_WIDTH);

    // bit shift the four byte address into 4 bytes
    uint8_t msb = (address & 0xFF000000) >> 24;
    uint8_t mid1 = (address & 0x00FF0000) >> 16;
    uint8_t mid2 = (address & 0x0000FF00) >> 8;
    uint8_t lsb = (address & 0x000000FF);

    // push the address bytes onto the data transmit register
    this->qspi.write_mem(QSPI_DTR, msb, QSPI_STD_WIDTH);
    this->qspi.write_mem(QSPI_DTR, mid1, QSPI_STD_WIDTH);
    this->qspi.write_mem(QSPI_DTR, mid2, QSPI_STD_WIDTH);
    this->qspi.write_mem(QSPI_DTR, lsb, QSPI_STD_WIDTH);

    // write the number of bytes we want to read + the preamble size to the ..
    // data transmit register
    for(int j =0; j < increment + PREAMBLE_SIZE; j++){
        this->qspi.write_mem(QSPI_DTR, DUMMY_DATA, QSPI_STD_WIDTH);
    }

    // issue chip select instruction onto the slave select registe
    this->qspi.write_mem(QSPI_SSR, CHIP_SELECT, QSPI_STD_WIDTH);
    // issue enable master transaction on the config reg to start the QSPI clock
    this->qspi.write_mem(QSPI_CONFIG_R, ENABLE_MASTER_TRAN, QSPI_CR_WIDTH);
    
    // wait for the tx buffer to be empty
    bool tx_state = tx_empty();
    while (tx_state == false){
        tx_state = tx_empty();
    }

    // read out and discard the preamble produced by a read transaction from .. 
    // the data receive register
    for(int k = 0; k < PREAMBLE_SIZE; k++){
        this->qspi.read_mem(QSPI_DRR, QSPI_STD_WIDTH);
    }

    // read the actual data bytes from the data transmit register in @increments
    for(int d =0; d < increment; d++){
        uint8_t byte = this->qspi.read_mem(QSPI_DRR, QSPI_STD_WIDTH);
        write_buffer[d] = byte;   // fill the buffer with the data byte
        // calculate the crc code
        uint8_t crc_byte = (uint8_t) (byte ^ crc); // XOR the byte
        crc = this->crc_table[crc_byte]; // look up the crc code for byte value
    }
    // if we are writing data to a bin file, write the write_buffer to out_file
    if(to_file){
        this->out_file.write((char*)&write_buffer, sizeof (write_buffer));
        memset(&write_buffer[0], 0, sizeof(write_buffer));
    }

    unsigned long bytes_read = increment;   //bytes read == increment

    /*  
    *   complete the remaining bytes to be read by constantly filling up the TX
    *   buffer, without starting a new transaction. 
    */
    while(bytes_read < num_bytes){
        for(int j =0; j < increment; j++){
            this->qspi.write_mem(QSPI_DTR, DUMMY_DATA, QSPI_STD_WIDTH);
        }

        //check the tx buffer is empty
        bool tx_state = tx_empty();
        while (tx_state == false){
            tx_state = tx_empty();
        }

        // read the data bytes, fill the write_buffer and calculate the crc code  
        for(int d =0; d < increment; d++){
            uint8_t byte = this->qspi.read_mem(QSPI_DRR, QSPI_STD_WIDTH);
            write_buffer[d] = byte;
            uint8_t crc_byte = (uint8_t) (byte ^ crc);
            crc = this->crc_table[crc_byte];
        }
        // if to_file, write the write_buffer to out_file
        if(to_file){
            this->out_file.write((char*)&write_buffer, sizeof (write_buffer));
            memset(&write_buffer[0], 0, sizeof(write_buffer));
        }
        // increment bytes read by increment
        bytes_read +=increment;
    }
    // once all of the bytes have been read, issue chip deselect command 
    this->qspi.write_mem(QSPI_SSR, CHIP_DESELECT, QSPI_STD_WIDTH);
    // issue disable master transaction on the config reg to stop the QSPI clock
    this->qspi.write_mem(QSPI_CONFIG_R, DISABLE_MASTER_TRAN, QSPI_CR_WIDTH);

    return bytes_read;
}

/*  Reads out a specified number of bytes from a flash memory device
*   @param mem_address : the memory address to start the read from
*   @param num_bytes : the number of bytes to read from the flash
*   @param filename : string value for the .bin file name to print to
*   @param to_file : boolean value, when true the mwemory content is printed to a .bin file
*   @throws mem_exception : if the .bin file fails to open
*   @throws mem_exception : if quad mode failed to enable
*   If to_file is true - writes to file. 
*   Calls read_loop with the FIFO aligned num_bytes and then with the overflow num_bytes
*   Calculates the time the read has taken in milliseconds.
*   @returns crc : a byte value of the CRC code.
*/
uint8_t qspi_device::read_flash_memory(uint32_t& mem_address, unsigned long& num_bytes, std::string& filename, bool to_file){

    std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
    
    unsigned long increment = FIFO_DEPTH; // max safe data in FIFO = 128
    /*  Calculate how many times 128 goes into num_bytes, round down to nearest.. 
        integer. i.e. calculate how many bytes can be read evenly on the 
        FIFO boundary
    */
    unsigned long int FIFO_aligned_num_bytes = (floor(num_bytes/FIFO_DEPTH)) * FIFO_DEPTH;
    // find the overflow between even bytes and requested bytes.    
    unsigned long int overflow_bytes = num_bytes - FIFO_aligned_num_bytes; 
   
    // if we are writing to a file open the out_file in binary mode
    if(to_file){
        this->out_file.open(filename, std::ios::out | std::ios::binary);
        // if the file fails to open throw exception
        if(!this->out_file.is_open()){
            throw mem_exception("Failed to Open .bin File");
        }
    }
    
    // initialise the CRC code for future calculations.
    uint8_t crc = 0;

    // if quad mode is not enabled, enable quad mode 
    if(!is_quad_enabled()){
        uint8_t status = 0x00;
        uint8_t config = 0x02;
        write_flash_registers(status, config);
    }
    if(is_quad_enabled()){
        
        // read the first block of fifo aligned bytes from the memory
        uint32_t next_addr = read_n_bytes(mem_address, 
                                        FIFO_aligned_num_bytes, 
                                        increment, 
                                        crc, 
                                        to_file
                                        );
        // read the overflow byts from the memory device starting next_addr
        read_n_bytes(next_addr, overflow_bytes, overflow_bytes, crc, to_file);
        // print out the CRC code
        std::cout << std::hex << "CRC code for read : 0x" << crc << std::dec 
        << std::endl;
        // if we were writing to a file, close the file now we have finished
        if(to_file){
            this->out_file.close();
        }
        // calculate performance and print the time in ms to complete read
        std::chrono::high_resolution_clock::time_point finish = std::chrono::high_resolution_clock::now();
        std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(finish - start).count() << " ms to read" << std::endl;
    }
    else{
        // throw mem exception if quad mode did not enable.
        throw mem_exception("Quad Mode Did Not Enable, Read Operation In-valid");
    }
    return crc;
}

/*
*   Erase the entire (64MB) flash memory by setting all bytes to 0xFF.
*   @param flash_num, int currently used to protect the flash memory 1 from being erased.
*   @throws mem_exception : if we erase flash number 1.
*   @throws mem_exception : if an erase error occured.
*/
void qspi_device::erase_flash_memory(int& flash_num){

    //temporay hack to ensure we dont erase flash 1..
    if(flash_num == 1){
        throw mem_exception("FATAL : COMMAND SET TO ERASE FLASH MEMORY CHIP 1");
    }
    else{
    
        std::chrono::high_resolution_clock::time_point start_erase = std::chrono::high_resolution_clock::now();
        write_enable(); // enable write
        // reset the fifo, enable master configuration
        this->qspi.write_mem(QSPI_CONFIG_R, RESET_FIFO_MSTR_CONFIG_ENABLE, QSPI_CR_WIDTH);
        // issue flash bulk erase instruction code onto the data transmit reg
        this->qspi.write_mem(QSPI_DTR, FL_BULK_ERASE, QSPI_STD_WIDTH);
        // issue chip select instruction onto the slave select register
        this->qspi.write_mem(QSPI_SSR, CHIP_SELECT, QSPI_STD_WIDTH);
        // issue enable master transaction on the config reg to start the QSPI clock
        this->qspi.write_mem(QSPI_CONFIG_R, ENABLE_MASTER_TRAN, QSPI_CR_WIDTH);
        // issue chip deselect instruction onto the slave select register
        this->qspi.write_mem(QSPI_SSR, CHIP_DESELECT, QSPI_STD_WIDTH);
        // issue disable master transaction on the config reg to stop the QSPI clock
        this->qspi.write_mem(QSPI_CONFIG_R, DISABLE_MASTER_TRAN, QSPI_CR_WIDTH);

        // wait for write to not be in progress i.e. to finish
        bool wip = write_in_progress();
        while(wip == true){
            wip = write_in_progress();
        }
        // check for an erase error.
        if(erase_error()){
            throw mem_exception("Erase Error Has Occured, Perform a Clear Status Register Operation to Reset the Device");
        }
        else{
            // print out performance stats on time to erase.
            std::chrono::high_resolution_clock::time_point finish_erase = std::chrono::high_resolution_clock::now();
            std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(finish_erase - start_erase).count() << " ms to erase." << std::endl;
            std::cout << "Erase Operation Complete" << std::endl;
        }

    }
}


/*
*   Write a specified number of fifo aligned bytes from in_file to a flash memory device.
*   @param mem_address : memory addres to start writing to.
*   @param num_bytes : nubmer of bytes to write to the flash memory
*   @param crc : cyclic redundancy check value
*   Writes byte data to a binary file in blocks of 512 bytes (page boundaries)
*   Calcualtes the crc code on the fly for the write
*   @throws mem_exception : if there is a program error
*   @return bytes_written : the next address to write to 
*/
uint32_t qspi_device::write_n_fifo_aligned_bytes_from_file(uint32_t& mem_address, unsigned long& num_bytes, uint8_t& crc){

    // initialise a buffer to hold 128 bytes
    uint8_t buffer[FIFO_DEPTH];

    // read 128 bytes from in_file into the buffer.
    this->in_file.read((char*)(&buffer[0]), FIFO_DEPTH);  

    // check the file read succeeded.
    if(!this->in_file){
        throw mem_exception("File Failed to read FIFO Depth Bytes");
    }

    uint32_t fbyte_address = mem_address;

    // check that write is enabled, if not - enable it.
    if(!is_write_enabled()){
        write_enable();
    }

    // reset the fifo, enable master configuration
    this->qspi.write_mem(QSPI_CONFIG_R, RESET_FIFO_MSTR_CONFIG_ENABLE, QSPI_CR_WIDTH);
    // issue flash quad page program instruction code onto the data transmit reg
    this->qspi.write_mem(QSPI_DTR, FL_QUAD_PP, QSPI_STD_WIDTH);

    // bit shift the memory address inot 4 bytes
    uint8_t msb = (fbyte_address & 0xFF000000) >> 24;
    uint8_t mid1 = (fbyte_address & 0x00FF0000) >> 16;
    uint8_t mid2 = (fbyte_address & 0x0000FF00) >> 8;
    uint8_t lsb = (fbyte_address & 0x000000FF);

    // push the address onto the data transmit reigster
    this->qspi.write_mem(QSPI_DTR, msb, QSPI_STD_WIDTH);
    this->qspi.write_mem(QSPI_DTR, mid1, QSPI_STD_WIDTH);
    this->qspi.write_mem(QSPI_DTR, mid2, QSPI_STD_WIDTH);
    this->qspi.write_mem(QSPI_DTR, lsb, QSPI_STD_WIDTH);

    // write the buffer of 128 bytes into memory 
    for(int i =0; i < FIFO_DEPTH; i++){
        this->qspi.write_mem(QSPI_DTR, buffer[i], QSPI_STD_WIDTH);
        // calculate the crc as we go
        uint8_t byte = (uint8_t) (buffer[i] ^ crc); //XOR the byte
        crc = this->crc_table[byte];
    }

    // issue chip select instruction onto the slave select register
    this->qspi.write_mem(QSPI_SSR, CHIP_SELECT, QSPI_STD_WIDTH);
    // issue enable master transaction on the config reg to start the QSPI clock
    this->qspi.write_mem(QSPI_CONFIG_R, ENABLE_MASTER_TRAN, QSPI_CR_WIDTH);
    
    // wait for the tx buffer to be empty
    bool tx__state = tx_empty();
    while (tx__state == false){
        tx__state = tx_empty();
    }

    // reset the buffer to 0 to read in the next 128 bytes.
    unsigned long bytes_written = FIFO_DEPTH;
    memset(&buffer[0], 0, sizeof(buffer));
    this->in_file.seekg(FIFO_DEPTH);

    // process the remaining bytes needed to write in one transaction
    while(bytes_written < num_bytes){

        // read 128 bytes from the file into buffer
        this->in_file.read((char*)(&buffer[0]), FIFO_DEPTH);

        // check the read operation is ok 
        if(!this->in_file){
            std::cout << "File Failed to read FIFO Depth Bytes\n" << std::endl;
        }

        // write the 128 bytes to memory
        for(int i =0; i < FIFO_DEPTH; i++){
            this->qspi.write_mem(QSPI_DTR, buffer[i], QSPI_STD_WIDTH);
            // calculate the crc code as we go
            uint8_t byte = (uint8_t) (buffer[i] ^ crc); //XOR the byte
            crc = this->crc_table[byte];
        }

        // if we have written a page (512 bytes), enable the transaction
        // we can only write in blocks of 512 bytes
        if(bytes_written % PAGE_SIZE == 0){
             // issue chip select instruction onto the slave select register
            this->qspi.write_mem(QSPI_SSR, CHIP_SELECT, QSPI_STD_WIDTH);
            // issue enable master transaction on the config reg to start the QSPI clock
            this->qspi.write_mem(QSPI_CONFIG_R, ENABLE_MASTER_TRAN, QSPI_CR_WIDTH);
        }

        // wait for the tx buffer to be empty
        bool tx_state_ = tx_empty();
        while (tx_state_ == false){
            tx_state_ = tx_empty();
        }

        //reset the buffer ready for the next 128 bytes
        bytes_written += FIFO_DEPTH;
        memset(&buffer[0], 0, sizeof(buffer));
        this->in_file.seekg(bytes_written);
        
        // if we've written a page we need to set up and start a new transaction
        if(bytes_written % PAGE_SIZE == 0){

            // issue chip deselect instruction onto the slave select register
            this->qspi.write_mem(QSPI_SSR, CHIP_DESELECT, QSPI_STD_WIDTH);
            // issue disable master transaction on the config reg to stop the QSPI clock
            this->qspi.write_mem(QSPI_CONFIG_R, DISABLE_MASTER_TRAN, QSPI_CR_WIDTH);
    
            // wait for the page write to complete
            bool wip = write_in_progress();
            while(wip == true){
                wip = write_in_progress();
            }

            // re-enable write
            write_enable();
            // reset the fifo, enable master configuration
            this->qspi.write_mem(QSPI_CONFIG_R, RESET_FIFO_MSTR_CONFIG_ENABLE, QSPI_CR_WIDTH);
            // issue flash quad page program instruction code onto the data transmit reg
            this->qspi.write_mem(QSPI_DTR, FL_QUAD_PP, QSPI_STD_WIDTH);

            // bit shift the address into 4 bytes
            uint8_t msb = (bytes_written & 0xFF000000) >> 24;
            uint8_t mid1 = (bytes_written & 0x00FF0000) >> 16;
            uint8_t mid2 = (bytes_written & 0x0000FF00) >> 8;
            uint8_t lsb = (bytes_written & 0x000000FF);
            // push the address onto the data transmit register
            this->qspi.write_mem(QSPI_DTR, msb, QSPI_STD_WIDTH);
            this->qspi.write_mem(QSPI_DTR, mid1, QSPI_STD_WIDTH);
            this->qspi.write_mem(QSPI_DTR, mid2, QSPI_STD_WIDTH);
            this->qspi.write_mem(QSPI_DTR, lsb, QSPI_STD_WIDTH);
        }
    }

    // once we have written all of the bytes..
    // issue chip deselect instruction onto the slave select register
    this->qspi.write_mem(QSPI_SSR, CHIP_DESELECT, QSPI_STD_WIDTH);
    // issue disable master transaction on the config reg to stop the QSPI clock
    this->qspi.write_mem(QSPI_CONFIG_R, DISABLE_MASTER_TRAN, QSPI_CR_WIDTH);
    
    // make sure all write operations have completed
    bool wip = write_in_progress();
    while(wip == true){
        wip = write_in_progress();
    }

    // check for a program error
    if(program_error()){
        throw mem_exception ("Program Error : Write Operation Failed.");
    }

    return bytes_written;
}

/*
*   Write a specified number of un-fifo aligned bytes from in_file to a flash memory device.
*   @param mem_address : memory addres to start writing to.
*   @param num_bytes : nubmer of bytes to write to the flash memory
*   @param crc : cyclic redundancy check value
*   Writes byte data to a binary file in one transaction
*   @throws mem_exception : if there is a program error
*   Calcualtes the crc code on the fly for the write
*/
void qspi_device::write_n_unaligned_bytes_from_file(uint32_t& mem_address, unsigned long& num_bytes, uint8_t& crc){

     // initialise a buffer to hold 128 bytes
    uint8_t buffer[num_bytes];
    //set the read pointer in the file to be the memory address.
    this->in_file.seekg(mem_address);
    // read 128 bytes
    this->in_file.read((char*)(&buffer[0]), num_bytes);

    // check the file read succeeded.
    if(!this->in_file){
        std::cout << "File Failed to read FIFO Depth Bytes\n" << std::endl;
    }

    // check that write is enabled, if not - enable it.
    if(!is_write_enabled()){
        write_enable();
    }
    // reset the fifo, enable master configuration
    this->qspi.write_mem(QSPI_CONFIG_R, RESET_FIFO_MSTR_CONFIG_ENABLE, QSPI_CR_WIDTH);
    // issue flash quad page program instruction code onto the data transmit reg
    this->qspi.write_mem(QSPI_DTR, FL_QUAD_PP, QSPI_STD_WIDTH);

    // bit shift the memory address inot 4 bytes
    uint8_t msb = (mem_address & 0xFF000000) >> 24;
    uint8_t mid1 = (mem_address & 0x00FF0000) >> 16;
    uint8_t mid2 = (mem_address & 0x0000FF00) >> 8;
    uint8_t lsb = (mem_address & 0x000000FF);

    // push the address onto the data transmit reigster
    this->qspi.write_mem(QSPI_DTR, msb, QSPI_STD_WIDTH);
    this->qspi.write_mem(QSPI_DTR, mid1, QSPI_STD_WIDTH);
    this->qspi.write_mem(QSPI_DTR, mid2, QSPI_STD_WIDTH);
    this->qspi.write_mem(QSPI_DTR, lsb, QSPI_STD_WIDTH);

    // write the buffer of 128 bytes into memory 
    for(int i =0; i < num_bytes; i++){
        this->qspi.write_mem(QSPI_DTR, buffer[i], QSPI_STD_WIDTH);
        // calculate the crc as we go
        uint8_t byte = (uint8_t) (buffer[i] ^ crc); //XOR the byte
        crc = this->crc_table[byte];
    }

    // issue chip select instruction onto the slave select register
    this->qspi.write_mem(QSPI_SSR, CHIP_SELECT, QSPI_STD_WIDTH);
     // issue enable master transaction on the config reg to start the QSPI clock
    this->qspi.write_mem(QSPI_CONFIG_R, ENABLE_MASTER_TRAN, QSPI_CR_WIDTH);

    //wait for the tx buffer to be empty
    bool tx_state_ = tx_empty();
    while (tx_state_ == false){
        tx_state_ = tx_empty();
    }

    // issue chip deselect instruction onto the slave select register
    this->qspi.write_mem(QSPI_SSR, CHIP_DESELECT, QSPI_STD_WIDTH);
    // issue disable master transaction on the config reg to stop the QSPI clo
    this->qspi.write_mem(QSPI_CONFIG_R, DISABLE_MASTER_TRAN, QSPI_CR_WIDTH);

    // wait for the write to complete  
    bool wip = write_in_progress();
    while(wip == true){
        wip = write_in_progress();
    }

    // check for program errors
    if(program_error()){
        throw mem_exception ("Program Error : Write Operation Failed");
    }

    // clear the buffer
    memset(&buffer[0], 0, sizeof(buffer));
}


/*
*   Writes a specificed number of bytes to a flash memory device
*   @param flash_num :  the flash number to erase
*   @param mem_address : memory address to start the write to
*   @param num_bytes : number of bytes to write to the memory
*   @param filename : string name of the file to program the flash from
*   @param verify :   boolean value, if true the program operation CRC is verified
*   Programs a flash memory by calculating fifo aligned bytes and overflow bytes
*   erases the flash memory and ensures both write and quad mode is enabled on the flash
*   @throws mem_exception : if the file fails to open i.e. does not exist
*   @throws mem_exception : if there is a a program error.
*   @throws mem_exception : if there is a verification error.
*/
void qspi_device::write_flash_memory(int& flash_num, uint32_t& mem_address, unsigned long& num_bytes, std::string& filename, bool& verify){

    // open the in_file with the filename provided.
    this->in_file.open(filename, std::ios::in | std::ios::binary);

    // check that the file opens ok 
    if(!this->in_file){
        throw mem_exception ("File Failed to Open");
    }

    // erase the flash to enable programming
    erase_flash_memory(flash_num);    
    std::chrono::high_resolution_clock::time_point start_write = std::chrono::high_resolution_clock::now();
    // calculate how many bytes can be read evenly on the FIFO boundary
    unsigned long int FIFO_aligned_num_bytes = (floor(num_bytes/FIFO_DEPTH)) * FIFO_DEPTH;
    // find the overflow between even bytes and requested bytes.    
    unsigned long int overflow_bytes = num_bytes - FIFO_aligned_num_bytes; 

    // enable write
    write_enable();
    uint8_t crc = 0;

    // check that quad is enabled, if not - enable it.
    if(!is_quad_enabled()){
        uint8_t status = 0x00;
        uint8_t config = 0x02;
        write_flash_registers(status, config);
    }
    else{
        // write the fifo aligned number of bytes
        uint32_t next_address = write_n_fifo_aligned_bytes_from_file(mem_address, FIFO_aligned_num_bytes, crc);
        // write the left over overflow bytes
        write_n_unaligned_bytes_from_file(next_address, overflow_bytes, crc);

        // check for a program error
        if(program_error()){
            throw mem_exception("Program Error : Write Operation Failed");
        }
        else{
            // print out crc code and timing stats
            std::cout << "CRC code for write : 0x" << std::hex << crc << std::endl << std::dec;
            std::chrono::high_resolution_clock::time_point finish_write = std::chrono::high_resolution_clock::now();
            std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(finish_write - start_write).count() << " ms to write." << std::endl;
            std::cout << "Write Successfull" << std::endl;
        }
    }
   
    if(verify){
        //read the flash memory using the parameters provided and not writing to file
        //calculating the CRC.
        uint8_t read_crc = read_flash_memory(mem_address, num_bytes,filename, false);
        
        if(crc == read_crc){
            std::cout << "Flash Program Verified Successfully" << std::endl;
        }
        else{
            throw mem_exception("Flash Program Verification Failed");
        }
    }

}
