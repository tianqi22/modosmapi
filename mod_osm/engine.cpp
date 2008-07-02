#include "engine.hpp"
#include "ioxml.hpp"
#include "dbhandler.hpp"

#include <iostream>
#include <fstream>
#include <exception>
#include <algorithm>
#include <vector>
#include <map>
#include <string>
#include <sstream>

#include <boost/cstdint.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/foreach.hpp>
#include <boost/tuple/tuple.hpp>

#include <boost/date_time/posix_time/posix_time.hpp>

#include <mysql/mysql.h>



// <host>/api/0.5select * from relation_tags inner join relations on relations.id=relation_tags.id inner join relation_members on relations.id=relation_members.id where member_id=4221467 and member_type='way'
//
// Creation:  PUT <objtype>/create + xml - returns id
// Retrieval: GET <objtype>/id           - returns xml
// Update:    PUT <objtype>/<id> + xml   - returns nothing
// Delete:    DELETE <objtype>/<id>      - returns nothing

// GET  /api/0.5/map?bbox=<left>,<bottom>,<right>,<top>
// GET  /api/0.5/trackpoints?bbox=<left>,<bottom>,<right>,<top>&page=<pagenumber>
// GET  /api/0.5/<objtype>/<id>/history
// GET  /api/0.5/<objtype>s?<objtype>s=<id>[,<id>]
// GET  /api/0.5/node/<id>/ways
// GET  /api/0.5/<objtype>/<id>/relations
// GET  /api/0.5/<objtype>/<id>/full
// GET  /api/0.5/changes?hours=1&zoom=16
// GET  /api/0.5/ways/search?type=<type>&value=<value>
// GET  /api/0.5/relations/search?type=<type>&value=<value>
// GET  /api/0.5/search?type=<type>&value=<value>
// POST /api/0.5/gpx/create
// GET  /api/0.5/gpx/<id>/details
// GET  /api/0.5/gpx/<id>/data
// GET  /api/0.5/user/preferences


namespace modosmapi
{
    class MapQueryGen
    {
        typedef boost::tuple<
            boost::uint64_t,
            boost::int64_t,
            boost::int64_t,
            bool,
            boost::posix_time::ptime,
            std::string> nodeData_t;
        
        typedef boost::tuple<
            boost::uint64_t,
            bool,
            boost::posix_time::ptime,
            boost::uint64_t> wayData_t;
    
        typedef boost::tuple<
            boost::uint64_t,
            bool,
            boost::posix_time::ptime,
            boost::uint64_t> relationData_t;

        typedef boost::tuple<
            boost::uint64_t,
            std::string> userData_t;

        typedef boost::tuple<
            boost::uint64_t,
            std::string,
            std::string,
            boost::uint64_t> wayTag_t;
    
        typedef boost::tuple<
            boost::uint64_t,
            std::string,
            std::string,
            std::string> relationTag_t;

        typedef boost::tuple<
            boost::uint64_t,
            std::string,
            boost::uint64_t,
            std::string> relationMember_t;

        typedef boost::tuple<boost::uint64_t, boost::uint64_t> wayNode_t;

        typedef std::map<boost::uint64_t, std::vector<wayTag_t> > wayTags_t; 
        typedef std::map<boost::uint64_t, std::vector<relationTag_t> > relationTags_t;
        typedef std::map<boost::uint64_t, std::vector<boost::uint64_t> > wayNodes_t;
        typedef std::map<boost::uint64_t, std::vector<relationMember_t> > relationMembers_t;

        DbConnection m_dbConn;
        std::ostream &m_out;
        Context &m_context;

        std::map<boost::uint64_t, std::string> m_userMap;
        wayNodes_t m_wayNodes;
        wayTags_t m_wayTags;
        relationTags_t m_relationTags;
        relationMembers_t m_relationMembers;

    public:
        MapQueryGen( std::ostream &out, Context &context, double minLat, double maxLat, double minLon, double MaxLon );
        ~MapQueryGen();

    private:
        void setupTemporaryTables( double minLat, double maxLat, double minLon, double MaxLon );
        void outputXML();
        void outputNode( const nodeData_t &node );
        void outputWay( const wayData_t &wayData );
        void outputRelation( const relationData_t &relation );
        void cleanupTemporaryTables();

