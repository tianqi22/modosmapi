#include "ioxml.h"
#include <ios>

namespace xml
{


FormatState *FormatState::streamState (std::ios_base &os)
{
    static int index = std::ios_base::xalloc ();
    void *&p = os.pword (index);
    if (p == NULL)
    {
        p = new FormatState;
        os.register_callback (cleanup, 0);
    }
    return static_cast<FormatState *> (p);
}

void FormatState::cleanup (std::ios_base::event e, std::ios_base &io, int)
{
    try
    {
        if (e == std::ios_base::erase_event)
            delete streamState (io);
    }
    catch (std::ios::failure &e)
    {
        // C++ Standard 27.4.2.6 states that ios_base callbacks must not throw.
    }
}

std::string escape (const std::string &input)
{
    // Escape '&', '<' and '>' in a string
    std::string output;
    output.reserve (input.size ());
    for (std::string::size_type i = 0, end = input.size (); i < end; ++i)
    {
        switch (input [i])
        {
            case '&':
                output += "&amp;";
                break;
            case '<':
                output += "&lt;";
                break;
            case '>':
                output += "&gt;";
                break;
            default:
                output += input [i];
                break;
        }
    }
    return output;
}

template <>
std::string quoteattr<std::string> (std::string input)
{
    // Escape '&', '<', '>' and '"' in a string. Enclose the string in '"'
    input = escape (input);
    std::string output;
    output.reserve (input.size () + 2);
    output += '"';
    for (std::string::size_type i = 0, end = input.size (); i < end; ++i)
    {
        if (input [i] == '"')
            output += "&quot;";
        else
            output += input [i];
    }
    output += '"';
    return output;
}

} // end namespace xml
