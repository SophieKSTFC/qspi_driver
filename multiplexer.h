/*
*   multiplexer.h
*   @Author Sophie Kirkham STFC, 2018
*   Sub-class implementation of memory_mapped_device for FEM-II Multiplexer
*   Extends memory_mapped_device
*/

#ifndef MULTIPLEXER_H_
#define MULTIPLEXER_H_

#include "memory_mapped_device.h"

class multiplexer : public memory_mapped_device{

    public:

        multiplexer() : memory_mapped_device(){};
        multiplexer(uint32_t base) : memory_mapped_device(base){};
        ~multiplexer(){};

};

#endif