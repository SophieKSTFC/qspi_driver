
#include <exception>
#include <string>

class MemException : public std::exception
{
public:

    // Blank Fem2ControlMsgException constructor
    MemException(void) throw() :
        error_msg("")
    {};

    // Fem2ControlMsgException with error message
    MemException(const std::string the_error_msg) throw() :
        error_msg(the_error_msg)
    {};

    // @override returns the error message as a c string
    virtual const char* what(void) const throw()
    {
        return error_msg.c_str();
    };

    // Destructor
    ~MemException(void) throw() {};

private:

    const std::string error_msg;  // The error message

};// fem2exception
