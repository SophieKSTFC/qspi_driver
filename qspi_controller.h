#ifndef QSPI_CONRTOLLER_H_
#define QSPI_CONRTOLLER_H_

#include "memory_mapped_device.h"

class qspi_controller : public memory_mapped_device{

    public:

        qspi_controller() : memory_mapped_device(){};
        qspi_controller(uint32_t base) : memory_mapped_device(base){};
        ~qspi_controller(){};

};

#endif