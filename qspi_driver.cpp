/*
*   qspi_driver.cpp
*   @Author Sophie Kirkham STFC, 2018
*   Main application entry point for qspi_driver .elf executable
*   Command line tool to drive the QSPI controller 
*   Facilitates read/erase/program access to/from .bin files with the ..
*   4 flash memory devices on-baord the FEM-II
*   Enables CRC-8 Verification of programming 
*/

#include <iostream>
#include <boost/program_options.hpp>
#include "boost/date_time/posix_time/posix_time.hpp"
#include "qspi_device.h"

namespace po = boost::program_options;


/*
*   Unmaps the qspi and multiplexer memory maps and exits the program
*   @throws mem_exception : if unmap() fails.
*/
void clean_exit(qspi_device& qspi){

    try{
        qspi.un_map_qspi_mux();
        exit(1);
    }
    catch(mem_exception& err){
        std::cout << "An error occured during memory map tear-down : " <<
        err.what() << std::endl;
        exit(1);
    }
}

/*
*   Main entry point for the qspi_driver
*   Passess command line arguments and calls the appropriate qspi_device methods
*   @param argc : number of command line arguments provided
*   @param argv : array of command line arguments
*/
int main(int argc, char* argv[]){

    std::string timestamp = boost::posix_time::to_iso_extended_string(boost::posix_time::microsec_clock::local_time());
    std::string operation;
    bool verify = false;
    int flash_chip;
    uint32_t address;
    std::string input_file;
    std::string output_file;
    unsigned long size;
    po::options_description options("Options");

    try{
        // set up options for command line arguments
        options.add_options()
            ("help, h", "Prints the help menu")
            ("operation, op", po::value<std::string>()->required(), 
                "Operation to perform (read, erase, program), mandatory argument.")
            ("flash_chip, f", po::value<int>()->required(),  
                "The flash chip to use (1: Chip 1, 2: Chip 2, 3: Chip 3, 4: Chip 4), mandatory argument.")
            ("verify, v", 
                "Perform a CRC-8 verification of the flash memory contents and the .bin file provided.")
            ("address, a", po::value<uint32_t>()->default_value(0x00000000), 
                "Hexidecimal Flash memory address to start the operation from (Default: 0x00000000.")
            ("input_file, i", po::value<std::string>(), 
                "Binary input filename to program the Flash with, file must pre-exist, required when op = write.")
            ("output_file, o", po::value<std::string>()->default_value(timestamp + "_flash_dump"), 
                "Binary output filename to store Flash memory contents in (Default: <timestamp> + _flash_dump)")
            ("size, s", po::value<unsigned long>()->required(), 
                "Integer-decimal value for the number of bytes to program, read or erase."); 
        
        //generate variables map and parse command line arguments 
        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, options), vm);
        
        // check for help first, print help then quit.
        if(vm.count("help")){
            std::cout << "Usage: qspi_driver [operation][flash number][options]" << std::endl;
            std::cout << options << std::endl;
            exit(1);
        }
        // get any notifications from the variables map i.e. missed required param
        //po::notify(vm);

        // pass flags and values
        if(vm.count("operation")){

            operation = vm["operation"].as<std::string>();

            // check an input file was provided for a read operation
            if(operation.compare("write") == 0){
                if(vm.count("input_file")){
                    input_file = vm["input_file"].as<std::string>();
                }
                else{
                    std::cout << "Input file is required when performing a write operation" << std::endl;
                    exit(1);
                }
            }
        }

        if(vm.count("flash_chip")){
            flash_chip = vm["flash_chip"].as<int>();
        }
        if(vm.count("verify")){
            verify = true;
        }
        if(vm.count("address")){
            char* end;
            // need to check whether address is being populated properly in hex.
            address = vm["address"].as<uint32_t>();
            //address = strtoul(address.c_str(), &end, 16);
        }
        if(vm.count("size")){
            size = vm["size"].as<unsigned long>();
        }
        if(vm.count("output_file")){
            output_file = vm["output_file"].as<std::string>();
        }

        po::notify(vm);
    }
    catch(const po::error &ex){
        std::cerr << ex.what() << std::endl;
    }

    // check and trim the size parameter to prevent memory over runs
    if((size + address) > SIXTY_FOUR_MB){
        std::cout << "Starting memory addres + size is greater than"
        << "flash memory size (64MB), clipping size to prevent overrun" << std::endl;
        size -= (size + address) - SIXTY_FOUR_MB;
    }

    qspi_device qspi; // initialised qspi_device

    // set up the memory mapped areas for qspi and mux
    try{
        qspi.map_qspi_mux();
    }
    catch(mem_exception& err){
        std::cout << "An error occured during memory map set-up : " <<
        err.what() << std::endl;
        exit(1);
    }

    //select the flash chip 
    try{
        qspi.select_flash(flash_chip);
    }
    catch(mem_exception& err){
        std::cout << "Error occured during selecting the Flash device : " 
        << err.what() << std::endl;
        clean_exit(qspi);
        return 1;
    }

    // handle a read operation 
    if(operation.compare("read") == 0){

        std::cout << "Reading " << size << " bytes from flash chip " 
        << flash_chip  << " starting at address " << std::hex << address
        << std::dec << "printing to a file called " << output_file.c_str();

        try{
            qspi.read_flash_memory(address, size, output_file, true);  
        }
        catch(mem_exception& err){
            std::cout << "An error occured during read operation : " 
            << err.what() << std::endl;
            clean_exit(qspi);
            return 1;
        }
    }

    // handle an erase operation
    else if(operation.compare("erase") == 0){

        std::cout << "Erasing flash chip " << flash_chip << std::endl;
        try{
            qspi.erase_flash_memory(flash_chip);
        }
        catch(mem_exception& err){
            std::cout << "An error occured during erase operation : " 
            << err.what() << std::endl;
            clean_exit(qspi);
            return 1;                    
        }
    }

    // handle a program operation 
    else if(operation.compare("program") == 0){

        std::cout << "Writing " << size << " bytes to flash chip " 
        << flash_chip  << " starting at address " << std::hex << address
        << std::dec << "from a file called " << input_file.c_str();
              
        try{
            qspi.write_flash_memory(flash_chip, address, size, input_file, verify);
        }
        catch(mem_exception& err){
            std::cout << "An error occured during write operation : " 
            << err.what() << std::endl;
            clean_exit(qspi);
            return 1;
        }

    }
    else if(operation.compare("") == 0 ){
       
    }

    // operation not supported - quit.
    else{
        std::cout << "Unsupported operation argument." << std::endl;
        std::cout << options << std::endl;
        clean_exit(qspi);
        exit(1);
    }

    clean_exit(qspi);
    return 0;

}