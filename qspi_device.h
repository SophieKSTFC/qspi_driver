/*
*   qspi_device.h
*   @Author Sophie Kirkham STFC, 2018
*   Header file for the qspi_device class ..
*   A class to provide the core functionality required to drive the QSPI Controller
*   And multiplexer on board the FEM-II to enable program/erase/read operations..
*   to be performed on the 4 flash memory devices on the FEM-II.
*/

#ifndef QSPI_DEVICE_H_
#define QSPI_DEVICE_H_ 
#include <bitset>
#include <fstream>
#include <math.h>
#include "qspi_controller.h"
#include "multiplexer.h"
#include "qspi_flash_defines.h"

class qspi_device{

    private:

        std::ofstream out_file; // file to read bytes too from memory
        std::ifstream in_file;  // file to write bytes to memory from
        uint8_t crc_table[256]; // cyclic refundancy check (CRC) table
        uint8_t polynominal = 0x1D;     // fixed 8 bit polynominal for CRC

    public: 

        qspi_controller qspi;   // memory mapped qspi_controller
        multiplexer mux;        // memory mapped multiplexer
        
        qspi_device(){};     
        ~qspi_device(){};

        uint8_t read_flash_status_reg();
        bool tx_empty();
        bool rx_empty();
        uint8_t read_flash_config_reg();
        bool write_in_progress();
        bool is_write_enabled();
        bool erase_error();
        bool is_quad_enabled();
        bool program_error();
        void write_enable();
        void read_spansion_id();
        void calc_CRC8_table();
        void erase_flash_memory(int& flash_num);
        void write_flash_registers(uint8_t& status_reg, uint8_t& config_reg);
       
        
        uint32_t read_n_bytes(uint32_t& address, 
                            unsigned long& num_bytes, 
                            unsigned long& increment, 
                            uint8_t& crc, 
                            bool to_file
                            );

        uint8_t read_flash_memory(uint32_t& mem_address, 
                                    unsigned long& num_bytes, 
                                    std::string& filename, 
                                    bool to_file
                                    );
        
        uint32_t write_n_fifo_aligned_bytes_from_file(uint32_t& mem_address, 
                                unsigned long& num_bytes, 
                                uint8_t& crc
                                );  

        void write_n_unaligned_bytes_from_file(uint32_t& mem_address, 
                                    unsigned long& num_bytes, 
                                    uint8_t& crc
                                    );
                                    
        void write_flash_memory(int& flash_num, 
                                uint32_t& mem_address, 
                                unsigned long& num_bytes, 
                                std::string& filename, 
                                bool& verify
                                );

};

#endif

