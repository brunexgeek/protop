#ifndef PROTOP_EXCEPTION
#define PROTOP_

#include <string>
#include <exception>

namespace protop {

class exception : public std::exception
{
    public:
        int line, column;

        exception( const std::string &message, int line = 1, int column = 1 );
        virtual ~exception();
        const char *what() const throw();
        const std::string cause() const;
    private:
        std::string message;
};

} // protop

#endif // PROTOP_EXCEPTION
