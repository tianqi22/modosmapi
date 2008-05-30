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
#include <boost/foreach.hpp>
#include <boost/tuple/tuple.hpp>

#include <mysql/mysql.h>


// <host>/api/0.5
//
// Creation:  PUT <objtype>/create + xml - returns id
// Retrieval: GET <objtype>/id           - returns xml
// Update:    PUT <objtype>/<id> + xml   - returns nothing
// Delete:    DELETE <objtype>/<id>      - returns nothing

// GET /api/0.5/map?bbox=<left>,<bottom>,<right>,<top>
// GET /api/0.5/trackpoints?bbox=<left>,<bottom>,<right>,<top>&page=<pagenumber>
// GET /api/0.5/<objtype>/<id>/history
// GET /api/0.5/<objtype>s?<objtype>s=<id>[,<id>]
// GET /api/0.5/node/<id>/ways
// GET /api/0.5/<objtype>/<id>/relations
// GET /api/0.5/<objtype>/<id>/full
// GET /api/0.5/changes?hours=1&zoom=16
// GET /api/0.5/ways/search?type=<type>&value=<value>
// GET /api/0.5/relations/search?type=<type>&value=<value>
// GET /api/0.5/search?type=<type>&value=<value>
// POST /api/0.5/gpx/create
// GET /api/0.5/gpx/<id>/details
// GET /api/0.5/gpx/<id>/data
// GET /api/0.5/user/preferences





class SqlException : public std::exception
{
    std::string m_message;

public:
    SqlException( const std::string &message ) : m_message( message )
    {
    }

    virtual ~SqlException() throw ()
    {
    }

    const std::string &getMessage() const
    {
        return m_message;
    }
};


class DbConnection
{
public:
    DbConnection(
                 const std::string &dbhost,
                 const std::string &dbname,
                 const std::string &dbuser,
                 const std::string &dbpass ) : m_res( NULL ), m_row( NULL )
    {
        if ( created )
        {
            throw SqlException( "Single instance has already been created" );
        }
        created = true;

        if ( !mysql_init( &m_dbconn ) )
        {
            throw SqlException( std::string( "Mysql init failure: " ) + mysql_error( &m_dbconn ) );
        }

        if ( !mysql_real_connect( &m_dbconn, dbhost.c_str(), dbname.c_str(), dbuser.c_str(), dbpass.c_str(), 0, NULL, 0 ) )
        {
            throw SqlException( std::string( "Mysql connection failed: " ) + mysql_error( &m_dbconn ) );
        }
    }

    void cleanUp()
    {
        if ( m_res )
        {
            mysql_free_result( m_res );
            m_res = NULL;
            m_row = NULL;
        }
    }

    void execute_noresult( std::string query )
    {
        cleanUp();

        std::cout << "Executing query: " << query << std::endl;
        if ( mysql_query( &m_dbconn, query.c_str() ) )
        {
            throw SqlException( std::string( "Query failed: " ) + mysql_error( &m_dbconn ) );
        }    
    }


    void execute( std::string query )
    {
        cleanUp();

        std::cout << "Executing query: " << query << std::endl;
        if ( mysql_query( &m_dbconn, query.c_str() ) )
        {
            throw SqlException( std::string( "Query failed: " ) + mysql_error( &m_dbconn ) );
        }

        if ( !(m_res=mysql_use_result( &m_dbconn )) )
        {
            throw SqlException( std::string( "Result store failed: " ) + mysql_error( &m_dbconn ) );
        }

        next();
    }

    bool next()
    {
        if ( !m_res )
        {
            throw SqlException( "Cannot call next - no valid result" );
        }

        m_row = mysql_fetch_row( m_res );

        return m_row;
    }

    template<typename T>
    T getField( size_t index )
    {
        if ( !m_row || !m_res )
        {
            throw SqlException( "Calling getField on empty row. Is query empty or exhausted?" );
        }

        if ( index > mysql_num_fields( m_res ) )
        {
            throw SqlException( "Index greater than number of fields" );
        }
        return boost::lexical_cast<T>( m_row[index] );
    }

    template<typename T1, typename T2>
    void readRow( boost::tuples::cons<T1, T2> &t, size_t index=0 )
    {
        t.head = getField<T1>( index );
        readRow( t.tail, index + 1 );
    }

    template<typename T>
    void readRow( boost::tuples::cons<T, boost::tuples::null_type> &t, size_t index=0 )
    {
        t.head = getField<T>( index );
    }


