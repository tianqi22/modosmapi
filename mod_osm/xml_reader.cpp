#include <iostream>

#include <boost/format.hpp>
#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>


#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/sax2/DefaultHandler.hpp>
#include <xercesc/sax2/Attributes.hpp>
#include <xercesc/sax2/XMLReaderFactory.hpp>

#include "osm_data.hpp"

std::string escapeChars( std::string toEscape )
{
    boost::algorithm::replace_all( toEscape, "&", "&amp;" );
    boost::algorithm::replace_all( toEscape, "<", "&lt;" );
    boost::algorithm::replace_all( toEscape, ">", "&gt;" );
    boost::algorithm::replace_all( toEscape, "'", "&apos;" );
    boost::algorithm::replace_all( toEscape, "\"", "&ltquot;" );

    return toEscape;
}

std::string transcodeString( const XMLCh *const toTranscode )
{
    char *fLocalForm = xercesc::XMLString::transcode( toTranscode );
    std::string theString( fLocalForm );
    xercesc::XMLString::release( &fLocalForm );

    return theString;
}


void extended_lexical_cast( const std::string &val, bool &var )
{
    std::string lowered( boost::algorithm::to_lower_copy( val ) );

    if ( lowered == "true" || lowered == "1" )
    {
        var = true;
    }
    else if ( lowered == "false" || lowered == "0" )
    {
        var = false;
    }
    else
    {
        throw boost::bad_lexical_cast();
    }
}

const boost::regex dateRegex( "(\\d{4})-(\\d{2})-(\\d{2})T(\\d{2}):(\\d{2}):(\\d{2})([\\+-]\\d{2}):(\\d{2})?" );

void extended_lexical_cast( const std::string &val, boost::posix_time::ptime &var )
{
    boost::smatch match;
    if ( boost::regex_match( val, match, dateRegex ) )
    {
         var = boost::posix_time::ptime(
             boost::gregorian::date(
                 boost::lexical_cast<int>( match[1] ),
                 boost::lexical_cast<int>( match[2] ),
                 boost::lexical_cast<int>( match[3] ) ),
             boost::posix_time::time_duration(
                 boost::lexical_cast<int>( match[4] ),
                 boost::lexical_cast<int>( match[5] ),
                 boost::lexical_cast<int>( match[6] ) ) );

         boost::posix_time::time_duration offset(
             boost::lexical_cast<int>( match[7] ),
             boost::lexical_cast<int>( match[8] ),
             0 );

         // TODO: Surely we should be adding this offset? But it doesn't seem to work...
         //var += offset;
    }
    else
    {
        throw boost::bad_lexical_cast();
    }
}


XercesInitWrapper::XercesInitWrapper()
{
    xercesc::XMLPlatformUtils::Initialize();

    m_parser = new xercesc::SAX2XMLReaderImpl();
}

XercesInitWrapper::~XercesInitWrapper()
{
    delete( m_parser );
    xercesc::XMLPlatformUtils::Terminate();
}

xercesc::SAX2XMLReaderImpl &XercesInitWrapper::getParser()
{
    return *m_parser;
}

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
    for ( size_t i = 0; i < attributes.getLength(); i++ )
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
        throw XmlParseException( "Cannot find build fn for: " + nodeName );
    }
        
    return findIt->second;
}

XMLReader::XMLReader( boost::shared_ptr<XMLNodeData> startNode )
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
    nodeBuildFn_t buildFn = m_buildStack.back()->getBuildFnFor( tagName );

    m_buildStack.push_back( boost::shared_ptr<XMLNodeData>( new XMLNodeData( attributes ) ) );
    buildFn( *(m_buildStack.back().get()) );
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
    //throw XMLParseException( "Unexpected characters in XML" );
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

