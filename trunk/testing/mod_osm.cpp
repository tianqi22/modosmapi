#define off64_t __off64_t

#include <httpd.h>
#include <http_protocol.h>
#include <http_config.h>

#include <boost/iostreams/stream_buffer.hpp>
#include <boost/iostreams/stream.hpp>
#include <string>

#include <iostream>
#include <fstream>
#include <exception>

#include <boost/cstdint.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/foreach.hpp>

#include <mysql/mysql.h>

struct Sink
{
    typedef char char_type;
    typedef boost::iostreams::sink_tag category;
    std::streamsize write (const char *s, std::streamsize n)
    {
        std::string str (s, n);
        ap_rputs (str.c_str (), r);
	return n;
    }
    request_rec *r;
};


typedef boost::iostreams::stream<Sink> apache_stream;


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

  void execute_noresult( std::string query )
  {
    std::cout << "Executing query: " << query << std::endl;
    if ( mysql_query( &m_dbconn, query.c_str() ) )
    {
      throw SqlException( std::string( "Query failed: " ) + mysql_error( &m_dbconn ) );
    }    
  }
    

  void execute( std::string query )
  {
    if ( m_res )
    {
      mysql_free_result( m_res );
      m_row = NULL;
    }

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
    if ( m_res )
    {
      mysql_free_result( m_res );
    }

    mysql_close( &m_dbconn );
  }

private:
  MYSQL m_dbconn;
  MYSQL_RES *m_res;
  MYSQL_ROW m_row;

  static bool created;
};

/*static*/ bool DbConnection::created = false;

void test(std::ostream &opFile)
{
  //std::ofstream opFile( "test_result.txt" );
  DbConnection dbConn( "localhost", "openstreetmap", "openstreetmap", "openstreetmap" );

  double minLat = 515000000;
  double maxLat = 520000000;
  double minLon = -10000000;
  double maxLon = -9000000;

  dbConn.execute_noresult( "CREATE TEMPORARY TABLE temp_node_ids( id int, PRIMARY KEY(id) )" );
  dbConn.execute_noresult( "CREATE TEMPORARY TABLE temp_way_ids( id int, PRIMARY KEY(id) )" );

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

    opFile << "Node id: " << nodeId << ", lat: " << latitude << ", lon: " << longitude << std::endl;
    
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
	  throw SqlException( boost::str( boost::format( "Node tag key value pair split failed: %s (%s)" ) % tag % tagString ) );
	}

        opFile << "  Tag: " << keyValue[0] << " - " << keyValue[1] << std::endl;
      }
    }
  }
  while ( dbConn.next() );

  // Also output: ways, way_nodes, relations

  dbConn.execute_noresult( "DROP TABLE temp_way_ids" );
  dbConn.execute_noresult( "DROP TABLE temp_node_ids" );

  // Similarly: SELECT ways.* FROM ways INNER JOIN temp_way_ids ON ways.id=temp_way_ids.id
  //            SELECT way_nodes.* FROM way_nodes INNER JOIN temp_way_ids ON way_nodes.id=temp_way_ids.id
  //            (and way_tags and relations)
}

extern "C"
{
static int osm_handler(request_rec* r)
{
	if (!r->handler || strcmp(r->handler, "osm"))
		return DECLINED;
	
	if (r->method_number != M_GET)
		return HTTP_METHOD_NOT_ALLOWED;

        apache_stream as;
        as.open (Sink ());
        as->r = r;
	
	ap_set_content_type(r, "text/html;charset=ascii");
	test (as);
/*
	as << "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\">\n";
	as << "<html><head><title>How surprising!</title></head>";
	as << "<body><h1>Wow 2!</h1></body></html>";
*/
	return OK;
}

static void register_hooks(apr_pool_t* pool)
{
	ap_hook_handler(osm_handler, NULL, NULL, APR_HOOK_MIDDLE);
}

module AP_MODULE_DECLARE_DATA osm_module = {
	STANDARD20_MODULE_STUFF,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	register_hooks
};
}
