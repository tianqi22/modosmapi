#include <iostream>

#include <boost/bind.hpp>
#include <boost/format.hpp>
#include <boost/shared_ptr.hpp>

#include "osm_data.hpp"
#include "xml_reader.hpp"


OSMNode::OSMNode( XMLNodeData &data )
{
    std::cout << "Node constructor" << std::endl;
    data.readAttributes()
        ( "id", m_id )
        ( "lat", m_lat )
        ( "lon", m_lon )
        ( "timestamp", m_timestamp )
        ( "user", m_user )
        ( "uid", m_userId );

    std::cout << m_id << ", " << m_lat << ", " << m_lon << ", " << m_timestamp << ", " << m_userId << std::endl;

    data.registerMembers()("tag", boost::bind( &OSMNode::readTag, this, _1 ) );
}

void OSMNode::readTag( XMLNodeData &data )
{
    string_t k, v;
    data.readAttributes()( "k", k )( "v", v );

    m_tags.push_back( tag_t( k, v ) );
}

OSMWay::OSMWay( XMLNodeData &data )
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

void OSMWay::readTag( XMLNodeData &data )
{
    string_t k, v;
    data.readAttributes()( "k", k )( "v", v );
    m_tags.push_back( tag_t( k, v ) );
}

void OSMWay::readNd( XMLNodeData &data )
{
    dbId_t ndId;
    data.readAttributes()( "ref", ndId );
    m_nodes.push_back( ndId );
}

OSMRelation::OSMRelation( XMLNodeData &data )
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
    m_tags.push_back( theTag );
}


void OSMFragment::build( XMLNodeData &data )
{
    data.readAttributes()
        ( "version", m_version )
        ( "generator", m_generator );

    std::cout << "OSM fragment: " << m_version << ", " << m_generator << std::endl;

    data.registerMembers()
        ( "node", boost::bind( &OSMFragment::readNode, this, _1 ) )
        ( "way", boost::bind( &OSMFragment::readWay, this, _1 ) )
        ( "relation", boost::bind( &OSMFragment::readRelation, this, _1 ) );
}

void OSMFragment::readNode( XMLNodeData &data )
{
    boost::shared_ptr<OSMNode> newNode( new OSMNode( data ) );
    m_nodes.push_back( newNode );
}

void OSMFragment::readWay( XMLNodeData &data )
{
    boost::shared_ptr<OSMWay> newWay( new OSMWay( data ) );
    m_ways.push_back( newWay );
}

void OSMFragment::readRelation( XMLNodeData &data )
{
    boost::shared_ptr<OSMRelation> newRelation( new OSMRelation( data ) );
    m_relations.push_back( newRelation );
}

