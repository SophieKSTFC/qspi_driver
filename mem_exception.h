#ifndef MEM_EXCEPTION_H_
#define MEM_EXCEPTION_H_
/*
*   Exception class implementation for memory mapped devices
*
*/
#include <exception>
#include <string>

class mem_exception : public std::exception
{
public:

    // Blank mem_exception constructor
    mem_exception(void) throw() :
        error_msg("")
    {};

    // mem_exception with error message
    mem_exception(const std::string the_error_msg) throw() :
        error_msg(the_error_msg)
    {};

    // @override returns the error message as a c string
    virtual const char* what(void) const throw()
    {
        return error_msg.c_str();
    };

    // Destructor
    ~mem_exception(void) throw() {};

private:

    const std::string error_msg;  // The error message

};// mem_exception

#endif