        void addUser( const userData_t &userData );
        void addWayNode( const wayNode_t &wayNode );
        void addWayTag( const wayTag_t &wayTag );
        void addRelationTag( const relationTag_t &relationTag );
        void addRelationMember( const relationMember_t &relationMember );
};

template<typename ArgType, typename FnType>
boost::function<void( const ArgType & )> cbFn( FnType fn )
{
    return boost::function<void( const ArgType & )>( fn );
}

MapQueryGen::MapQueryGen( std::ostream &out, Context &context, double minLat, double maxLat, double minLon, double maxLon ) :
    m_dbConn( "localhost", "openstreetmap", "openstreetmap", "openstreetmap" ),
    m_out( out ), m_context( context )
{
    setupTemporaryTables( minLat, maxLat, minLon, maxLon );
    
    // Fill the user, tag and member tables
    m_context.logTime( "Getting users" );
    m_dbConn.executeBulkRetrieve( "SELECT id, display_name FROM users",
        cbFn<userData_t>( boost::bind( &MapQueryGen::addUser, boost::ref( this ), _1 ) ) );
    
    m_context.logTime( "Getting way nodes" );
    m_dbConn.executeBulkRetrieve( "SELECT way_nodes.id, node_id FROM way_nodes "
        "INNER JOIN temp_way_ids ON way_nodes.id=temp_way_ids.id",
        cbFn<wayNode_t>( boost::bind( &MapQueryGen::addWayNode, boost::ref( this ), _1 ) ) );
    
    m_context.logTime( "Getting relation tags" );
    m_dbConn.executeBulkRetrieve( "SELECT relation_tags.id, k, v, version FROM relation_tags "
        "INNER JOIN temp_relation_ids ON relation_tags.id=temp_relation_ids.id",
        cbFn<relationTag_t>( boost::bind( &MapQueryGen::addRelationTag, boost::ref( this ), _1 ) ) );
    
    m_context.logTime( "Getting relation members" );
    m_dbConn.executeBulkRetrieve( "SELECT relation_members.id, member_type, member_id, "
        "member_role FROM relation_members INNER JOIN temp_relation_ids "
        "ON relation_members.id=temp_relation_ids.id",
        cbFn<relationMember_t>( boost::bind( &MapQueryGen::addRelationMember, boost::ref( this ), _1 ) ) );
    
    m_context.logTime( "Getting way tags" );
    m_dbConn.executeBulkRetrieve( "SELECT way_tags.id, k, v, version FROM way_tags "
        "INNER JOIN temp_way_ids ON way_tags.id=temp_way_ids.id",
        cbFn<wayTag_t>( boost::bind( &MapQueryGen::addWayTag, boost::ref( this ), _1 ) ) );
    
    outputXML();
}

void MapQueryGen::outputXML()
{
    m_context.logTime( "Beginning XML output" );
    
    m_out << xml::setformat (4, ' ');
    m_out << xml::indent << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    m_out << xml::indent << "<osm version=\"0.5\" generator=\"modosmapi\">\n";
    m_out << xml::inc;
    
    m_context.logTime( "Outputting nodes" );
    m_dbConn.executeBulkRetrieve( "SELECT nodes.id, latitude, longitude, visible, "
        "timestamp, tags FROM nodes INNER JOIN temp_node_ids "
        "ON nodes.id=temp_node_ids.id",
        cbFn<nodeData_t>( boost::bind( &MapQueryGen::outputNode, boost::ref( this ), _1 ) ) );
    
    m_context.logTime( "Outputting ways" );
    m_dbConn.executeBulkRetrieve( "SELECT ways.id, visible, timestamp, user_id "
        "FROM ways INNER JOIN temp_way_ids ON ways.id=temp_way_ids.id",
        cbFn<wayData_t>( boost::bind( &MapQueryGen::outputWay, boost::ref( this ), _1 ) ) );
    
    m_context.logTime( "Outputting relations" );
    m_dbConn.executeBulkRetrieve( "SELECT relations.id, visible, timestamp, user_id "
        "FROM relations INNER JOIN temp_relation_ids ON relations.id=temp_relation_ids.id",
        cbFn<relationData_t>( boost::bind( &MapQueryGen::outputRelation, boost::ref( this ), _1 ) ) );
    
    m_out << xml::dec;
    m_out << xml::indent << "</osm>\n";
    
    m_context.logTime( "XML output complete. Clearing up..." );
}

void MapQueryGen::outputNode( const nodeData_t &nodeData )
{
    const std::string nodeAttrNames[] = { "id", "lat", "lon", "visible", "timestamp" };
    const std::string &tagString = nodeData.get<5>();
    
    m_out << xml::indent << "<node " << xml::attrs (nodeAttrNames, boost::make_tuple(
        nodeData.get<0>(),
        nodeData.get<1>() / 10000000.0,
        nodeData.get<2>() / 10000000.0,
        nodeData.get<3>(),
        boost::posix_time::to_iso_extended_string( nodeData.get<4>() ) + "+00:00" ) );
    
    std::vector<std::string> tags;
    boost::algorithm::split( tags, tagString, boost::algorithm::is_any_of( ";" ) );
    if (tags.empty () || (tags.size () == 1 && tags.front ().empty ()))
    {
        m_out << "/>\n";
    }
    else
    {
        m_out << ">\n";
        m_out << xml::inc;
        
        BOOST_FOREACH( const std::string &tag, tags )
        {
            if ( !tag.empty() )
            {
                // TODO: Be less Lazy
                std::vector<std::string> keyValue;
                boost::algorithm::split( keyValue, tag, boost::algorithm::is_any_of( "=" ) );
                
                if ( keyValue.size() != 2 )
                {
                    m_context.logError( boost::str(
                                      boost::format( "Node tag is not an '=' delimited key-value pair: (%s, %s)\n" )
                                      % tag % tagString ) );
                }
                else
                {
                    const std::string tagAttrNames[] = { "k", "v" };
                    m_out << xml::indent << "<tag " << xml::attrs(tagAttrNames, boost::make_tuple( 
                        boost::algorithm::trim_copy( keyValue[0] ),
                        boost::algorithm::trim_copy( keyValue[1] ) ) ) << "/>\n";
                }
            }
        }
        m_out << xml::dec;
        m_out << xml::indent << "</node>\n";
    }
}

void MapQueryGen::outputWay( const wayData_t &wayData )
{
    boost::uint64_t wayId = wayData.get<0>();
    
    const std::string wayAttrNames[] = { "id", "visible", "timestamp", "user" };
    
    m_out << xml::indent << "<way " << xml::attrs (wayAttrNames,
        boost::make_tuple(
            wayData.get<0>(),
            wayData.get<1>(),
            boost::posix_time::to_iso_extended_string( wayData.get<2>() ) + "+00:00",
            wayData.get<3>() ) ) << ">\n";
    
    m_out << xml::inc;
    
    wayNodes_t::iterator wFindIt = m_wayNodes.find( wayId );
    if ( wFindIt != m_wayNodes.end() )
    {
        BOOST_FOREACH( boost::uint64_t nodeId, wFindIt->second )
        {
            m_out << xml::indent << "<nd ref=" << xml::quoteattr (nodeId) << "/>\n";
        } 
    }
    
    wayTags_t::iterator tFindIt = m_wayTags.find( wayId );
    if ( tFindIt != m_wayTags.end() )
    {
        BOOST_FOREACH( const wayTag_t &wayTag, tFindIt->second )
        {
            const std::string tagAttrNames[] = { "k", "v", "version" };
            m_out << xml::indent << "<tag " << xml::attrs (tagAttrNames, wayTag.tail) << "/>\n";
        }
    }
    
    m_out << xml::dec;
    m_out << xml::indent << "</way>\n";
}

void MapQueryGen::outputRelation( const relationData_t &relation )
{
    
    const std::string relationTagNames[] = { "id", "visible", "timestamp", "user" };
    m_out << xml::indent << "<relation " << xml::attrs(
        relationTagNames,
        boost::make_tuple(
            relation.get<0>(),
            relation.get<1>(),
            boost::posix_time::to_iso_extended_string( relation.get<2>() ) + "+00:00",
            relation.get<3>() ) ) << ">\n";
    
    m_out << xml::inc;
    
    relationTags_t::iterator rFindIt = m_relationTags.find( relation.get<0>() );
    if ( rFindIt != m_relationTags.end() )
    {
        const std::string tagTagNames[] = { "k", "v" };
        BOOST_FOREACH( const relationTag_t &theTag, rFindIt->second )
        {
            std::string k = theTag.get<1>();
            std::string v = theTag.get<2>();
            
            m_out << xml::indent << "<tag " << xml::attrs (tagTagNames, boost::make_tuple (k, v)) << "/>\n";
        }
    }
    
    relationMembers_t::iterator mFindIt = m_relationMembers.find( relation.get<0>() );
    if ( mFindIt != m_relationMembers.end() )
    {
        const std::string memberTagNames[] = { "type", "ref", "role" };
        BOOST_FOREACH( const relationMember_t &theMember, mFindIt->second )
        {
            m_out << xml::indent << "<member " << xml::attrs (memberTagNames, theMember.tail) << "/>\n";
        }
    }
    
    m_out << xml::dec;
    m_out << xml::indent << "</relation>\n";
}

MapQueryGen::~MapQueryGen()
{
    cleanupTemporaryTables();
}

void MapQueryGen::setupTemporaryTables( double minLat, double maxLat, double minLon, double maxLon )
{
    boost::int64_t minLatInt = (minLat * 10000000.0);
    boost::int64_t maxLatInt = (maxLat * 10000000.0);
    boost::int64_t minLonInt = (minLon * 10000000.0);
    boost::int64_t maxLonInt = (maxLon * 10000000.0);
    
    std::string setup [] =
    {
        "CREATE TEMPORARY TABLE temp_node_ids    ( id BIGINT )",
        "CREATE TEMPORARY TABLE temp_way_ids     ( id BIGINT )",
        "CREATE TEMPORARY TABLE temp_relation_ids( id BIGINT )",
        "CREATE TEMPORARY TABLE temp_way_node_ids( id BIGINT )",
        boost::str( boost::format(
                    "INSERT INTO temp_node_ids SELECT id FROM nodes WHERE "
                    "latitude > %f AND latitude < %f AND longitude > %f AND longitude < %f" )
            % minLatInt % maxLatInt % minLonInt % maxLonInt ),
        "INSERT INTO temp_way_ids SELECT DISTINCT way_nodes.id FROM way_nodes INNER JOIN "
        "temp_node_ids ON way_nodes.node_id=temp_node_ids.id",
        "ALTER TABLE temp_node_ids add PRIMARY KEY(id)",
        "INSERT INTO temp_way_node_ids SELECT DISTINCT way_nodes.node_id FROM way_nodes "
        "INNER JOIN temp_way_ids ON way_nodes.id=temp_way_ids.id WHERE way_nodes.node_id NOT IN "
        "( SELECT id from temp_node_ids )",
        "INSERT INTO temp_node_ids SELECT id FROM temp_way_node_ids",
        "INSERT INTO temp_relation_ids SELECT DISTINCT relation_members.id FROM "
        "relation_members INNER JOIN temp_node_ids ON relation_members.member_id=temp_node_ids.id "
        "WHERE member_type='node'",
        "INSERT IGNORE INTO temp_relation_ids SELECT DISTINCT relation_members.id "
        "FROM relation_members INNER JOIN temp_way_ids ON relation_members.member_id=temp_way_ids.id "
        "WHERE member_type='way'",
    };
    
    m_context.logTime( "Pre setup" );
    BOOST_FOREACH (const std::string &command, setup)
    {
        m_context.logTime( std::string( "Starting: " + command ) );
        m_dbConn.executeNoResult (command);
    }
    m_context.logTime( "Setup complete" );
}

void MapQueryGen::cleanupTemporaryTables()
{
    m_dbConn.executeNoResult( "DROP TABLE temp_way_ids" );
    m_dbConn.executeNoResult( "DROP TABLE temp_node_ids" );
    m_dbConn.executeNoResult( "DROP TABLE temp_relation_ids" );
    m_dbConn.executeNoResult( "DROP TABLE temp_way_node_ids" );
}

void MapQueryGen::addUser( const userData_t &userData )
{
    m_userMap.insert( std::make_pair( userData.get<0>(), userData.get<1>() ) );
}

void MapQueryGen::addWayNode( const boost::tuple<boost::uint64_t, boost::uint64_t> &wayNode )
{
    m_wayNodes[wayNode.get<0>()].push_back( wayNode.get<1>() );
}

void MapQueryGen::addWayTag( const wayTag_t &wayTag )
{
    m_wayTags[wayTag.get<0>()].push_back( wayTag );
}

void MapQueryGen::addRelationTag( const relationTag_t &relationTag )
{
    m_relationTags[relationTag.get<0>()].push_back( relationTag );
}

void MapQueryGen::addRelationMember( const relationMember_t &relationMember )
{
    m_relationMembers[relationMember.get<0>()].push_back( relationMember );
}


void map(std::ostream &out, // destination for output
    Context &context,  // database connection settings, etc...
    double minLat, double maxLat, double minLon, double maxLon ) // function specific parameters
{
    MapQueryGen m( out, context, minLat, maxLat, minLon, maxLon );
}

} // end namespace modosmapi
