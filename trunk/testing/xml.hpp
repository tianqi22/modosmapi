

std::string escapeChars( std::string toEscape );
std::string transcodeString( const XMLCh *const toTranscode );

class XmlParseException : public std::exception
{
    std::string m_message;

public:
    XmlParseException( const std::string &message ) : m_message( message ) { }
    virtual ~XmlParseException() throw () { }
};

