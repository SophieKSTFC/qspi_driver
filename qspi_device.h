#include <bitset>
#include <fstream>
#include <math.h>
#include "qspi_controller.h"
#include "multiplexer.h"
#include "qspi_flash_defines.h"

class qspi_device{

    private:

        qspi_controller qspi; //QSPI_BASE);
        multiplexer mux; //(MUX_BASE);

        std::ofstream out_file; // file to read bytes too from memory
        std::ifstream in_file;// file to write bytes to memory from

        uint8_t crc_table[256];
        uint8_t polynominal = 0x1D; 

    public: 

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
        
        uint32_t topup_read(uint32_t& address, 
                            unsigned long& num_bytes, 
                            unsigned long& increment, 
                            uint8_t& crc, 
                            bool to_file
                            );

        uint8_t read_spansion_memory(uint32_t& mem_address, 
                                    unsigned long& num_bytes, 
                                    std::string& filename, 
                                    bool to_file
                                    );
        
        uint32_t file_write_loop(uint32_t& mem_address, 
                                unsigned long& num_bytes, 
                                uint8_t& crc
                                );  

        void custom_file_write_loop(uint32_t& mem_address, 
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




