#include <iostream>

#include <boost/bind.hpp>
#include <boost/format.hpp>
#include <boost/shared_ptr.hpp>

#include "osm_data.hpp"
#include "xml_reader.hpp"


OSMNode::OSMNode( OSMFragment &frag, XMLNodeData &data )
{
    data.readAttributes()
        ( "id", m_id )
        ( "lat", m_lat )
        ( "lon", m_lon )
        ( "timestamp", m_timestamp )
        ( "user", m_user, true, std::string( "none" ) )
        ( "uid", m_userId, true, dbId_t( 0 ) );

    if ( m_user != "none" && m_userId != 0 )
    {
        frag.addUser( m_userId, m_user );
    }
        

    data.registerMembers()("tag", boost::bind( &OSMNode::readTag, this, _1 ) );
}

void OSMNode::readTag( XMLNodeData &data )
{
    string_t k, v;
    data.readAttributes()( "k", k )( "v", v );

    m_tags.insert( tag_t( k, v ) );
}

OSMWay::OSMWay( OSMFragment &frag, XMLNodeData &data )
{
    data.readAttributes()
        ( "id", m_id )
        ( "timestamp", m_timestamp )
        //( "visible", m_visible )
        ( "user", m_user, true, std::string( "none" ) )
        ( "uid", m_userId, true, dbId_t( 0 ) );

    if ( m_user != "none" && m_userId != 0 )
    {
        frag.addUser( m_userId, m_user );
    }

    data.registerMembers()
        ( "nd", boost::bind( &OSMWay::readNd, this, _1 ) )
        ( "tag", boost::bind( &OSMWay::readTag, this, _1 ) );
}

void OSMWay::readTag( XMLNodeData &data )
{
    string_t k, v;
    data.readAttributes()( "k", k )( "v", v );
    m_tags.insert( tag_t( k, v ) );
}

void OSMWay::readNd( XMLNodeData &data )
{
    dbId_t ndId;
    data.readAttributes()( "ref", ndId );
    m_nodes.push_back( ndId );
}

OSMRelation::OSMRelation( OSMFragment &frag, XMLNodeData &data )
{
    data.readAttributes()
        ( "id", m_id )
        ( "timestamp", m_timestamp )
        ( "user", m_user, true, std::string( "none" ) )
        ( "uid", m_userId, true, dbId_t( 0 ) );

    if ( m_user != "none" && m_userId != 0 )
    {
        frag.addUser( m_userId, m_user );
    }

    data.registerMembers()
        ( "member", boost::bind( &OSMRelation::readMember, this, _1 ) )
        ( "tag", boost::bind( &OSMRelation::readTag, this, _1 ) );
}

void OSMRelation::readMember( XMLNodeData &data )
{
    member_t theMember;
    data.readAttributes()
        ( "type", theMember.get<0>() )
        ( "ref", theMember.get<1>() )
        ( "role", theMember.get<2>() );
    m_members.push_back( theMember );
}

void OSMRelation::readTag( XMLNodeData &data )
{
    tag_t theTag;
    data.readAttributes()
        ( "k", theTag.first )
        ( "v", theTag.second );
    m_tags.insert( theTag );
}


void OSMFragment::build( XMLNodeData &data )
{
    data.readAttributes()
        ( "version", m_version )
        ( "generator", m_generator );

    data.registerMembers()
        ( "node", boost::bind( &OSMFragment::readNode, this, _1 ) )
        ( "way", boost::bind( &OSMFragment::readWay, this, _1 ) )
        ( "relation", boost::bind( &OSMFragment::readRelation, this, _1 ) );
}

void OSMFragment::readNode( XMLNodeData &data )
{
    boost::shared_ptr<OSMNode> newNode( new OSMNode( *this, data ) );
    m_nodes.insert( std::make_pair( newNode->getId(), newNode ) );
}

void OSMFragment::readWay( XMLNodeData &data )
{
    boost::shared_ptr<OSMWay> newWay( new OSMWay( *this, data ) );
    m_ways.insert( std::make_pair( newWay->getId(), newWay ) );
}

void OSMFragment::readRelation( XMLNodeData &data )
{
    boost::shared_ptr<OSMRelation> newRelation( new OSMRelation( *this, data ) );
    m_relations.insert( std::make_pair( newRelation->getId(), newRelation ) );
}

void OSMFragment::addUser( dbId_t userId, const std::string &userName )
{
    if ( m_userDetails.find( userId ) == m_userDetails.end() )
    {
        m_userDetails.insert( std::make_pair( userId, userName ) );
    }
}