    ~DbConnection()
    {
        cleanUp();

        mysql_close( &m_dbconn );
    }

private:
    MYSQL m_dbconn;
    MYSQL_RES *m_res;
    MYSQL_ROW m_row;

    static bool created;
};

/*static*/ bool DbConnection::created = false;

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
    XMLWriter( std::ostream &outStream ) : m_outStream( outStream )
    {
    }

    void startNode( const std::string &nodeName );

    template<typename T>
    void startNode( const std::string &nodeName, const std::string &attrName, const T &attrValue );

    template<typename T1, typename T2>
    void startNode( const std::string &nodeName, const std::string attrNames[], const boost::tuples::cons<T1, T2> &attrValueTuple );

    void endNode( const std::string &nodeName );

private:

    std::ostream &m_outStream;
};

void XMLWriter::startNode( const std::string &nodeName )
{
    m_outStream << "<" << nodeName << ">";
}

template<typename T>
void XMLWriter::startNode( const std::string &nodeName, const std::string &attrName, const T &attrValue )
{
    m_outStream << "<" << nodeName << " " << attrName << "=\"" << attrValue << "\">";
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
void XMLWriter::startNode( const std::string &nodeName, const std::string attrNames[], const boost::tuples::cons<T1, T2> &attrValueTuple )
{
    std::vector<std::string> tags;
  
    renderTags( attrValueTuple, attrNames, tags, 0 );

    m_outStream << "<" << nodeName << " " << boost::algorithm::join( tags, " " ) << ">";
}

void XMLWriter::endNode( const std::string &nodeName )
{
    m_outStream << "</" << nodeName << ">";
}

void map( double minLat, double maxLat, double minLon, double maxLon )
{
    std::ofstream opFile( "test_result.txt" );
    XMLWriter xmlWriter( opFile );

    DbConnection dbConn( "localhost", "openstreetmap", "openstreetmap", "openstreetmap" );

    dbConn.execute_noresult( "CREATE TEMPORARY TABLE temp_node_ids( id BIGINT, PRIMARY KEY(id) )" );
    dbConn.execute_noresult( "CREATE TEMPORARY TABLE temp_way_ids( id BIGINT, PRIMARY KEY(id) )" );

    dbConn.execute_noresult( boost::str( boost::format(
                                                       "INSERT INTO temp_node_ids SELECT id FROM nodes WHERE "
                                                       "latitude > %f AND latitude < %f AND longitude > %f AND longitude < %f" )
                                         % minLat % maxLat % minLon % maxLon ) );

    dbConn.execute_noresult(
                            "INSERT INTO temp_way_ids SELECT DISTINCT way_nodes.id FROM way_nodes INNER JOIN "
                            "temp_node_ids ON way_nodes.node_id=temp_node_ids.id" );

    // TODO: Make this not insert duplicate
    //dbConn.execute_noresult(
    //  "INSERT INTO temp_node_ids SELECT DISTINCT way_nodes.node_id FROM way_nodes "
    //  "INNER JOIN temp_way_ids ON way_nodes.id=temp_way_ids.id" );

    dbConn.execute( "SELECT COUNT(*) FROM temp_way_ids" );
    std::cout << dbConn.getField<int>( 0 ) << std::endl;

    dbConn.execute( "SELECT COUNT(*) FROM temp_node_ids" );
    std::cout << dbConn.getField<int>( 0 ) << std::endl;

    // Node attrs: id, lat, lon, user, visible, timestamp
    // Node children: tags with tag attrs k, v (split of tag line on ; and then =)

    // Way attrs: id, user, visible, timestamp
    // Way children: nds with nd attr ref (node id)

    dbConn.execute( "SELECT nodes.id, latitude, longitude, visible, timestamp, tags  FROM nodes INNER JOIN temp_node_ids ON nodes.id=temp_node_ids.id" );
    std::cout << "Writing out nodes" << std::endl;
    do
    {
        const std::string nodeAttrNames[] = { "id", "lat", "lon", "visible", "timestamp" };
        boost::tuple<
            boost::uint64_t,
            boost::int64_t,
            boost::int64_t,
            bool,
            std::string> nodeData;

        dbConn.readRow( nodeData );

        xmlWriter.startNode( "node", nodeAttrNames, nodeData );

        std::string tagString = dbConn.getField<std::string>( 5 );

        std::vector<std::string> tags;
        boost::algorithm::split( tags, tagString, boost::algorithm::is_any_of( ";" ) );
        BOOST_FOREACH( const std::string &tag, tags )
        {
            if ( !tag.empty() )
            {
                // TODO: Be less Lazy
                std::vector<std::string> keyValue;
                boost::algorithm::split( keyValue, tag, boost::algorithm::is_any_of( "=" ) );

                if ( keyValue.size() != 2 )
                {
                    std::cout << "Node tag is not an '=' delimited key-value pair: " << tag << " (" << tagString << ")" << std::endl;
                }
                else
                {
                    const std::string tagAttrNames[] = { "k", "v" };
                    xmlWriter.startNode( "tag", tagAttrNames, boost::make_tuple( keyValue[0], keyValue[1] ) );
                    xmlWriter.endNode( "tag" );
                }
            }
        }
        xmlWriter.endNode( "node" );
    }
    while ( dbConn.next() );

    // TODO: Use a nice sorted vector as for the way tags
    typedef std::map<boost::uint64_t, std::vector<boost::uint64_t> > wayNodes_t;
    wayNodes_t wayNodes;
    dbConn.execute( "SELECT way_nodes.id, node_id FROM way_nodes INNER JOIN temp_way_ids ON way_nodes.id=temp_way_ids.id" );
    do
    {
        boost::uint64_t wayId = dbConn.getField<boost::uint64_t>( 0 );
        boost::uint64_t nodeId = dbConn.getField<boost::uint64_t>( 1 );

        wayNodes[wayId].push_back( nodeId );
    }
    while ( dbConn.next() );

    std::vector<wayTag_t> wayTags;
    dbConn.execute( "SELECT way_tags.id, k, v, version FROM way_tags INNER JOIN temp_way_ids ON way_tags.id=temp_way_ids.id" );
    do
    {
        wayTag_t wayTag;

        dbConn.readRow( wayTag );

        wayTags.push_back( wayTag );
    }
    while ( dbConn.next() );

    std::sort( wayTags.begin(), wayTags.end(), WayTagLt() );

    std::cout << "Number of way tags: " << wayTags.size() << std::endl;

    // TODO: Look up the user name from user id
    dbConn.execute( "SELECT ways.id, visible, timestamp, user_id FROM ways INNER JOIN temp_way_ids ON ways.id=temp_way_ids.id" );

    do
    {
        boost::tuple<
            boost::uint64_t,
            bool,
            std::string,
            boost::uint64_t> wayData;

        dbConn.readRow( wayData );
        boost::uint64_t wayId = wayData.get<0>();

        const std::string wayAttrNames[] = { "id", "visible", "timestamp", "user" };

        xmlWriter.startNode( "way", wayAttrNames, wayData );

        wayNodes_t::iterator findIt = wayNodes.find( wayId );
        if ( findIt != wayNodes.end() )
        {
            BOOST_FOREACH( boost::uint64_t nodeId, findIt->second )
            {
                xmlWriter.startNode( "nd", "ref", nodeId );
                xmlWriter.endNode( "nd" );
            } 
        } 

        wayTagRange_t tags = std::equal_range( wayTags.begin(), wayTags.end(), wayId, WayTagLt() );

        for ( ; tags.first != tags.second; tags.first++ )
        {
            const std::string tagAttrNames[] = { "k", "v", "version" };
            xmlWriter.startNode( "tag", tagAttrNames, *tags.first );
            xmlWriter.endNode( "tag" );
        }
		
        xmlWriter.endNode( "way" );
    }
    while ( dbConn.next() );

    // Also output: ways, way_nodes, relations

    dbConn.execute_noresult( "DROP TABLE temp_way_ids" );
    dbConn.execute_noresult( "DROP TABLE temp_node_ids" );

    // Similarly: SELECT ways.* FROM ways INNER JOIN temp_way_ids ON ways.id=temp_way_ids.id
    //            SELECT way_nodes.* FROM way_nodes INNER JOIN temp_way_ids ON way_nodes.id=temp_way_ids.id
    //            (and way_tags and relations)
}

int main( int argc, char **argv )
{
    try
    {
        double minLat = 515000000;
        double maxLat = 520000000;
        double minLon = -10000000;
        double maxLon = -9000000;

        map( minLat, maxLat, minLon, maxLon );
    }
    catch ( const SqlException &e )
    {
        std::cout << "SQL exception thrown: " << e.getMessage() << std::endl;
        return -1;
    }
    catch ( const std::exception &e )
    {
        std::cout << "std::exception thrown: " << e.what() << std::endl;
    }

    return 0;
}
