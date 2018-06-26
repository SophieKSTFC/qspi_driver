/*
*   qspi_flash_defines.h
*   @Author Sophie Kirkham STFC, 2018
*   Header file for constant definitions used by the ..
*   QSPI Controller/Flash Memory
*/

#ifndef QSPI_FLASH_DEFINES_H_
#define QSPI_FLASH_DEFINES_H_

#include <iostream>

//Multiplexer Definitions
#define MUX_BASE 0x41210000 // Multiplexer (MUX) base address
#define MUX_OFFSET 0x08     // Offset address to change the flash device 
#define MUX_SET_FL1 0x104   // Data value sent to MUX to select flash chip 1
#define MUX_SET_FL2 0x105   // Data value sent to MUX to select flash chip 2
#define MUX_SET_FL3 0x106   // Data value sent to MUX to select flash chip 3
#define MUX_SET_FL4 0x107   // Data value sent to MUX to select flash chip 4
#define MUX_DESET 0x100     // Data value sent to MUX to deselect any flash chip
#define MUX_WIDTH 32        // The data width used in transactions with the MUX

//QSPI Controller Definitions
#define QSPI_CR_WIDTH 32    // The data width used in QSPI Control Reg transactions
#define QSPI_STD_WIDTH 8    // The data width used in all other QSPI transactions
#define QSPI_BASE 0xA0030000// The base address of the QSPI controller
#define QSPI_CONFIG_R 0x60  // The offset address of the QSPI Config Reg
#define QSPI_STATUS_R 0x64  // The offset address of the QSPI Status Reg
#define QSPI_DTR 0x68       // The offset address of the QSPI data transmit Reg
#define QSPI_DRR 0x6C       // The offset address of the QSPI data receive reg
#define QSPI_SSR 0x70       // The offset address of the QSPI Slave Select reg

#define RESET_FIFO_MSTR_CONFIG_ENABLE 0x000001E6    // Value sent to reset FIFO and enable master transaction.    
#define ENABLE_MASTER_TRAN 0x00000086   // Value sent to enable master transaction
#define DISABLE_MASTER_TRAN 0x00000186  // Value sent to disable master transaction
#define CHIP_SELECT 0x00    // Value sent to issue chip select command.        
#define CHIP_DESELECT 0x01  // Value sent to issue chip deselect command.

//Flash Memory Instruction Codes
#define FL_READ_ID 0x90     // Instruction code to read flash memory device ID
#define FL_WRITE_ENABLE 0x06// Instruction code to enable write on the flash
#define FL_WRITE_REG 0x01   // Instruction code to write to the flash registers
#define FL_READ_STATUS 0x05 // Instruction code to read the flash status register
#define FL_READ_CONFIG 0x35 // Instruction code to read the flash config register
#define FL_READ_BAR 0x16    // Instruction code to read the bank address register
#define FL_READ_QUAD_OUT 0x6C// Instruction code to read flash memory array in QUAD mode.
#define FL_READ_QUAD_IO 0xEB// Instruction code to read flash memory array in QUADO I/O mode.
#define FL_QUAD_PP 0x34     // Instruction code to program the flash memory array in QUAD mode
#define FL_BULK_ERASE 0x60  // Instruction code to erase the entire flash memory array.

//Helper Definitions
#define DUMMY_DATA 0xDD     // Dummy data byte value.
#define FL_MEM_START 0x00000000 // Start address of the flash memory array
#define FL_MEM_SIZE 28734812// Current configuration bin file size.
#define FIFO_DEPTH 128      // Max number of bytes to issue into the FIFO
#define PREAMBLE_SIZE 9     // Size of the preamble generated using single transaction reads.
#define DEFAULT_FLASH 1     // Default flash to select.
#define SIXTY_FOUR_MB 64000000  // 64MB 
#define PAGE_SIZE 512
#define MAX_FLASH_ADDRESS (SIXTY_FOUR_MB - 16) // Maximum safe flash address to begin a read from.
const std::string BIN_EXT = ".bin";

#endif