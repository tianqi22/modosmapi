#include <iostream>

#include <boost/format.hpp>
#include <boost/algorithm/string/replace.hpp>

#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/sax2/DefaultHandler.hpp>
#include <xercesc/sax2/Attributes.hpp>
#include <xercesc/sax2/XMLReaderFactory.hpp>

#include <xercesc/parsers/SAX2XMLReaderImpl.hpp>

#include "data.hpp"

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


XMLMemberRegistration::XMLMemberRegistration( nodeBuildFnMap_t &nodeFnBuildMap ) : m_nodeFnBuildMap( nodeFnBuildMap )
{
}

XMLMemberRegistration &XMLMemberRegistration::operator()( const std::string &memberName, nodeBuildFn_t callBackFn )
{
    m_nodeFnBuildMap.insert( std::make_pair( memberName, callBackFn ) );

    return *this;
}

XMLNodeAttributeMap::XMLNodeAttributeMap( const xercesc::Attributes &attributes )
{
    for ( int i = 0; i < attributes.getLength(); i++ )
    {
        std::string key   = transcodeString( attributes.getLocalName( i ) );
        std::string value = transcodeString( attributes.getValue( i ) );

        m_attributes.insert( std::make_pair( key, value ) );
    }
}

XMLNodeData::XMLNodeData() : m_memberRegistration( m_nodeFnBuildMap )
{
}


XMLNodeData::XMLNodeData( const xercesc::Attributes &attributes ) :
    m_nodeAttributeMap( attributes ),
    m_memberRegistration( m_nodeFnBuildMap )
{
}

XMLNodeAttributeMap &XMLNodeData::readAttributes()
{
    return m_nodeAttributeMap;
}

XMLMemberRegistration &XMLNodeData::registerMembers()
{
    return m_memberRegistration;
}

nodeBuildFn_t XMLNodeData::getBuildFnFor( const std::string &nodeName )
{
    nodeBuildFnMap_t::iterator findIt = m_nodeFnBuildMap.find( nodeName );
        
    if ( findIt == m_nodeFnBuildMap.end() )
    {
        // Can't find the tag - make a nice error
        throw std::exception();
    }
        
    return findIt->second;
}

XMLReader::XMLReader( const XMLNodeData &startNode )
{
    m_buildStack.push_back( startNode );
}
    
void XMLReader::startElement(
    const XMLCh *const uri,
    const XMLCh *const localname,
    const XMLCh *const qame,
    const xercesc::Attributes &attributes )
{
    std::string tagName = transcodeString( localname );
    nodeBuildFn_t buildFn = m_buildStack.back().getBuildFnFor( tagName );

    m_buildStack.push_back( XMLNodeData( attributes ) );
    buildFn( m_buildStack.back() );
}

void XMLReader::endElement(
    const XMLCh *const uri,
    const XMLCh *const localname,
    const XMLCh *const qname )
{
    m_buildStack.pop_back();
}


void XMLReader::characters(
    const XMLCh *const chars,
    const unsigned int length )
{
    // Throw an exception to complain about the characters we didn't expect
    throw std::exception();
}

void XMLReader::warning( const xercesc::SAXParseException &e )
{
    int line = e.getLineNumber();
    std::string message( transcodeString( e.getMessage() ) );
    std::cerr << "XML parse warning (line " << line << ") " << message << std::endl;
}

void XMLReader::error( const xercesc::SAXParseException &e )
{
    int line = e.getLineNumber();
    std::string message( transcodeString( e.getMessage() ) );

    throw XmlParseException( boost::str( boost::format(
        "XML parse error (line: %d): %s" )
        % line % message ) );

}

void XMLReader::fatalError( const xercesc::SAXParseException &e )
{
     int line = e.getLineNumber();
     std::string message( transcodeString( e.getMessage() ) );

     throw XmlParseException( boost::str( boost::format(
         "XML parse error (line: %d): %s" )
         % line % message ) );
}

void buildOSMFragment( XMLNodeData &data )
{
    // TODO: Clearly this needs to go somewhere...
    boost::shared_ptr<OSMFragment> newFragment( new OSMFragment( data ) );
}

int main( int argc, char **argv )
{
    try
    {
        XercesInitWrapper x;

        xercesc::SAX2XMLReaderImpl &parser = x.getParser();

        XMLNodeData startNdData;

        startNdData.registerMembers()( "osm", &buildOSMFragment );

        XMLReader handler( startNdData );

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
