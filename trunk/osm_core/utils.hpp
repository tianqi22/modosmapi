#ifndef UTILS_HPP
#define UTILS_HPP

#include <map>
#include <vector>
#include <string>

#include <boost/operators.hpp>

extern const double PI;
double distBetween( double, double, double, double );


class ConstTagString :
    boost::less_than_comparable<ConstTagString,
    boost::equality_comparable<ConstTagString> >
{
private:
    typedef std::map<std::string, size_t> stringMap_t;

    static std::vector<std::string> m_theStrings;
    static size_t m_lastIndex;
    static stringMap_t m_stringIndexMap;
    
    size_t m_stringIndex;

public:
    ConstTagString();
    ConstTagString( const char *str );
    ConstTagString( const std::string &str );
    ConstTagString( const ConstTagString &rhs );
    ConstTagString &operator=( const ConstTagString &rhs );
    bool operator==( const ConstTagString &rhs ) const;
    bool operator<( const ConstTagString &rhs ) const;

    std::string toString() const;

    static size_t numStrings() { return m_stringIndexMap.size(); }

private:
    void assignString( const std::string &str );
};

std::ostream &operator<<( std::ostream &s, const ConstTagString &val );

#endif // UTILS_HPP




