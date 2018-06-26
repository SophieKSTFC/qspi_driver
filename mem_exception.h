#ifndef MEM_EXCEPTION_H_
#define MEM_EXCEPTION_H_
/*
*   mem_exception.h
*   @Author Sophie Kirkham STFC, 2018 
*   Exception class implementation for memory mapped devices
*   Extends std::exception
*/

#include <exception>
#include <string>


class mem_exception : public std::exception{
    
    public:

        // Blank mem_exception constructor
        mem_exception(void) throw() :
            error_msg("")
        {};

        /*  mem_exception constructor with error message
        *   @param the_error_msg : the error message to throw
        */
        mem_exception(const std::string the_error_msg) throw() :
            error_msg(the_error_msg)
        {};

        
        /*  @override implements what() from std::exception
        *   @return : the error message as a c string
        */
        virtual const char* what(void) const throw(){
            return error_msg.c_str();
        };

        // Destructor
        ~mem_exception(void) throw() {};

    private:

        const std::string error_msg;  // The error message

};

#endif