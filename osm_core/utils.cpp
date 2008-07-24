#include <cmath>
#include <iostream>

#include <utils.hpp>

const double PI = acos( -1.0 );

// Should return the distance in km between the two points
// Ref: http://jan.ucc.nau.edu/~cvm/latlon_formula.html
double distBetween( double lat1, double lon1, double lat2, double lon2 )
{
    using namespace std;

    double a1 = (lat1 / 180.0) * PI;
    double b1 = (lon1 / 180.0) * PI;
    double a2 = (lat2 / 180.0) * PI;
    double b2 = (lon2 / 180.0) * PI;

    double dist = 6378.0 * acos(cos(a1)*cos(b1)*cos(a2)*cos(b2) + cos(a1)*sin(b1)*cos(a2)*sin(b2) + sin(a1)*sin(a2));

    return dist;
}

std::vector<std::string>    ConstTagString::m_theStrings;
size_t                      ConstTagString::m_lastIndex = 0;
ConstTagString::stringMap_t ConstTagString::m_stringIndexMap;

ConstTagString::ConstTagString( const std::string &str )
{
    assignString( str );
}

ConstTagString::ConstTagString( const ConstTagString &rhs )
{
    m_stringIndex = rhs.m_stringIndex;
}

ConstTagString &ConstTagString::operator=( const ConstTagString &rhs )
{
    m_stringIndex = rhs.m_stringIndex;
    return *this;
}

void ConstTagString::assignString( const std::string &str )
{
    stringMap_t::iterator findIt = m_stringIndexMap.find( str );
    
    if ( findIt == m_stringIndexMap.end() )
    {
        m_theStrings.push_back( str );
        m_stringIndex = m_lastIndex++;
        m_stringIndexMap.insert( std::make_pair( str, m_stringIndex ) );            
    }
    else
    {
        m_stringIndex = findIt->second;
    }
}

bool ConstTagString::operator==( const ConstTagString &rhs )
{
    return m_stringIndex == rhs.m_stringIndex;
}

bool ConstTagString::operator<( const ConstTagString &rhs )
{
    // NOTE: operator< does not give lexicographic ordering
    return m_stringIndex < rhs.m_stringIndex;
}



