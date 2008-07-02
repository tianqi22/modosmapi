#ifndef _IOXML_H_
#define _IOXML_H_

#include <iomanip>
#include <sstream>
#include <vector>
#include <boost/algorithm/string/join.hpp>
#include <boost/tuple/tuple.hpp>

//
// manipulators for printing xml
//

namespace xml
{


// Format state for printing xml
struct FormatState
{
    int indentLevel, indentSize;
    char indentChar;
    static FormatState *streamState (std::ios_base &os);
private:
    FormatState () : indentLevel (0), indentSize (2), indentChar (' ') {}
    static void cleanup (std::ios_base::event e, std::ios_base &io, int);
};

// Xml format manipulators 
inline std::ostream& indent (std::ostream &os)
{
    FormatState *state = FormatState::streamState (os);
    return (os << std::string (state->indentLevel * state->indentSize,
                                state->indentChar));
}

inline std::ostream& inc (std::ostream &os)
{
    ++(FormatState::streamState (os)->indentLevel);
    return os;
}

inline std::ostream& dec (std::ostream &os)
{
    --(FormatState::streamState (os)->indentLevel);
    return os;
}

struct XmlFormatArgs {int indentSize; char indentChar;};
inline XmlFormatArgs setformat (int indentSize, char indentChar)
{
    XmlFormatArgs args = {indentSize, indentChar};
    return args;
}

inline std::ostream& operator << (std::ostream &os, XmlFormatArgs args)
{
    FormatState *state = FormatState::streamState (os);
    state->indentSize = args.indentSize;
    state->indentChar = args.indentChar;
    return os;
}

std::string escape (const std::string &input);

template <typename T> std::string quoteattr (T input);

// Escape '&', '<', '>' and '"' in a string. Enclose the string in '"'
template <> std::string quoteattr<std::string> (std::string input);

template <typename T> std::string quoteattr (T input)
{
    std::stringstream ss;
    ss << std::setprecision( 10 ) << input;
    return quoteattr (ss.str ());
}

namespace
{
template<typename T>
void renderTags (const boost::tuples::cons<T, boost::tuples::null_type> &tuple,
                 const std::string attrNames[],
                 std::stringstream &os,
                 int index )
{
    os << attrNames[index] << tuple.head;
    //tags.push_back (attrNames[index] + "=" + quoteattr (tuple.head));
}

template<typename T1, typename T2>
void renderTags (const boost::tuples::cons<T1, T2> &tuple,
                 const std::string attrNames[],
                 std::stringstream &os,
                 int index )
{
    os << attrNames[index] << "=" << tuple.head << " ";
    //tags.push_back (attrNames[index] + "=" + quoteattr (tuple.head));
    renderTags (tuple.tail, attrNames, os, index + 1);
}
} // end anonymous namespace

template<typename T1, typename T2>
std::string attrs (const std::string attrNames[], const boost::tuples::cons<T1, T2> &attrValueTuple)
{
    std::stringstream ss;

    renderTags (attrValueTuple, attrNames, ss, 0);

    return ss.str();
}

} // end namespace xml

#endif // _IOXML_H_
