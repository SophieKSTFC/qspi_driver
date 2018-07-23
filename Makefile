CC_dyn=arm-xilinx-linux-gnueabi-g++

qspi_driver: qspi_driver.cpp memory_mapped_device.cpp qspi_device.cpp 
	$(CC_dyn) -std=c++11 -o qspi_driver qspi_driver.cpp memory_mapped_device.cpp qspi_device.cpp -I/u/dbu19518/develop/projects/xilinx/target/usr/include -L/u/dbu19518/develop/projects/xilinx/target/usr/lib -lboost_program_options -lboost_date_time
