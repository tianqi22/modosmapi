#include "dbhandler.hpp"

namespace modosmapi
{
    static const size_t stringBufferSize = 8192;

    BindArgDataHolder::BindArgDataHolder( MYSQL *dbConn, MYSQL_STMT *ps ) : m_dbConn( dbConn ), m_ps( ps )
    {
    }

    void BindArgDataHolder::ArgBinder::setMem( MYSQL_BIND &arg, enum enum_field_types type )
    {
        ArgParams_t *paramData = new ArgParams_t;
        boost::shared_array<char> data( reinterpret_cast<char *>( paramData ) );
        m_argMem.push_back( data );
        
        arg.buffer_type = type;
        arg.is_null = &paramData->is_null;
        arg.length  = &paramData->length;
        arg.error   = &paramData->error;
    }

    void BindArgDataHolder::ArgBinder::operator()( MYSQL_BIND &arg, const std::string &value )
    {
        boost::shared_array<char> data( new char[stringBufferSize] );
        m_argMem.push_back( data );

        setMem( arg, MYSQL_TYPE_STRING );
        arg.buffer = data.get();
        arg.buffer_length = stringBufferSize;
        *arg.is_null = 0;
    }


    void BindArgDataHolder::ArgBinder::operator()( MYSQL_BIND &arg, const int & )
    {
        boost::shared_array<char> data( reinterpret_cast<char *>( new int ) );
        m_argMem.push_back( data );

        setMem( arg, MYSQL_TYPE_LONG );
        arg.buffer_type = MYSQL_TYPE_LONG;
        arg.buffer = data.get();
        arg.is_unsigned = 0;
        *arg.is_null = 0;
    }


    void BindArgDataHolder::ArgBinder::operator()( MYSQL_BIND &arg, const boost::uint64_t &value )
    {
        boost::shared_array<char> data( reinterpret_cast<char *>( new boost::uint64_t ) );
        m_argMem.push_back( data );

        setMem( arg,  MYSQL_TYPE_LONGLONG );
        arg.buffer = data.get();
        arg.is_unsigned = 1;
        *arg.is_null = 0;
    }

    void BindArgDataHolder::ArgBinder::operator()( MYSQL_BIND &arg, const boost::int64_t &value )
    {
        boost::shared_array<char> data( reinterpret_cast<char *>( new boost::int64_t ) );
        m_argMem.push_back( data );

        setMem( arg, MYSQL_TYPE_LONGLONG );
        arg.buffer = data.get();
        arg.is_unsigned = 0;
        *arg.is_null = 0;
    }
    
    void BindArgDataHolder::ArgBinder::operator()( MYSQL_BIND &arg, const double &value )
    {
        boost::shared_array<char> data( reinterpret_cast<char *>( new double ) );
        m_argMem.push_back( data );

        setMem( arg, MYSQL_TYPE_DOUBLE );
        arg.buffer = data.get();
        arg.is_null = NULL;
    }

    void BindArgDataHolder::ArgBinder::operator()( MYSQL_BIND &arg, const bool &value )
    {
        boost::shared_array<char> data( reinterpret_cast<char *>( new char ) );
        m_argMem.push_back( data );

        setMem( arg, MYSQL_TYPE_TINY );
        arg.buffer = data.get();
        arg.is_null = NULL;
    }

    void BindArgDataHolder::ArgBinder::operator()( MYSQL_BIND &arg, const boost::posix_time::ptime &datetime )
    {
        boost::shared_array<char> buf( new char[sizeof(MYSQL_TIME)/sizeof(char)] );
        m_argMem.push_back( buf );
        
        setMem( arg, MYSQL_TYPE_TIMESTAMP );
        arg.buffer = buf.get();
        *arg.is_null = 0;
        *arg.length = 0;
    }

    void BindArgDataHolder::ArgGetter::operator()( MYSQL_BIND &arg, std::string &value )
    {
        value = std::string( reinterpret_cast<char *>( arg.buffer ), *arg.length );
    }

    void BindArgDataHolder::ArgGetter::operator()( MYSQL_BIND &arg, int &value )
    {
        value = *reinterpret_cast<int *>( arg.buffer );
    }

    void BindArgDataHolder::ArgGetter::operator()( MYSQL_BIND &arg, boost::uint64_t &value )
    {
        value = *reinterpret_cast<boost::uint64_t *>( arg.buffer );
    }

    void BindArgDataHolder::ArgGetter::operator()( MYSQL_BIND &arg, boost::int64_t &value )
    {
        value = *reinterpret_cast<boost::int64_t *>( arg.buffer );
    }

