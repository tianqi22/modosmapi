#include <map>
#include <vector>
#include <string>

extern const double PI;
double distBetween( double, double, double, double );


class ConstTagString
{
private:
    typedef std::map<std::string, size_t> stringMap_t;

    static std::vector<std::string> m_theStrings;
    static size_t m_lastIndex;
    static stringMap_t m_stringIndexMap;
    
    size_t m_stringIndex;

public:
    ConstTagString( const std::string &str );
    ConstTagString( const ConstTagString &rhs );
    ConstTagString &operator=( const ConstTagString &rhs );
    bool operator==( const ConstTagString &rhs );
    bool operator<( const ConstTagString &rhs );

private:
    void assignString( const std::string &str );
};




