#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>


namespace modosmapi
{

   
    template<typename T>
    T DbConnection::getField( size_t index )
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
    void DbConnection::readRow( boost::tuples::cons<T1, T2> &t, size_t index )
    {
        t.head = getField<T1>( index );
        readRow( t.tail, index + 1 );
    }

    template<typename T>
    void DbConnection::readRow( boost::tuples::cons<T, boost::tuples::null_type> &t, size_t index )
    {
        t.head = getField<T>( index );
    }

    template<typename T>
    void DbConnection::executeBulkRetrieve( std::string query, boost::function<void( const T & )> fn )
    {
        cleanUp();

        MYSQL_STMT *ps = mysql_stmt_init( &m_dbconn );
        if ( !ps )
        {
            throw SqlException( std::string( "Statement init failed: " ) + mysql_error( &m_dbconn ) );
        }

        if ( mysql_stmt_prepare( ps, query.c_str(), query.size() ) )
        {
            throw SqlException( std::string( "Statement prepare failed: " ) + mysql_error( &m_dbconn ) );
        }

        if ( mysql_stmt_execute( ps ) )
        {
            throw SqlException( "Query execution failed" );
        }

        BindArgDataHolder dh( &m_dbconn, ps );

        T row;
        dh.bindResults( row );

        while ( !mysql_stmt_fetch( ps ) )
        {
            dh.getArgs( row );
            fn( row );
        }

        mysql_stmt_close( ps );
    }

    template<typename T>
    void DbConnection::executeBulkRetrieve( std::string query, std::vector<T> &result )
    {
        boost::function<void( const T & )> fn = boost::bind( &std::vector<T>::push_back, boost::ref( result ), _1 );
        executeBulkRetrieve( query, fn );
    }


    template<typename T>
    void DbConnection::executeBulkInsert( std::string query, const std::vector<T> &rows )
    {
        cleanUp();

        MYSQL_STMT *ps = mysql_stmt_init( &m_dbconn );
        if ( !ps )
        {
            throw SqlException( std::string( "Statement init failed: " ) + mysql_error( &m_dbconn ) );
        }
    
        if ( mysql_stmt_prepare( ps, query.c_str(), query.size() ) )
        {
            throw SqlException( std::string( "Statement prepare failed: " ) + mysql_error( &m_dbconn ) );
        }


        BindArgDataHolder dh( &m_dbconn, ps );
        T temp;
        dh.bindParams( temp );

        BOOST_FOREACH( const T &row, rows )
        {
            dh.setArgs( row );

            if ( mysql_stmt_execute( ps ) )
            {
                throw SqlException( std::string( "Statement execute failed: " ) + mysql_error( &m_dbconn ) );
            }
        }

        mysql_stmt_close( ps );
    }
}
