#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>


namespace modosmapi
{
    template<typename T>
    void DbConnection::execute_bulk_insert( std::string query, const std::vector<T> &rows )
    {
        MYSQL_STMT *ps = mysql_stmt_init( &m_dbconn );
        if ( !ps )
        {
            throw SqlException( std::string( "Statement init failed: " ) + mysql_error( &m_dbconn ) );
        }

        if ( mysql_stmt_prepare( ps, query.c_str(), query.size() ) )
        {
            throw SqlException( std::string( "Statement prepare failed: " ) + mysql_error( &m_dbconn ) );
        }

        MYSQL_BIND args[boost::tuples::length<T>::value];

        BOOST_FOREACH( const T &row, rows )
        {
            memset( args, 0, sizeof( args ) );

            bindArgsRec( args, row, 0 );

            mysql_stmt_bind_param( ps, args );

            if ( mysql_stmt_execute( ps ) )
            {
                throw SqlException( std::string( "Statement execute failed: " ) + mysql_error( &m_dbconn ) );
            }
        }


        mysql_stmt_close( ps );

    }

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
    void DbConnection::bindArgsRec( MYSQL_BIND *args, boost::tuples::cons<T1, T2> &row, int offset )
    { 
        bindArg( args, row.head, offset );
        bindArgsRec( args, row.tail, offset+1 );
    }

    template<typename T>
    void DbConnection::bindArgsRec( MYSQL_BIND *args, boost::tuples::cons<T, boost::tuples::null_type> &row, int offset )
    {
        bindArg( args, row.head, offset );
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

}
