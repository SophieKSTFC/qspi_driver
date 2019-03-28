CC_dyn=arm-xilinx-linux-gnueabi-g++

qspi_driver: qspi_driver.cpp memory_mapped_device.cpp qspi_device.cpp 
	$(CC_dyn) --std=c++11 -D_GLIBCXX_USE_CXX11_ABI=0 -o qspi_driver qspi_driver.cpp memory_mapped_device.cpp qspi_device.cpp \
	 -I/aeg_sw/work/projects/fem-ii/target/boost_1.66.0/usr/include \
	 -L/aeg_sw/work/projects/fem-ii/target/boost_1.66.0/usr/lib -lboost_program_options -lboost_date_time
