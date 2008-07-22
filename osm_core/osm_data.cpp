#include <iostream>

#include <boost/bind.hpp>
#include <boost/format.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/foreach.hpp>

#include "osm_data.hpp"
#include "xml_reader.hpp"

const static double minLat = -180.0;
const static double maxLat = +180.0;
const static double minLon = -90.0;
const static double maxLon = +90.0;


// Cache tile filenames of form tile_<lat>_<lon>.cache
// If modified in memory - need to rewrite the whole
//
// Put ways (and tags) in tile of first member node
// Put relations (and member lists) in tile of first member (node or way)

// Do we need a single cache map of node locations? Perhaps...

const static size_t cacheTileDivisions = 128;

void OSMBase::readBaseData( OSMFragment &frag, XMLNodeData &data )
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

}

OSMNode::OSMNode( OSMFragment &frag, XMLNodeData &data )
{
    readBaseData( frag, data );

    data.readAttributes()
        ( "lat", m_lat )
        ( "lon", m_lon );

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
    readBaseData( frag, data );

    data.readAttributes()
        ( "visible", m_visible, true, true );

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
    readBaseData( frag, data );

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
    m_members.insert( theMember );
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
        ( "relation", boost::bind( &OSMFragment::readRelation, this, _1 ) )
        ( "bounds", boost::bind( &OSMFragment::readBounds, this, _1 ) );
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

void OSMFragment::readBounds( XMLNodeData &data )
{
    // Ignore for now
}

void OSMFragment::addUser( dbId_t userId, const std::string &userName )
{
    if ( m_userDetails.find( userId ) == m_userDetails.end() )
    {
        m_userDetails.insert( std::make_pair( userId, userName ) );
    }
}




/* Filling the db...

* "INSERT INTO nodes (id,latitude,longitude,user_id,visible,tags,timestamp,tile) VALUES (?,ROUND(?*10000000),ROUND(?*10000000),?,1,?,?,tile_for_point(CAST(ROUND(?*10000000) AS UNSIGNED),CAST(ROUND(?*10000000) AS UNSIGNED)))"
* "INSERT INTO ways (id,user_id,timestamp,version,visible) VALUES (?,?,?,1,1)"
* "INSERT INTO way_tags (id,k,v,version) VALUES (?,?,?,1)";
* "INSERT INTO relations (id,user_id,timestamp,version,visible) VALUES (?,?,?,1,1)"
* "INSERT INTO relation_tags (id,k,v,version) VALUES (?,?,?,1)"
* "INSERT INTO relation_members (id,member_type,member_id,member_role,version) VALUES (?,?,?,?,1)"
* 


*/