    void BindArgDataHolder::ArgGetter::operator()( MYSQL_BIND &arg, double &value )
    {
        value = *reinterpret_cast<double *>( arg.buffer );
    }

    void BindArgDataHolder::ArgGetter::operator()( MYSQL_BIND &arg, bool &value )
    {
        value = *reinterpret_cast<my_bool *>( arg.buffer );
    }

    void BindArgDataHolder::ArgGetter::operator()( MYSQL_BIND &arg, boost::posix_time::ptime &datetime )
    {
        MYSQL_TIME *ts = reinterpret_cast<MYSQL_TIME *>( arg.buffer );

        datetime = boost::posix_time::ptime(
            boost::gregorian::date( ts->year, ts->month, ts->day ),
            boost::posix_time::time_duration( ts->hour, ts->minute, ts->second ) );
    }

    void BindArgDataHolder::ArgSetter::operator()( MYSQL_BIND &arg, const std::string &value )
    {
        // TODO: Make sure this doesn't exceed the buffer size
        if ( value.size() >= stringBufferSize )
        {
            throw ModException( "String buffer size exceeded" );
        }
        memcpy( arg.buffer, value.c_str(), value.size() );
        *arg.length = ((long unsigned int) value.size());
    }

    void BindArgDataHolder::ArgSetter::operator()( MYSQL_BIND &arg, const int &value )
    {
        *reinterpret_cast<int *>( arg.buffer ) = value;
    }

    void BindArgDataHolder::ArgSetter::operator()( MYSQL_BIND &arg, const boost::uint64_t &value )
    {
        *reinterpret_cast<uint64_t *>( arg.buffer ) = value;
    }

    void BindArgDataHolder::ArgSetter::operator()( MYSQL_BIND &arg, const boost::int64_t &value )
    {
        *reinterpret_cast<int64_t *>( arg.buffer ) = value;
    }

    void BindArgDataHolder::ArgSetter::operator()( MYSQL_BIND &arg, const double &value )
    {
        *reinterpret_cast<double *>( arg.buffer ) = value;
    }

    void BindArgDataHolder::ArgSetter::operator()( MYSQL_BIND &arg, const bool &value )
    {
    }

    void BindArgDataHolder::ArgSetter::operator()( MYSQL_BIND &arg, const boost::posix_time::ptime &datetime )
    {
        MYSQL_TIME *ts = reinterpret_cast<MYSQL_TIME *>( arg.buffer );

        ts->year   = datetime.date().year();
        ts->month  = datetime.date().month();
        ts->day    = datetime.date().day();
        ts->hour   = datetime.time_of_day().hours();
        ts->minute = datetime.time_of_day().minutes();
        ts->second = datetime.time_of_day().seconds();

    }

    DbConnection::DbConnection(
        const std::string &dbhost,
        const std::string &dbname,
        const std::string &dbuser,
        const std::string &dbpass ) : m_res( NULL ), m_row( NULL )
    {
        if ( created )
        {
            throw ModException( "Single instance has already been created" );
        }
        created = true;
        
        if ( !mysql_init( &m_dbconn ) )
        {
            throw ModException( std::string( "Mysql init failure: " ) + mysql_error( &m_dbconn ) );
        }
        
        if ( !mysql_real_connect( &m_dbconn, dbhost.c_str(), dbname.c_str(), dbuser.c_str(), dbpass.c_str(), 0, NULL, 0 ) )
        {
            throw ModException( std::string( "Mysql connection failed: " ) + mysql_error( &m_dbconn ) );
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
        m_bulkInsertBuf.clear();
    }


    void DbConnection::executeNoResult( std::string query )
    {
        cleanUp();

        if ( mysql_query( &m_dbconn, query.c_str() ) )
        {
            throw ModException( std::string( "Query failed: " ) + mysql_error( &m_dbconn ) );
        }    
    }


    void DbConnection::execute( std::string query )
    {
        cleanUp();

        if ( mysql_query( &m_dbconn, query.c_str() ) )
        {
            throw ModException( std::string( "Query failed: " ) + mysql_error( &m_dbconn ) );
        }
        
        if ( !(m_res=mysql_use_result( &m_dbconn )) )
        {
            throw ModException( std::string( "Result store failed: " ) + mysql_error( &m_dbconn ) );
        }
    }

    bool DbConnection::next()
    {
        if ( !m_res )
        {
            throw ModException( "Cannot call next - no valid result" );
        }

        m_row = mysql_fetch_row( m_res );

        return m_row;
    }

    DbConnection::~DbConnection()
    {
        cleanUp();

        mysql_close( &m_dbconn );
        
        created = false;
    }

    /*static*/ bool DbConnection::created = false;
};


