import sys
import argparse
import subprocess

  
MUX_BASE = 0x41210008
MUX_SET = 0x104
MUX_DESET = 0x100
MUX_WIDTH = 32 #MUX Commands are in 32 bit

QSPI_CR_WIDTH = 32 #control register is 32 bit
QSPI_STD_WIDTH = 8 # all other QSPI commands are in 8 bit
QSPI_BASE = 0xA0030000
QSPI_CONFIG_R = 0x60
QSPI_STATUS_R = 0x64
QSPI_DTR = 0x68
QSPI_DRR = 0x6C
QSPI_SSR = 0x70

#Flash instruction codes
READ_ID = 0x90
WRITE_ENABLE = 0x06
WRITE_REG = 0x01
READ_STATUS = 0x05
READ_CONFIG = 0x35
READ_BAR = 0x16
READ_QUAD_OUT = 0x6B
READ_QUAD_IO = 0xEB


look_up = {"READ_ID" : READ_ID, 
        "WRITE_ENABLE" : WRITE_ENABLE, 
        "WRITE_REG" : WRITE_REG, 
        "READ_STATUS": READ_STATUS, 
        "READ_CONFIG": READ_CONFIG, 
        "READ_BAR": READ_BAR, 
        "READ_QUAD_OUT" : READ_QUAD_OUT, 
        "READ_QUAD_IO": READ_QUAD_IO
    }


def read_id():

    print("Reading SPANSION Flash ID")

    read_id_sequence = "devmem 0x41210008 32 0x104\n\
                        devmem 0xA0030060 32 0x000001E6\n\
                        devmem 0xA0030060 32 0x000001E6\n\
                        devmem 0xA0030068 8 0x90\n\
                        devmem 0xA0030068 8 0x00\n\
                        devmem 0xA0030068 8 0x00\n\
                        devmem 0xA0030068 8 0x00\n\
                        devmem 0xA0030068 8 0xAB\n\
                        devmem 0xA0030068 8 0xAB\n\
                        devmem 0xA0030070 8 0x00\n\
                        devmem 0xA0030060 32 0x00000086\n\
                        devmem 0xA0030070 8 0x01\n\
                        devmem 0xA0030060 32 0x00000186\n\
                        devmem 0xA003006C 8\n\
                        devmem 0xA003006C 8\n\
                        devmem 0xA003006C 8\n\
                        devmem 0xA003006C 8\n\
                        devmem 0xA003006C 8\n\
                        devmem 0xA003006C 8\n\
                        devmem 0x41210008 32 0x100\n"

    id_pipe = subprocess.Popen(read_id_sequence, shell=True, stdout=subprocess.PIPE)

    output, error = id_pipe.communicate()
    print(output.decode('utf8'))
  

def read_status_register():

    print("Reading SPANSION Flash Status Register")

    read_status_sequence = "devmem 0x41210008 32 0x104 \n\
                            devmem 0xA0030060 32 0x000001E6 \n\
                            devmem 0xA0030068 8 0x05\n\
                            devmem 0xA0030068 8 0x00\n\
                            devmem 0xA0030070 8 0x00\n\
                            devmem 0xA0030060 32 0x00000086\n\
                            devmem 0xA0030070 8 0x01\n\
                            devmem 0xA0030060 32 0x00000186\n\
                            devmem 0xA003006C 8\n\
                            devmem 0xA003006C 8\n\
                            devmem 0x41210008 32 0x100\n"

    id_pipe = subprocess.Popen(read_status_sequence, shell=True, stdout=subprocess.PIPE)
        
    output, error = id_pipe.communicate()
    

    status_register = output[5:]
    print(status_register.decode('utf8'))
    return status_register

def read_config_register():

    print("Reading SPANSION Flash Configuration Register")
    
    read_config_sequence = "devmem 0x41210008 32 0x104\n\
                            devmem 0xA0030060 32 0x000001E6\n\
                            devmem 0xA0030068 8 0x35\n\
                            devmem 0xA0030068 8 0x00\n\
                            devmem 0xA0030070 8 0x00\n\
                            devmem 0xA0030060 32 0x00000086\n\
                            devmem 0xA0030070 8 0x01\n\
                            devmem 0xA0030060 32 0x00000186\n\
                            devmem 0xA003006C 8\n\
                            devmem 0xA003006C 8\n\
                            devmem 0x41210008 32 0x100\n"
    
    id_pipe = subprocess.Popen(read_config_sequence, shell=True, stdout=subprocess.PIPE)

    output, error = id_pipe.communicate()

    config_register = output[5:] #the second byte is the actual register.
    print(config_register.decode('utf8'))

