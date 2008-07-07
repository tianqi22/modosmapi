#ifndef EXCEPTIONS_HPP
#define EXCEPTIONS_HPP

#include <boost/shared_array.hpp>

namespace modosmapi
{

    class ModException : public std::exception
    {
        boost::shared_array<char> m_messageText;

    public:
        ModException( const std::string &message )
        {
            m_messageText.reset( new char[message.size() + 1] );
            memcpy( m_messageText.get(), message.c_str(), message.size() );
            m_messageText.get()[message.size()] = '\0';
        }
        virtual ~ModException() throw () {}
        virtual const char *what() const throw ()
        {
            return m_messageText.get();
        }
    };
}

#endif // EXCEPTIONS_HPP

