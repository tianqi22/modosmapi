#include <iostream>

#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/sax2/DefaultHandler.hpp>
#include <xercesc/sax2/Attributes.hpp>
#include <xercesc/sax2/XMLReaderFactory.hpp>

XERCES_CPP_NAMESPACE_USE

class DummySaxHandler : public HandlerBase
{
public:
    void startElement( const XMLCh *const name, AttributeList &attributes )
    {
    }

    void endElement( )
    {
    }

    void characters( const XMLCh *const chars, const unsigned int length )
    {
    }
};

std::string escapeChars( std::string toEscape )
{
    boost::algorithm::replace_all( toEscape, "&", "&amp;" );
    boost::algorithm::replace_all( toEscape, "<", "&lt;" );
    boost::algorithm::replace_all( toEscape, ">", "&gt;" );
    boost::algorithm::replace_all( toEscape, "'", "&apos;" );
    boost::algorithm::replace_all( toEscape, "\"", "&ltquot;" );
}

std::string transcodeString( const XMLCh *const toTranscode )
{
    char *fLocalForm = xercesc::XMLString::transcode( toTranscode );
    std::string theString( fLocalForm );
    xeresc::XMLString::release( &fLocalForm );

    return theString;
}

class XercesInitWrapper
{
    xercesc::Sax2XMLReaderImpl *m_parser;

public:
    XercesInitWrapper()
    {
        XMLPlatformUtils::Initialize();

        m_parser = new xercesc::Sax2XMLReaderImpl();
    }

    ~XercesInitWrapper()
    {
        delete( m_parser );
        XMLPlatformUtils::Terminate();
    }

    xercesc::Sax2XMLReaderImpl &getParser()
    {
        return *m_parser;
    }
};


int main( int argc, char **argv )
{
    try
    {
        XercesInitWrapper x;

        xercesc::Sax2XMLReaderImpl &parser = x.getParser();

    }
    catch ( const XMLException &toCatch )
    {
        std::cerr << "Exception thrown in Xerces initialise" << std::endl;
        return 1;
    }

    return 0;
}