def read_BAR_register():

    print("Reading SPANSION Flash Bank Address Register")

    read_bar_sequence = "devmem 0x41210008 32 0x104\n\
                        devmem 0xA0030060 32 0x000001E6\n\
                        devmem 0xA0030068 8 0x16\n\
                        devmem 0xA0030068 8 0x00\n\
                        devmem 0xA0030070 8 0x00\n\
                        devmem 0xA0030060 32 0x00000086\n\
                        devmem 0xA0030070 8 0x01\n\
                        devmem 0xA0030060 32 0x00000186\n\
                        devmem 0xA003006C 8\n\
                        devmem 0xA003006C 8\n\
                        devmem 0x41210008 32 0x100\n"
    
    id_pipe = subprocess.Popen(read_bar_sequence, shell=True, stdout=subprocess.PIPE)
   
    output, error = id_pipe.communicate()
    print(output.decode('utf8'))


def write_registers(status_reg, config_reg):

    '''
    1. check that write is enabled.
    2. write registers
    3. read registers and check they have actually been written
    '''

    print("Writing Status and Configuration Registers in the SPANSION Flash to %s : %s ." % (status_reg, config_reg))

    write_enable() #check that write enable is set 

    write_reigster_sequence ="devmem 0x41210008 32 0x104\n\
                            devmem 0xA0030060 32 0x000001E6\n\
                            devmem 0xA0030068 8 0x01\n\
                            devmem 0xA0030068 8 " + status_reg +"\n\
                            devmem 0xA0030068 8 " + config_reg +"\n\
                            devmem 0xA0030070 8 0x00\n\
                            devmem 0xA0030060 32 0x00000086\n\
                            devmem 0xA0030070 8 0x01\n\
                            devmem 0xA0030060 32 0x00000186\n\
                            devmem 0x41210008 32 0x100\n"

    id_pipe = subprocess.Popen(write_reigster_sequence, shell=True, stdout=subprocess.PIPE)
    output, error = id_pipe.communicate()
    print(output.decode('utf8'))

    read_status_register() #read back out the status reg
    read_config_register() #read back out the config reg

def write_enable():

    write_latch = int(read_status_register(), 16) #read_status_register() #check if already set..

    if write_latch != 0x02:
        
        print("Setting Write Enable in the SPANSION Flash Status Register")

        write_enable_sequence = "devmem 0x41210008 32 0x104\n\
                                devmem 0xA0030060 32 0x000001E6\n\
                                devmem 0xA0030068 8 0x06\n\
                                devmem 0xA0030068 8 0x00\n\
                                devmem 0xA0030068 8 0x01\n\
                                devmem 0xA0030070 8 0x00\n\
                                devmem 0xA0030060 32 0x00000086\n\
                                devmem 0xA0030070 8 0x01\n\
                                devmem 0xA0030060 32 0x00000186\n\
                                devmem 0x41210008 32 0x100\n"
    
        id_pipe = subprocess.Popen(write_enable_sequence, shell=True, stdout=subprocess.PIPE)
        output, error = id_pipe.communicate()
        print(output.decode('utf8'))
    

        write_latch = int(read_status_register(), 16)

        if write_latch == 0x02:
            print("Write Enabled")
        else:
            print('Write Failed to Enable')

    else:
        print("Flash Write is Already Enabled")

def read_memory(base_address, offset_address, dummy_bytes):
    pass

