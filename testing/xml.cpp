#include <iostream>

#include <boost/format.hpp>
#include <boost/algorithm/string/replace.hpp>

#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/sax2/DefaultHandler.hpp>
#include <xercesc/sax2/Attributes.hpp>
#include <xercesc/sax2/XMLReaderFactory.hpp>

#include <xercesc/parsers/SAX2XMLReaderImpl.hpp>

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
    xercesc::XMLString::release( &fLocalForm );

    return theString;
}

class XmlParseException : public std::exception
{
    std::string m_message;

public:
    XmlParseException( const std::string &message ) : m_message( message )
    {
    }

    virtual ~XmlParseException() throw ()
    {
    }
};

class XercesInitWrapper
{
    xercesc::SAX2XMLReaderImpl *m_parser;

public:
    XercesInitWrapper()
    {
        xercesc::XMLPlatformUtils::Initialize();

        m_parser = new xercesc::SAX2XMLReaderImpl();
    }

    ~XercesInitWrapper()
    {
        delete( m_parser );
        xercesc::XMLPlatformUtils::Terminate();
    }

    xercesc::SAX2XMLReaderImpl &getParser()
    {
        return *m_parser;
    }
};

class DummySaxHandler : public xercesc::DefaultHandler
{
public:
    void startElement(
        const XMLCh *const uri,
        const XMLCh *const localname,
        const XMLCh *const qame,
        const xercesc::Attributes &attributes )
    {
        std::cout << "Start: " << transcodeString( localname ) << std::endl;
    }

    void endElement(
        const XMLCh *const uri,
        const XMLCh *const localname,
        const XMLCh *const qame )
    {
        std::cout << "End: " << transcodeString( localname ) << std::endl;
    }

    void characters(
        const XMLCh *const chars,
        const unsigned int length )
    {
    }

    void warning( const xercesc::SAXParseException &e )
    {
        int line = e.getLineNumber();
        std::string message( transcodeString( e.getMessage() ) );
        std::cerr << "XML parse warning (line " << line << ") " << message << std::endl;
    }

    void error( const xercesc::SAXParseException &e )
    {
        int line = e.getLineNumber();
        std::string message( transcodeString( e.getMessage() ) );

        throw XmlParseException( boost::str( boost::format(
            "XML parse error (line: %d): %s" )
            % line % message ) );

    }

    void fatalError( const xercesc::SAXParseException &e )
    {
        int line = e.getLineNumber();
        std::string message( transcodeString( e.getMessage() ) );

        throw XmlParseException( boost::str( boost::format(
            "XML parse error (line: %d): %s" )
            % line % message ) );
    }
};



int main( int argc, char **argv )
{
    try
    {
        XercesInitWrapper x;

        xercesc::SAX2XMLReaderImpl &parser = x.getParser();

        DummySaxHandler handler;

        parser.setContentHandler( &handler );
        parser.setErrorHandler( &handler );
        
        parser.parse( "/home/alex/devel/osm/data/oxford-cotswolds-080514.osm" );

    }
    catch ( const xercesc::XMLException &toCatch )
    {
        std::cerr << "Exception thrown in XML parse" << std::endl;
        return -1;
    }
    catch ( const std::exception &e )
    {
        std::cerr << "Exception thrown in XML parse: " << e.what() << std::endl;
        return -1;
    }

    return 0;
}
