/*
*   qspi_constroller.h
*   @Author Sophie Kirkham STFC, 2018
*   Sub-class implementation of memory_mapped_device for FEM-II QSPI Controller
*   Extends memory_mapped_device
*/

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