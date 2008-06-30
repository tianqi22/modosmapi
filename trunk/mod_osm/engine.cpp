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
    typedef boost::tuple<boost::uint64_t, std::string, std::string, boost::uint64_t> wayTag_t;
    typedef std::pair<std::vector<wayTag_t>::iterator, std::vector<wayTag_t>::iterator> wayTagRange_t;

    struct WayTagLt
    {
        bool operator()( const wayTag_t &lhs, const wayTag_t &rhs ) const
        {
            return lhs.get<0>() < rhs.get<0>();
        }
        bool operator()( boost::uint64_t lhs, const wayTag_t &rhs ) const
        {
            return lhs < rhs.get<0>();
        }
        bool operator()( const wayTag_t &lhs, boost::uint64_t rhs ) const
        {
            return lhs.get<0>() < rhs;
        }

    };

    class XMLWriter
    {

    public:
        XMLWriter( std::ostream &outStream ) : indentLevel( 0 ), m_outStream( outStream )
        {
        }

        void startNode( const std::string &nodeName );

        template<typename T>
        void startNode( const std::string &nodeName, const std::string &attrName, const T &attrValue );

        template<typename T1, typename T2>
        void startNode( const std::string &nodeName, const std::string attrNames[], const boost::tuples::cons<T1, T2> &attrValueTuple );

        void endNode( const std::string &nodeName );

    private:
        void writeIndent();

        int indentLevel;
        std::ostream &m_outStream;
    };

    void XMLWriter::writeIndent()
    {
        m_outStream << std::string( indentLevel * 2, ' ' );
    }

    void XMLWriter::startNode( const std::string &nodeName )
    {
        writeIndent();
        m_outStream << "<" << nodeName << ">" << std::endl;
        indentLevel++;
    }

    template<typename T>
    void XMLWriter::startNode( const std::string &nodeName, const std::string &attrName, const T &attrValue )
    {
        writeIndent();
        m_outStream << "<" << nodeName << " " << attrName << "=\"" << attrValue << "\">" << std::endl;
        indentLevel++;
    }

    template<typename T>
    void renderTags(
        const boost::tuples::cons<T, boost::tuples::null_type> &tuple,
        const std::string attrNames[],
        std::vector<std::string> &tags,
        int index )
    {
        std::stringstream ss;
        ss << attrNames[index] << "=\"" << tuple.head << "\"";
        tags.push_back( ss.str() );
    }

    template<typename T1, typename T2>
    void renderTags(
        const boost::tuples::cons<T1, T2> &tuple,
        const std::string attrNames[],
        std::vector<std::string> &tags,
        int index )
    {
        std::stringstream ss;
        ss << attrNames[index] << "=\"" << tuple.head << "\"";
        tags.push_back( ss.str() );

        renderTags( tuple.tail, attrNames, tags, index + 1 );
    }

    template<typename T1, typename T2>
    void XMLWriter::startNode(
        const std::string &nodeName,
        const std::string attrNames[],
        const boost::tuples::cons<T1, T2> &attrValueTuple )
    {
        std::vector<std::string> tags;
  
        renderTags( attrValueTuple, attrNames, tags, 0 );

        writeIndent();
        m_outStream << "<" << nodeName << " " << boost::algorithm::join( tags, " " ) << ">" << std::endl;

        indentLevel++;
    }

    void XMLWriter::endNode( const std::string &nodeName )
    {
        indentLevel--;
        writeIndent();
        m_outStream << "</" << nodeName << ">" << std::endl;
    }

    void map(std::ostream &out, // destination for output
        Context &context,  // database connection settings, etc...
        double minLat, double maxLat, double minLon, double maxLon ) // function specific parameters
    {
        DbConnection dbConn( "localhost", "openstreetmap", "openstreetmap", "openstreetmap" );

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

        context.logTime( "Pre setup" );
        BOOST_FOREACH (const std::string &command, setup)
        {
            context.logTime( std::string( "Starting: " + command ) );
            dbConn.executeNoResult (command);
        }
        context.logTime( "Setup complete" );

        // Node attrs: id, lat, lon, user, visible, timestamp
        // Node children: tags with tag attrs k, v (split of tag line on ; and then =)

        // Way attrs: id, user, visible, timestamp
        // Way children: nds with nd attr ref (node id)

        out << xml::setformat (4, ' ');
        out << xml::indent << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
        out << xml::indent << "<osm version=\"0.5\" generator=\"modosmapi\">\n";
        out << xml::inc;

        typedef boost::tuple<
            boost::uint64_t,
            boost::int64_t,
            boost::int64_t,
            bool,
            boost::posix_time::ptime,
            std::string> nodeData_t;

        context.logTime( "Getting users" );
        typedef boost::tuple<boost::uint64_t, std::string> userData_t;
        std::vector<userData_t> userData;
        dbConn.executeBulkRetrieve( "SELECT id, display_name FROM users", userData );
        std::map<boost::uint64_t, std::string> userMap;
        BOOST_FOREACH( const userData_t &user, userData )
        {
            userMap.insert( std::make_pair( user.get<0>(), user.get<1>() ) );
        }

        context.logTime( "Getting nodes" );
        std::vector<nodeData_t> nodesData;
        dbConn.executeBulkRetrieve( "SELECT nodes.id, latitude, longitude, visible, timestamp, tags FROM nodes INNER JOIN temp_node_ids ON nodes.id=temp_node_ids.id", nodesData );
        context.logTime( "Node query complete" );

        BOOST_FOREACH( const nodeData_t &nodeData, nodesData )
        {
            const std::string nodeAttrNames[] = { "id", "lat", "lon", "visible", "timestamp" };
            const std::string &tagString = nodeData.get<5>();
            
            out << xml::indent << "<node " << xml::attrs (nodeAttrNames, boost::make_tuple(
               nodeData.get<0>(),
               nodeData.get<1>() / 10000000.0,
               nodeData.get<2>() / 10000000.0,
               nodeData.get<3>(),
               boost::posix_time::to_iso_extended_string( nodeData.get<4>() ) + "+00:00" ) );
            
            std::vector<std::string> tags;
            boost::algorithm::split( tags, tagString, boost::algorithm::is_any_of( ";" ) );
            if (tags.empty () || (tags.size () == 1 && tags.front ().empty ()))
            {
                out << "/>\n";
            }
            else
            {
                out << ">\n";
                out << xml::inc;
                
                BOOST_FOREACH( const std::string &tag, tags )
                {
                    if ( !tag.empty() )
                    {
                        // TODO: Be less Lazy
                        std::vector<std::string> keyValue;
                        boost::algorithm::split( keyValue, tag, boost::algorithm::is_any_of( "=" ) );
                        
                        if ( keyValue.size() != 2 )
                        {
                            context.logError( boost::str(
                                                  boost::format( "Node tag is not an '=' delimited key-value pair: (%s, %s)\n" )
                                                  % tag % tagString ) );
                        }
                        else
                        {
                            const std::string tagAttrNames[] = { "k", "v" };
                            out << xml::indent << "<tag " << xml::attrs(tagAttrNames, boost::make_tuple( 
                                boost::algorithm::trim_copy( keyValue[0] ),
                                boost::algorithm::trim_copy( keyValue[1] ) ) ) << "/>\n";
                        }
                    }
                }
                out << xml::dec;
                out << xml::indent << "</node>\n";
            }
        }
        
        // TODO: Use a nice sorted vector as for the way tags
        typedef std::map<boost::uint64_t, std::vector<boost::uint64_t> > wayNodes_t;
        wayNodes_t wayNodes;
        dbConn.execute( "SELECT way_nodes.id, node_id FROM way_nodes INNER JOIN temp_way_ids ON way_nodes.id=temp_way_ids.id" );
        while ( dbConn.next() )
        {
            boost::uint64_t wayId = dbConn.getField<boost::uint64_t>( 0 );
            boost::uint64_t nodeId = dbConn.getField<boost::uint64_t>( 1 );
            
            wayNodes[wayId].push_back( nodeId );
        }
        context.logTime( "Retrieval complete" );
        
        // TODO: Use a nice sorted vector as for the way tags
        context.logTime( "Retrieving relations" );
        typedef boost::tuple<boost::uint64_t, std::string, std::string, std::string> relationTag_t;
        typedef std::map<boost::uint64_t, std::vector<relationTag_t> > relationTags_t;
        relationTags_t relationTags;
        dbConn.execute( "SELECT relation_tags.id, k, v, version FROM relation_tags INNER JOIN temp_relation_ids "
            "ON relation_tags.id=temp_relation_ids.id" );


        while( dbConn.next() )
        {
            relationTag_t relationTag;
            dbConn.readRow( relationTag );
            relationTags[relationTag.get<0>()].push_back( relationTag );
        }
        
        typedef boost::tuple<boost::uint64_t, std::string, boost::uint64_t, std::string> relationMember_t;
        typedef std::map<boost::uint64_t, std::vector<relationMember_t> > relationMembers_t;
        relationMembers_t relationMembers;
        dbConn.execute( "SELECT relation_members.id, member_type, member_id, member_role FROM relation_members INNER JOIN "
            "temp_relation_ids ON relation_members.id=temp_relation_ids.id" );


        while ( dbConn.next() )
        {
            relationMember_t relationMember;
            dbConn.readRow( relationMember );
            relationMembers[relationMember.get<0>()].push_back( relationMember );
        }
        

        typedef boost::tuple<boost::uint64_t, bool, boost::posix_time::ptime, boost::uint64_t> relation_t;
        std::vector<relation_t> relations;
        dbConn.executeBulkRetrieve( "SELECT relations.id, visible, timestamp, user_id FROM relations INNER JOIN "
            "temp_relation_ids ON relations.id=temp_relation_ids.id", relations );

        const std::string relationTagNames[] = { "id", "visible", "timestamp", "user" };
        BOOST_FOREACH( const relation_t &relation, relations )
        {        
            out << xml::indent << "<relation " << xml::attrs(
                relationTagNames,
                boost::make_tuple(
                    relation.get<0>(),
                    relation.get<1>(),
                    boost::posix_time::to_iso_extended_string( relation.get<2>() ) + "+00:00",
                    relation.get<3>() ) ) << ">\n";

            out << xml::inc;
            
            relationTags_t::iterator rFindIt = relationTags.find( relation.get<0>() );
            if ( rFindIt != relationTags.end() )
            {
                const std::string tagTagNames[] = { "k", "v" };
                BOOST_FOREACH( const relationTag_t &theTag, rFindIt->second )
                {
                    std::string k = theTag.get<1>();
                    std::string v = theTag.get<2>();
                    
                    out << xml::indent << "<tag " << xml::attrs (tagTagNames, boost::make_tuple (k, v)) << "/>\n";
                }
            }
            
            relationMembers_t::iterator mFindIt = relationMembers.find( relation.get<0>() );
            if ( mFindIt != relationMembers.end() )
            {
                const std::string memberTagNames[] = { "type", "ref", "role" };
                BOOST_FOREACH( const relationMember_t &theMember, mFindIt->second )
                {
                    out << xml::indent << "<member " << xml::attrs (memberTagNames, theMember.tail) << "/>\n";
                }
            }
            
            out << xml::dec;
            out << xml::indent << "</relation>\n";
        }
        context.logTime( "Retrieval complete" );
        
        context.logTime( "Retrieving ways" );
        std::vector<wayTag_t> wayTags;
        dbConn.executeBulkRetrieve( "SELECT way_tags.id, k, v, version FROM way_tags INNER JOIN temp_way_ids ON way_tags.id=temp_way_ids.id", wayTags );

        std::sort( wayTags.begin(), wayTags.end(), WayTagLt() );

        // TODO: Look up the user name from user id
        typedef boost::tuple<
            boost::uint64_t,
            bool,
            boost::posix_time::ptime,
            boost::uint64_t> wayData_t;
    
        std::vector<wayData_t> waysData;
        dbConn.executeBulkRetrieve( "SELECT ways.id, visible, timestamp, user_id FROM ways INNER JOIN temp_way_ids ON ways.id=temp_way_ids.id", waysData );

        BOOST_FOREACH( const wayData_t wayData, waysData )
        { 
            boost::uint64_t wayId = wayData.get<0>();
            
            const std::string wayAttrNames[] = { "id", "visible", "timestamp", "user" };
            
            out << xml::indent << "<way " << xml::attrs (wayAttrNames,
                boost::make_tuple(
                    wayData.get<0>(),
                    wayData.get<1>(),
                    boost::posix_time::to_iso_extended_string( wayData.get<2>() ) + "+00:00",
                    wayData.get<3>() ) ) << ">\n";

            out << xml::inc;
            
            wayNodes_t::iterator findIt = wayNodes.find( wayId );
            if ( findIt != wayNodes.end() )
            {
                BOOST_FOREACH( boost::uint64_t nodeId, findIt->second )
                {
                    out << xml::indent << "<nd ref=" << xml::quoteattr (nodeId) << "/>\n";
                } 
            } 
            
            wayTagRange_t tags = std::equal_range( wayTags.begin(), wayTags.end(), wayId, WayTagLt() );
            
            for ( ; tags.first != tags.second; ++tags.first )
            {
                const std::string tagAttrNames[] = { "k", "v", "version" };
                out << xml::indent << "<tag " << xml::attrs (tagAttrNames, (*tags.first).tail) << "/>\n";
            }
            
            out << xml::dec;
            out << xml::indent << "</way>\n";
        }
        
        context.logTime( "Retrieval complete" );


        out << xml::dec;
        out << xml::indent << "</osm>\n";

        dbConn.executeNoResult( "DROP TABLE temp_way_ids" );
        dbConn.executeNoResult( "DROP TABLE temp_node_ids" );
        dbConn.executeNoResult( "DROP TABLE temp_relation_ids" );
        dbConn.executeNoResult( "DROP TABLE temp_way_node_ids" );
    }

} // end namespace modosmapi

// int main( int argc, char *argv [] )
// {
//     try
//     {
//         double minLat = 515000000;
//         double maxLat = 520000000;
//         double minLon = -10000000;
//         double maxLon = -9000000;

//         std::ofstream opFile( "test_result.txt" );
//         modosmapi::Context c;
//         modosmapi::map(opFile, c, minLat, maxLat, minLon, maxLon );
//     }
//     catch ( const modosmapi::SqlException &e )
//     {
//         std::cout << "SQL exception thrown: " << e.getMessage() << std::endl;
//         return -1;
//     }
//     catch ( const std::exception &e )
//     {
//         std::cout << "std::exception thrown: " << e.what() << std::endl;
//         throw;
//     }

//     return 0;
// }

