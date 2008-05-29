#include <iostream>
#include <fstream>
#include <exception>
#include <algorithm>
#include <vector>
#include <map>

#include <boost/cstdint.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/foreach.hpp>
#include <boost/tuple/tuple.hpp>

#include <mysql/mysql.h>


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

void test()
{
  std::ofstream opFile( "test_result.txt" );
  DbConnection dbConn( "localhost", "openstreetmap", "openstreetmap", "openstreetmap" );

  double minLat = 515000000;
  double maxLat = 520000000;
  double minLon = -10000000;
  double maxLon = -9000000;

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

  dbConn.execute( "SELECT nodes.id, latitude, longitude, user_id, visible, timestamp, tags  FROM nodes INNER JOIN temp_node_ids ON nodes.id=temp_node_ids.id" );
  std::cout << "Writing out nodes" << std::endl;
  do
  {
    boost::uint64_t nodeId = dbConn.getField<boost::uint64_t>( 0 );
    boost::int64_t latitude = dbConn.getField<boost::int64_t>( 1 );
    boost::int64_t longitude = dbConn.getField<boost::int64_t>( 2 );
    std::string userId = dbConn.getField<std::string>( 3 );
    bool visible = dbConn.getField<bool>( 4 );
    std::string timeStamp = dbConn.getField<std::string>( 5 );
    std::string tagString = dbConn.getField<std::string>( 6 );

    opFile << boost::str( boost::format(
      "<node id=\"%d\" lat=\"%d\" lon=\"%d\" visible=\"%s\" timestamp=\"%s\">" )
      % nodeId % latitude % longitude % (visible ? "true" : "false") % timeStamp ) << std::endl;

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
	  opFile << boost::str( boost::format( "  <tag k=\"%s\" v=\"%s\"/>" ) % keyValue[0] % keyValue[1] ) << std::endl;
	}
      }
    }
    opFile << "</node>" << std::endl;
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
    boost::uint64_t id = dbConn.getField<boost::uint64_t>( 0 );
    std::string k = dbConn.getField<std::string>( 1 );
    std::string v = dbConn.getField<std::string>( 2 );
    boost::uint64_t version = dbConn.getField<boost::uint64_t>( 3 );

    wayTags.push_back( boost::make_tuple( id, k, v, version ) );
  }
  while ( dbConn.next() );

  std::sort( wayTags.begin(), wayTags.end(), WayTagLt() );

  std::cout << "Number of way tags: " << wayTags.size() << std::endl;

  dbConn.execute( "SELECT ways.id, visible, timestamp, user_id FROM ways INNER JOIN temp_way_ids ON ways.id=temp_way_ids.id" );

  do
  {
    boost::uint64_t wayId = dbConn.getField<boost::uint64_t>( 0 );
    bool visible = dbConn.getField<bool>( 1 );
    std::string timeStamp = dbConn.getField<std::string>( 2 );
    boost::uint64_t user_id = dbConn.getField<boost::uint64_t>( 3 );

    // TODO: Look up the user name from user id
    opFile << boost::str( boost::format( "<way id=\"%d\" visible=\"%s\" timestamp=\"%s\" user=\"%d\">" )
      % wayId % (visible ? "true" : "false") % timeStamp % user_id ) << std::endl;

    wayNodes_t::iterator findIt = wayNodes.find( wayId );
    if ( findIt != wayNodes.end() )
    {
      BOOST_FOREACH( boost::uint64_t nodeId, findIt->second )
      {
	opFile << boost::str( boost::format( "  <nd ref=\"%d\"/>" ) % nodeId ) << std::endl;
      } 
    } 

    wayTagRange_t tags = std::equal_range( wayTags.begin(), wayTags.end(), wayId, WayTagLt() );

    for ( ; tags.first != tags.second; tags.first++ )
    {
      opFile << boost::str( boost::format( "  <tag k=\"%s\" v=\"%s\" version=\"%d\"/>" )
        % tags.first->get<1>() % tags.first->get<2>() % tags.first->get<3>() ) << std::endl;
    }
    opFile << "</way>" << std::endl;
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
    test();
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
