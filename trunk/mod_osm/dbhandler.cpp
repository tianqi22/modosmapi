#include "dbhandler.hpp"

namespace modosmapi
{

    DbConnection::DbConnection(
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

    void DbConnection::cleanUp()
    {
        if ( m_res )
        {
            mysql_free_result( m_res );
            m_res = NULL;
            m_row = NULL;
        }
    }



    void DbConnection::executeNoResult( std::string query )
    {
        cleanUp();

        if ( mysql_query( &m_dbconn, query.c_str() ) )
        {
            throw SqlException( std::string( "Query failed: " ) + mysql_error( &m_dbconn ) );
        }    
    }


    void DbConnection::execute( std::string query )
    {
        cleanUp();

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

    bool DbConnection::next()
    {
        if ( !m_res )
        {
            throw SqlException( "Cannot call next - no valid result" );
        }

        m_row = mysql_fetch_row( m_res );

        return m_row;
    }


    void DbConnection::bindArg( MYSQL_BIND &args, const std::string &value )
    {
        args.buffer_type = MYSQL_TYPE_STRING;

        // TODO: WILL NOT WORK - BUFFER NOT AVAILABLE FOR LONG ENOUGH
        args.buffer = const_cast<char *>( "BLAH" );
        args.buffer_length = 4;
        args.is_null = 0;

        // TODO: WHAT TO DO ABOUT THIS RETURNED LENGTH?
        unsigned long str_length;
        args.length = &str_length;
    }

    void DbConnection::bindArg( MYSQL_BIND &args, const int &value )
    {
        args.buffer_type = MYSQL_TYPE_LONG;
        args.buffer = reinterpret_cast<char *>( const_cast<int *>( &value ) );
        args.is_unsigned = 0;
        args.is_null = 0;
        args.length = 0;
    }


    void DbConnection::bindArg( MYSQL_BIND &args, const boost::uint64_t &value )
    {
        args.buffer_type = MYSQL_TYPE_LONGLONG;
        args.buffer = reinterpret_cast<char *>( const_cast<boost::uint64_t *>( &value ) );
        args.is_unsigned = 1;
        args.is_null = 0;
        args.length = 0;
    }

    void DbConnection::bindArg( MYSQL_BIND &args, const boost::int64_t &value )
    {
        args.buffer_type = MYSQL_TYPE_LONGLONG;
        args.buffer = reinterpret_cast<char *>( const_cast<boost::int64_t *>( &value ) );
        args.is_unsigned = 0;
        args.is_null = 0;
        args.length = 0;
    }
    
    void DbConnection::bindArg( MYSQL_BIND &args, const double &value )
    {
        args.buffer_type = MYSQL_TYPE_DOUBLE;
        args.buffer = reinterpret_cast<char *>( const_cast<double *>( &value ) );
        args.is_null = 0;
    }

    void DbConnection::bindArg( MYSQL_BIND &args, const bool &value )
    {
    }

    void DbConnection::bindArg( MYSQL_BIND &args, const boost::posix_time::ptime &datetime )
    {
        MYSQL_TIME ts;
        ts.year   = datetime.date().year();
        ts.month  = datetime.date().month();
        ts.day    = datetime.date().day();
        ts.hour   = datetime.time_of_day().hours();
        ts.minute = datetime.time_of_day().minutes();
        ts.second = datetime.time_of_day().seconds();

        args.buffer_type = MYSQL_TYPE_TIMESTAMP;
        args.buffer = reinterpret_cast<char *>( &ts );
        args.is_null = 0;
        args.length = 0;
    }


    DbConnection::~DbConnection()
    {
        cleanUp();

        mysql_close( &m_dbconn );
    }

    /*static*/ bool DbConnection::created = false;
};