def read_quad_out_memory(mem_address, num_bytes):

    
    address = int(mem_address, 16)
    '''
    hex_test = hex(test)

    print(test)
    print(hex_test)

    msb = (test & 0xFF0000) >> 16
    mid = (test & 0x00FF00) >> 8
    lsb = (test & 0x0000FF)
    
    print(hex(msb))
    print(hex(mid))
    print(hex(lsb))
    '''
    #do once..

    #num_bytes must always be 8 less than boundary.

    
    output_file = open("qspi_bitfile.bin", "wb")
    print_out = ""

    for x in range(0, 240, 16):

        read_mem_sequence = ""
        msb = (address & 0xFF0000) >> 16
        mid = (address & 0x00FF00) >> 8
        lsb = (address & 0x0000FF)

        print("Mem Address:  %s" % hex(address))
        print(hex(msb))
        print(hex(mid))
        print(hex(lsb))

        read_mem_sequence +="devmem 0x41210008 32 0x104\n\
                            devmem 0xA0030060 32 0x000001E6\n\
                            devmem 0xA0030068 8 0x6B\n\
                            devmem 0xA0030068 8 " + hex(msb) + "\n" + "devmem 0xA0030068 8 " + hex(mid) + "\n" + "devmem 0xA0030068 8 " + hex(lsb) + "\n"

        for x in range(0, 24):#add 16 dummy bytes.
            read_mem_sequence += "devmem 0xA0030068 8 0xAA\n"

        read_mem_sequence +="devmem 0xA0030070 8 0x00\n\
                            devmem 0xA0030060 32 0x00000086\n\
                            devmem 0xA0030070 8 0x01\n\
                            devmem 0xA0030060 32 0x00000186\n"
        
        for x in range(0, 24):#read 16 dummy bytes
            read_mem_sequence += "devmem 0xA003006C 8\n"

        
        id_pipe = subprocess.Popen(read_mem_sequence, shell=True, stdout=subprocess.PIPE)
        output, error = id_pipe.communicate()
        output = str(output)[39:]
        print_out += output
        
        address += 16

    #print(read_mem_sequence)
    print(print_out)
    #print(output.decode('utf8'))


    """
    read_mem_sequence ="devmem 0x41210008 32 0x104\n\
                        devmem 0xA0030060 32 0x000001E6\n\
                        devmem 0xA0030068 8 0x6B\n\
                        devmem 0xA0030068 8 " + hex(msb) + "\n" + "devmem 0xA0030068 8 " + hex(mid) + "\n" + "devmem 0xA0030068 8 " + hex(lsb) + "\n"

    print(read_mem_sequence)
    """

    """
    num_bytes = int(num_bytes, 10) + 8
    print(num_bytes)
    
    for x in range(0, num_bytes):
        read_mem_sequence += "devmem 0xA0030068 8 0xAA\n"

    read_mem_sequence +="devmem 0xA0030070 8 0x00\n\
                        devmem 0xA0030060 32 0x00000086\n\
                        devmem 0xA0030070 8 0x01\n\
                        devmem 0xA0030060 32 0x00000186\n"

    for x in range(0, num_bytes):
        read_mem_sequence += "devmem 0xA003006C 8\n"
    
    #print(read_mem_sequence)
    
    id_pipe = subprocess.Popen(read_mem_sequence, shell=True, stdout=subprocess.PIPE)
    output, error = id_pipe.communicate()

    output = str(output)[39:]


    
    print(output.decode('utf8'))
    """

def read_quad_io_memory():
    pass


def main():

    parser = argparse.ArgumentParser()

    parser.add_argument("-operation", "--operation", help="Function to perform, \
                        default = READ_ID, 0x90", default="READ_ID")


    parser.add_argument("-status_val", "--status_val", help="Status Register Value to Write, \
                        default = 0x00", default= 0x00)

    parser.add_argument("-config_val", "--config_val", help="Configuration Register Value to Write, \
                        default = 0x02 - Latency Code 00, Quad Mode Enabled", default= 0x02)
    
    parser.add_argument("-read_address", "--read_address", help="24 Bit Address to Start Reading Flash Memory, \
                        default = 0x000120", default= "0x000120")

    parser.add_argument("-num_bytes", "--num_bytes", help="Number of Bytes to Read From Flash Memory, \
                        default = 8", default="8")
    
    args = parser.parse_args()

    if args.operation not in look_up:
        print("Invalid operation.. quiting")
        exit(1)
    else:
        operation = look_up[args.operation]

    # Set up the echo command and direct the output to a pipe
    p1 = subprocess.Popen(['echo', args.operation], stdout=subprocess.PIPE)
    p2 = subprocess.Popen(['echo', str(operation)], stdout=subprocess.PIPE)

    # Run the command
    output = "String operation: " + p1.communicate()[0] + "Instruction Code: " + p2.communicate()[0]
    
    print(output)

    if args.operation == "READ_ID":
        read_id()
    elif args.operation == "WRITE_ENABLE":
        write_enable()
    elif args.operation == "WRITE_REG":
        
        write_registers(args.status_val, args.config_val)

    elif args.operation == "READ_STATUS":
        read_status_register()
    elif args.operation == "READ_CONFIG":
        read_config_register()
    elif args.operation == "READ_BAR":
        read_BAR_register()
    elif args.operation == "READ_QUAD_OUT":
        read_quad_out_memory(args.read_address, args.num_bytes)
    elif args.operation == "READ_QUAD_IO":
        read_quad_io_memory()
    else:
        pass


if __name__ == "__main__":
    main()
