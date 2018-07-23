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
*   Checks through the command line args and returns true if the parameter was found
*   @param arg_num : number of arguments provided
*   @param argv :  array of the command line arguments provided
*   @param param : parameter argument to look for
*   @param value : string reference to store the value of the argument  
*   @returns true if the param was found.
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
*   Sets the flash memory chip to use through the multiplexer MMAP
*   @param flash : integer value representing the flash chip number
*   @param qspi : reference to a qspi_device object 
*   @return 1 if an invalid flash memory chip was called.(! 1-4)
*/
int set_flash(int flash, qspi_device& qspi){
   
    try{
        qspi.select_flash(flash);
    }
    catch(mem_exception& err){
        std::cout << "Error occured during selecting the Flash device : " 
        << err.what() << std::endl;
    }
}

/*
*   Searches for the -f argument and sets the flash through set_flash()
*   @param argc : command line argument count
*   @param argv[] : command line argument array
*   @param qspi : reference to a qspi_device object
*   @return flash_num : the flash number, or -1 if invalid flash num
*/
int find_set_flash(int argc, char * argv[], qspi_device& qspi){

    int flash_num;
    char* end;
    std::string value;
    
    // if we find a -f flag, pass the value 
    if(arg_found(argc, argv, "-f", value)){

        flash_num = strtoul(value.c_str(), &end, 10); 
        if(set_flash(flash_num, qspi) != 1){
            // do nothing
        }
        else{
            return -1; // invalid flash number, quit.
        }
    }
    else{ // no flash flag was provided, use the default - 1.
        std::cout << "Using default flash" << std::endl;
        set_flash(DEFAULT_FLASH, qspi);
    }
    return flash_num;
}

/*
*   Searches for the -a address command line argument
*   If no -a flag is found, the default value of 0x0 is returned.
*   @param argc : command line argument count
*   @param argv[] : command line argument array
*   @return address : the address value or -1 if an invalid memory address is provided
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
*   Searches for the -s size command line argument
*   If no -s is provided, the default of 64MB is used.
*   @param argc command line argument count
*   @param argv[], command line argument array
*   @returns size : the size value or -1 if an invalid size is provided
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
*   Searches for the -o output command line argument
*   If no -o is provided, the default filename is used
*   @param argc : command line argument count
*   @param argv[] : command line argument array
*   @returns file : the filename with the extension of .bin
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
*   Searches for the -i input command line argument
*   @param argc : command line argument count
*   @param argv[] : command line argument array
*   @returns file : the filename with the extension of .bin or .. 
*    "-1" if no -i was provided.
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
                "Binary input filename to program the Flash with, file must pre-exist, required when op = read.")
            ("output_file, o", po::value<std::string>()->default_value(timestamp + "_flash_dump"), 
                "Binary output filename to store Flash memory contents in (Default: <timestamp> + _flash_dump)")
            ("size, s", po::value<unsigned long>(), 
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
        po::notify(vm);

        // pass flags and values
        if(vm.count("operation")){

            operation = vm["operation"].as<std::string>();

            // check an input file was provided for a read operation
            if(operation.compare("read") == 0){
                if(vm.count("input_file")){
                    input_file = vm["input_file"].as<std::string>();
                }
                else{
                    std::cout << "Input file is required when performing a read operation" << std::endl;
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
    }
    catch(const po::error &ex){
        std::cerr << ex.what() << std::endl;
    }

    // print out helpful message
    std::cout << "Performing " << operation << " operation." << std::endl;
    std::cout << "Using flash chip " << flash_chip << std::endl;

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

    // operation not supported - quit.
    else{
        std::cout << "Unsupported operation argument." << std::endl;
        std::cout << options << std::endl;
        clean_exit(qspi);
        exit(1);
    }

    clean_exit(qspi);
    return 0;


    /*
   //pass cmd line arguments
    if(argc == 1){
        std::cerr << "Insufficient arguments.. quiting." << std::endl;
        print_help();
        clean_exit(qspi);
        //return 1;
    }
    else if (argc == 2 && strcmp(argv[1], "-h") == 0){
        print_help();   
        return 1;
    }
    else{
        std::string value;
        bool verify = false;
        if(arg_found(argc, argv, "-p", value)){
 
            //init_qspi_mux(qspi);

            int flash = find_set_flash(argc, argv, qspi);
            if(flash == -1){
                clean_exit(qspi);
                return 1;
            }
            else{
                uint32_t start_address = find_address(argc, argv);
                if(start_address == -1){
                    clean_exit(qspi);
                    return 1;
                }
                else{
                    std::string filename = get_in_filename(argc, argv);
                    if(filename.compare("-1") == 0){
                        clean_exit(qspi);
                        return 1;
                    }
                    else{
                        unsigned long size = find_size(argc, argv);
                        if(size == -1){
                            clean_exit(qspi);
                            return 1;
                        }
                        else if(size == SIXTY_FOUR_MB){
                            size -= start_address;
                        }
                        if(arg_found(argc, argv, "-v", value)){
                            verify = true;
                        }
                        printf("Writing %d bytes to flash number %d starting at address 0x%X, from a file called %s.\n", size, flash, start_address, filename.c_str());
                        
                        try{
                            qspi.write_flash_memory(flash, start_address, size, filename, verify);
                        }
                        catch(mem_exception& err){
                            std::cout << "An error occured during write operation : " 
                            << err.what() << std::endl;
                            clean_exit(qspi);
                            return 1;
                        }
                    } 
                }
            }
            clean_exit(qspi);
            return 0;
        }

        else if(arg_found(argc, argv, "-r", value)){

            int flash = find_set_flash(argc, argv, qspi);
            if(flash == -1){
                clean_exit(qspi);
                return 1;
            }
            else{
                uint32_t start_address = find_address(argc, argv);
                if(start_address == -1){
                    clean_exit(qspi);
                    return 1;
                }
                else{
                    std::string filename = get_out_filename(argc, argv);
                    unsigned long size = find_size(argc, argv);
                    if(size == -1){
                        clean_exit(qspi);
                        return 1;
                    }
                    else if(size == SIXTY_FOUR_MB){
                        size -= start_address;
                    }
                    printf("Reading %d bytes from flash number %d starting at address 0x%X, printing to a file called %s.\n", size, flash, start_address, filename.c_str());
                    try{
                        qspi.read_flash_memory(start_address, size, filename, true);  
                    }
                    catch(mem_exception& err){
                        std::cout << "An error occured during read operation : " 
                        << err.what() << std::endl;
                        clean_exit(qspi);
                        return 1;
                    }
                }
                clean_exit(qspi);
            }
            return 0;
        }
        /*
        else if(arg_found(argc, argv, "-e", value)){

            //init_qspi_mux(qspi);
            int flash = find_set_flash(argc, argv, qspi);
            if(flash == -1){
                clean_exit(qspi);
                return 1;
            }
            else{
                printf("Erasing flash number %d.\n", flash);
                try{
                    qspi.erase_flash_memory(flash);
                }
                catch(mem_exception& err){
                    std::cout << "An error occured during erase operation : " 
                    << err.what() << std::endl;
                    clean_exit(qspi);
                    return 1;                    
                }
            }
            clean_exit(qspi);
            return 0;
        }
        
        else{
            std::cerr << "No suitable program command was provided" << std::endl;
            print_help();
            return 1;
        }
        return 0;
    }

    */
}