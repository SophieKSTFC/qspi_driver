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
#include "qspi_device.h"


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


/*  initialises the qspi and mux memory maps
*   @param qspi : reference to a qspi_device object 
*/
void init_qspi_mux(qspi_device& qspi){

    qspi.qspi.map();
    qspi.mux.map();
}

/*  un maps the qspi and mux memory maps
*   @param qspi : reference to a qspi_device object 
*/
void un_init_qspi_mux(qspi_device& qspi){

    qspi.qspi.unmap();
    qspi.mux.write_mem(MUX_OFFSET, MUX_DESET, MUX_WIDTH);
    qspi.mux.unmap();
}


/*
*   Sets the flash memory chip to use through the multiplexer MMAP
*   @param flash : integer value representing the flash chip number
*   @param qspi : reference to a qspi_device object 
*   @return 1 if an invalid flash memory chip was called.(! 1-4)
*/
int set_flash(int flash, qspi_device& qspi){

    switch(flash){
        case 1:
            std::cout << "Using Flash Memory Chip 1.." << std::endl;
            qspi.mux.write_mem(MUX_OFFSET, MUX_SET_FL1, MUX_WIDTH);
            break;
        case 2:
            std::cout << "Using Flash Memory Chip 2.." << std::endl;
            qspi.mux.write_mem(MUX_OFFSET, MUX_SET_FL2, MUX_WIDTH);
            break;
        case 3:
            std::cout << "Using Flash Memory Chip 3.." << std::endl;
            qspi.mux.write_mem(MUX_OFFSET, MUX_SET_FL3, MUX_WIDTH);
            break;
        case 4:
            std::cout << "Using Flash Memory Chip 4.." << std::endl;
            qspi.mux.write_mem(MUX_OFFSET, MUX_SET_FL4, MUX_WIDTH);
            break;
        default:
            std::cerr << "Invalid flash number, acceptable flash numbers are 1 - 4" << std::endl;
            return 1;
            break;
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



int main(int argc, char* argv[]){

    qspi_device qspi;

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
 
            init_qspi_mux(qspi);

            int flash = find_set_flash(argc, argv, qspi);
            if(flash == -1){
                un_init_qspi_mux(qspi);
                return 1;
            }
            else{
                uint32_t start_address = find_address(argc, argv);
                if(start_address == -1){
                    un_init_qspi_mux(qspi);
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
                            un_init_qspi_mux(qspi);
                            return 1;
                        }
                        else if(size == SIXTY_FOUR_MB){
                            size -= start_address;
                        }
                        if(arg_found(argc, argv, "-v", value)){
                            verify = true;
                        }
                        printf("Writing %d bytes to flash number %d starting at address 0x%X, from a file called %s.\n", size, flash, start_address, filename.c_str());
                        qspi.write_flash_memory(flash, start_address, size, filename, verify);
                    } 
                }
            }
            un_init_qspi_mux(qspi);
            return 0;
        }

        else if(arg_found(argc, argv, "-r", value)){

            init_qspi_mux(qspi);

            int flash = find_set_flash(argc, argv, qspi);
            if(flash == -1){
                un_init_qspi_mux(qspi);
                return 1;
            }
            else{
                uint32_t start_address = find_address(argc, argv);
                if(start_address == -1){
                    un_init_qspi_mux(qspi);
                    return 1;
                }
                else{
                    std::string filename = get_out_filename(argc, argv);
                    unsigned long size = find_size(argc, argv);
                    if(size == -1){
                        un_init_qspi_mux(qspi);
                        return 1;
                    }
                    else if(size == SIXTY_FOUR_MB){
                        size -= start_address;
                    }
                    printf("Reading %d bytes from flash number %d starting at address 0x%X, printing to a file called %s.\n", size, flash, start_address, filename.c_str());
                    qspi.read_flash_memory(start_address, size, filename, true);  
                }
                un_init_qspi_mux(qspi);
            }
            return 0;
        }

        else if(arg_found(argc, argv, "-e", value)){

            init_qspi_mux(qspi);
            int flash = find_set_flash(argc, argv, qspi);
            if(flash == -1){
                un_init_qspi_mux(qspi);
                return 1;
            }
            else{
                printf("Erasing flash number %d.\n", flash);
                qspi.erase_flash_memory(flash);
            }
            un_init_qspi_mux(qspi);
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