CC_dyn=arm-xilinx-linux-gnueabi-g++

qspi_driver: flash_interface.cpp qspi_memory.cpp MemException.cpp
	$(CC_dyn) -std=c++11 -o qspi_driver flash_interface.cpp qspi_memory.cpp MemException.cpp -I/u/dbu19518/develop/projects/xilinx/target/usr/include -L/u/dbu19518/develop/projects/xilinx/target/usr/lib 
