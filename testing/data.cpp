#include <iostream>

#include <boost/bind.hpp>
#include <boost/format.hpp>


#include "data.hpp"
#include "xml.hpp"

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

class OSMNode
{
private:
    id_t               m_id;
    boost::int64_t     m_lat;
    boost::int64_t     m_lon;
    string_t           m_timestamp;
    string_t           m_user;
    id_t               m_userId;

    std::vector<tag_t> m_tags;

public:
    void readTag( XMLNodeData &data )
    {
        string_t k, v;
        data.readAttributes()( "k", k )( "v", v );

        m_tags.push_back( tag_t( k, v ) );
    }

    void readNode( XMLNodeData &data )
    {
        data.readAttributes()
            ( "id", m_id )
            ( "lat", m_lat )
            ( "lon", m_lon )
            ( "timestamp", m_timestamp )
            ( "user", m_user )
            ( "uid", m_userId );

        data.registerMembers()("tag", boost::bind( &OSMNode::readTag, this, _1 ) );
    }
};

class OSMWay
{
private:
    id_t               m_id;
    string_t           m_timestamp;
    string_t           m_user;
    id_t               m_userId;

    std::vector<id_t> m_nodes;
    std::vector<tag_t> m_tags;

public:
    void readTag( XMLNodeData &data )
    {
        string_t k, v;
        data.readAttributes()( "k", k )( "v", v );
        m_tags.push_back( tag_t( k, v ) );
    }

    void readNd( XMLNodeData &data )
    {
        dbId_t ndId;
        data.readAttributes()( "ref", ndId );
        m_nodes.push_back( ndId );
    }

    void readWay( XMLNodeData &data )
    {
        data.readAttributes()
            ( "id", m_id )
            ( "timestamp", m_timestamp )
            ( "user", m_user )
            ( "uid", m_userId );

        data.registerMembers()
            ( "nd", boost::bind( &OSMWay::readNd, this, _1 ) )
            ( "tag", boost::bind( &OSMWay::readTag, this, _1 ) );
    }
};

class OSMRelation
{
private:
    id_t m_id;
    string_t m_timestamp;
    string_t m_user;
    id_t m_userId;

    std::vector<tag_t> m_tags;
    std::vector<member_t> m_members;

public:
    void readMember( XMLNodeData &data )
    {
        member_t theMember;
        data.readAttributes()
            ( "type", theMember.get<0>() )
            ( "ref", theMember.get<1>() )
            ( "role", theMember.get<2>() );
        m_members.push_back( theMember );
    }

    void readTag( XMLNodeData &data )
    {
        tag_t theTag;
        data.readAttributes()
            ( "k", theTag.first )
            ( "v", theTag.second );
        m_tags.push_back( theTag );
    }

    void readRelation( XMLNodeData &data )
    {
        data.readAttributes()
            ( "id", m_id )
            ( "timestamp", m_timestamp )
            ( "user", m_user )
            ( "uid", m_userId );

        data.registerMembers()
            ( "member", boost::bind( &OSMRelation::readMember, this, _1 ) )
            ( "tag", boost::bind( &OSMRelation::readTag, this, _1 ) );
    }
};
