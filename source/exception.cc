#include "exception.hh"
#include <sstream>

namespace protop {

exception::exception( const std::string &message, int line, int column ) :
    line(line), column(column), message(message)
{
    std::stringstream ss;
    ss << message << " (" << line << ':' << column << ')';
    this->message = ss.str();
}

exception::~exception()
{
}

const char *exception::what() const throw()
{
    return message.c_str();
}

const std::string exception::cause() const
{
    return message.c_str();
}

} // protop
