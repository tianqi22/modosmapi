#ifndef EQUALITY_TESTER_HPP
#define EQUALITY_TESTER_HPP

#include <string>

class EqualityTester
{
private:
    std::string m_context;

public:
    EqualityTester( const std::string &context ) : m_context( context ) {}

    EqualityTester context( const std::string &nested_context ) const
    {
        return EqualityTester( m_context + ":" + nested_context );
    }

    template<typename T1, typename T2>
    void requireEqual( const T1 &val1, const T2 &val2, const std::string &message ) const
    {
        if ( val1 != val2 )
        {
            std::cerr << "<" << m_context << "> " << message << " (" << val1 << ", " << val2 << ")" << std::endl;
        }
    }

    void error( const std::string &message ) const
    {
        std::cerr << "<" << m_context << "> " << message << std::endl;
    }
};

#endif // EQUALITY_TESTER_HPP

